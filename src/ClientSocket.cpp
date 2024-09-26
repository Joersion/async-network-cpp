#include "ClientSocket.h"

ClientSocket::ClientSocket(const std::string& ip, int port, int timeout)
    : ioContext_(),
      remote_(boost::asio::ip::address::from_string(ip), port),
      resolver_(ioContext_),
      timeout_(timeout),
      reconnectTimer_(ioContext_) {
}

void ClientSocket::start(std::function<void(const std::string&)> errorFunc, int reconncetTime) {
    reconnectTimeout_ = reconncetTime;
    errorCb_ = errorFunc;
    resolver();
    ioContext_.run();
}

void ClientSocket::send(const std::string& data) {
    if (session_) {
        session_->send(data.data(), data.length());
    }
}

void ClientSocket::resolver() {
    resolver_.async_resolve(remote_, std::bind(&ClientSocket::resolverHandle, this, std::placeholders::_1,
                                               std::placeholders::_2));
}

void ClientSocket::resolverHandle(const boost::system::error_code& error,
                                  boost::asio::ip::tcp::resolver::results_type endpoints) {
    if (!error) {
        syncConnect(endpoints);
    } else {
        if (errorCb_) {
            errorCb_(error.what());
        }
        startTimer();
    }
}

void ClientSocket::syncConnect(boost::asio::ip::tcp::resolver::results_type endpoints) {
    std::shared_ptr<Session> session = std::make_shared<Session>(this, ioContext_, timeout_);
    if (!session.get()) {
        return;
    }
    session->installCloseCb([this](const std::string& ip) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            session_.reset();
        }
        startTimer();
    });
    auto socket = session->getSocket();
    boost::asio::async_connect(*socket, endpoints,
                               std::bind(&ClientSocket::ConnectHandle, this, session, std::placeholders::_1,
                                         std::placeholders::_2));
}

void ClientSocket::ConnectHandle(std::shared_ptr<Session> session, const boost::system::error_code& error,
                                 const boost::asio::ip::tcp::endpoint& endpoint) {
    if (!error) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            session_ = session;
        }
        session->start();
    } else {
        session->close(error.what());
    }
}

void ClientSocket::startTimer() {
    if (reconnectTimeout_ > 0) {
        reconnectTimer_.cancel();
        reconnectTimer_.expires_from_now(boost::posix_time::milliseconds(reconnectTimeout_));
        reconnectTimer_.async_wait(std::bind(&ClientSocket::timerHandle, this));
    }
}

void ClientSocket::timerHandle() {
    resolver();
}