#include "ClientConnection.h"

ClientConnection::ClientConnection(const std::string& ip, int port, int timeout)
    : ioContext_(), remote_(boost::asio::ip::address::from_string(ip), port), resolver_(ioContext_), timeout_(timeout), reconnectTimer_(ioContext_) {
}

void ClientConnection::start(int reconncetTime) {
    reconnectTimeout_ = reconncetTime;
    resolver();
    ioContext_.run();
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
    onResolver(err);
}

void ClientConnection::doClose(const std::string& ip, int port, const std::string& error) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        session_.reset();
    }
    startTimer();
    onClose(ip, port, error);
}

void ClientConnection::syncConnect(boost::asio::ip::tcp::resolver::results_type endpoints) {
    std::shared_ptr<Session> session = std::make_shared<Session>(this, ioContext_, timeout_);
    if (!session.get()) {
        return;
    }
    auto socket = session->getSocket();
    boost::asio::async_connect(*socket, endpoints,
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
    onConnect(ip, port, err);
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