#include "Session.h"
Session::Session(boost::asio::io_context &ioContext, int timeout)
    : socket_(ioContext), timeout_(timeout), timer_(ioContext, boost::posix_time::microseconds(timeout)) {
}

void Session::start() {
    isClose_ = false;
    onConnect();
    syncRecv();
    startTimer();
}

void Session::start(boost::asio::ip::tcp::resolver::results_type endpoints) {
    syncConnect(endpoints);
}

void Session::syncRecv() {
    ::memset(recvBuf_, 0, BUFFER_MAX_LEN);
    socket_.async_read_some(boost::asio::buffer(recvBuf_, BUFFER_MAX_LEN),
                            std::bind(&Session::readHandle, shared_from_this(), std::placeholders::_1,
                                      std::placeholders::_2));
}

void Session::readHandle(const boost::system::error_code &error, size_t len) {
    if (!error) {
        if (isClose_) {
            return;
        }
        onRead(recvBuf_, len);
        syncRecv();
    } else {
        close(error.what());
    }
}

void Session::send(const char *msg, size_t len) {
    if (isClose_) {
        return;
    }
    std::string data;
    {
        std::lock_guard<std::mutex> lock(sendLock_);
        if (sendBuf_.size() > 0) {
            sendBuf_.push(msg);
            return;
        }
        sendBuf_.push(msg);
    }
    syncSend(data);
}

void Session::syncSend(const std::string &msg) {
    boost::asio::async_write(socket_, boost::asio::buffer(msg.data(), msg.size()),
                             std::bind(&Session::writeHandle, shared_from_this(), std::placeholders::_1,
                                       std::placeholders::_2));
}

void Session::writeHandle(const boost::system::error_code &error, size_t len) {
    if (!error) {
        if (isClose_) {
            return;
        }
        onWrite();
        std::string data;
        {
            std::lock_guard<std::mutex> lock(sendLock_);
            if (sendBuf_.size() > 0) {
                data = sendBuf_.front();
                sendBuf_.pop();
            } else {
                return;
            }
        }
        syncSend(data);
    } else {
        close(error.what());
    }
}

void Session::close(const std::string &error) {
    isClose_ = true;
    onClose(error);
    if (closeCb_) {
        closeCb_(ip());
    }

    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);

    timeout_ = 0;
    timer_.cancel();

    std::lock_guard<std::mutex> lock(sendLock_);
    sendBuf_ = std::move(std::queue<std::string>());
}

void Session::startTimer() {
    if (timeout_ == 0 || isClose_) {
        return;
    }
    timer_.expires_from_now(boost::posix_time::milliseconds(timeout_));
    timer_.async_wait(std::bind(&Session::timerHandle, shared_from_this()));
}

void Session::timerHandle() {
    if (timeout_ == 0 || isClose_) {
        return;
    }
    onTimer();
    startTimer();
}

void Session::syncConnect(boost::asio::ip::tcp::resolver::results_type endpoints) {
    boost::asio::async_connect(socket_, endpoints,
                               std::bind(&Session::ConnectHandle, shared_from_this(), std::placeholders::_1,
                                         std::placeholders::_2));
}

void Session::ConnectHandle(const boost::system::error_code &error, const boost::asio::ip::tcp::endpoint &endpoint) {
    if (!error) {
        start();
    } else {
        close(error.what());
    }
}