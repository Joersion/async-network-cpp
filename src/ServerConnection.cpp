#include "ServerConnection.h"

#include "ConnectionPool.h"

ServerConnection::ServerConnection(int port, int timeout)
    : ioContext_(ConnectionPool::instance().getContext()),
      acceptor_(boost::asio::make_strand(ioContext_), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      timeout_(timeout) {
}

void ServerConnection::start() {
    accept();
}

void ServerConnection::doClose(const std::string &ip, int port, const std::string &error) {
    delSession(ip);
    onClose(ip, port, error);
}

void ServerConnection::accept() {
    std::shared_ptr<Session> session = std::make_shared<Session>(this, ioContext_, timeout_);
    if (!session.get()) {
        return;
    }
    auto socket = session->getSocket();
    acceptor_.async_accept(*socket, std::bind(&ServerConnection::acceptHandle, this, session, std::placeholders::_1));
}

void ServerConnection::acceptHandle(std::shared_ptr<Session> session, const boost::system::error_code &error) {
    std::string ip = session->ip();
    int port = session->port();
    std::string err;
    if (!error) {
        addSession(session);
        session->start();
        accept();
    } else {
        err = error.what();
        session->close();
    }
    if (error != boost::asio::error::operation_aborted) {
        onConnect(ip, port, err);
    }
}

void ServerConnection::send(const std::string &ip, const std::string &msg) {
    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(ip);
        if (it != sessions_.end()) {
            session = sessions_[ip];
        }
    }
    if (session.get()) {
        session->send(msg.data(), msg.length());
    }
}

void ServerConnection::addSession(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[session->ip()] = session;
}

void ServerConnection::delSession(const std::string &ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(ip);
}