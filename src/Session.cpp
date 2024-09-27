#include "Session.h"
Session::Session(Connection *conn, boost::asio::io_context &ioContext, int timeout)
    : conn_(conn),
      socket_(std::make_shared<boost::asio::ip::tcp::socket>(ioContext)),
      timeout_(timeout),
      timer_(ioContext, boost::posix_time::microseconds(timeout)) {
}

std::shared_ptr<boost::asio::ip::tcp::socket> Session::getSocket() {
    return socket_;
}

std::string Session::ip() {
    if (!socket_ || !socket_->is_open()) {
        return "";
    }
    try {
        return socket_->remote_endpoint().address().to_string();
    } catch (const boost::system::system_error &e) {
        return "";
    }
}

int Session::port() {
    if (!socket_ || !socket_->is_open()) {
        return 0;
    }
    try {
        return socket_->remote_endpoint().port();
    } catch (const boost::system::system_error &e) {
        return 0;
    }
}

void Session::start() {
    isClose_ = false;
    syncRecv();
    startTimer();
}

void Session::syncRecv() {
    ::memset(recvBuf_, 0, BUFFER_MAX_LEN);
    socket_->async_read_some(boost::asio::buffer(recvBuf_, BUFFER_MAX_LEN),
                             std::bind(&Session::readHandle, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void Session::readHandle(const boost::system::error_code &error, size_t len) {
    std::string err;
    if (!error) {
        if (isClose_) {
            return;
        }
        syncRecv();
    } else {
        err = error.what();
        close();
    }
    conn_->onRead(ip(), port(), recvBuf_, len, err);
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
    boost::asio::async_write(*socket_, boost::asio::buffer(msg.data(), msg.size()),
                             std::bind(&Session::writeHandle, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void Session::writeHandle(const boost::system::error_code &error, size_t len) {
    std::string data, err;
    if (!error) {
        if (isClose_) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(sendLock_);
            if (sendBuf_.size() > 0) {
                data = sendBuf_.front();
                sendBuf_.pop();
                if (data.length() > len) {
                    err = std::string("[error]too length data, max len is:" + std::to_string(len));
                }
            } else {
                err = "[error]sendBuf is no data";
                return;
            }
        }
        syncSend(data);
    } else {
        err = error.what();
        close();
    }
    conn_->onWrite(ip(), port(), data, err);
}

void Session::close() {
    isClose_ = true;
    if (socket_->is_open()) {
        boost::system::error_code ignored_ec;
        socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        socket_->close(ignored_ec);
        std::string errStr;
        if (ignored_ec) {
            errStr = ignored_ec.what();
        }
        conn_->doClose(ip(), port(), errStr);
    }
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
    conn_->onTimer(ip(), port());
    startTimer();
}