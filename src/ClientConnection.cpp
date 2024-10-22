#include "ClientConnection.h"

#include "ConnectionPool.h"

ClientConnection::ClientConnection(const std::string& ip, int port, int timeout)
    : ioContext_(ConnectionPool::instance().getContext()),
      remote_(boost::asio::ip::address::from_string(ip), port),
      resolver_(boost::asio::make_strand(ioContext_)),
      timeout_(timeout),
      reconnectTimer_(boost::asio::make_strand(ioContext_)) {
}

void ClientConnection::start(int reconncetTime) {
    reconnectTimeout_ = reconncetTime;
    resolver();
}

void ClientConnection::send(const std::string& data) {
    if (session_) {
        session_->send(data.data(), data.length());
    }
}

void ClientConnection::resolver() {
    resolver_.async_resolve(remote_, std::bind(&ClientConnection::resolverHandle, this, std::placeholders::_1, std::placeholders::_2));
}

void ClientConnection::resolverHandle(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type endpoints) {
    std::string err;
    if (!error) {
        syncConnect(endpoints);
    } else {
        startTimer();
    }
    if (error != boost::asio::error::operation_aborted) {
        onResolver(err);
    }
}

void ClientConnection::doClose(const std::string& ip, int port, const std::string& error) {
    startTimer();
    if (!ip.empty()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            session_.reset();
        }
        onClose(ip, port, error);
    }
}

void ClientConnection::syncConnect(boost::asio::ip::tcp::resolver::results_type endpoints) {
    std::shared_ptr<Session> session = std::make_shared<Session>(this, ioContext_, timeout_);
    if (!session.get()) {
        return;
    }
    boost::asio::async_connect(session->getSocket(), endpoints,
                               std::bind(&ClientConnection::ConnectHandle, this, session, std::placeholders::_1, std::placeholders::_2));
}

void ClientConnection::ConnectHandle(std::shared_ptr<Session> session, const boost::system::error_code& error,
                                     const boost::asio::ip::tcp::endpoint& endpoint) {
    std::string ip = session->ip();
    int port = session->port();
    std::string err;
    if (!error) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            session_ = session;
        }
        session->start();
    } else {
        err = error.what();
        session->close();
    }
    if (error != boost::asio::error::operation_aborted) {
        onConnect(ip, port, err);
    }
}

void ClientConnection::startTimer() {
    if (reconnectTimeout_ > 0) {
        reconnectTimer_.cancel();
        reconnectTimer_.expires_from_now(boost::posix_time::milliseconds(reconnectTimeout_));
        reconnectTimer_.async_wait(std::bind(&ClientConnection::timerHandle, this));
    }
}

void ClientConnection::timerHandle() {
    resolver();
}