#include "ServerSocket.h"

ServerSocket::ServerSocket(int port, int timeout)
    : ioContext_(),
      acceptor_(ioContext_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      timeout_(timeout) {
}

void ServerSocket::start() {
    accept();
    ioContext_.run();
}

void ServerSocket::accept() {
    std::shared_ptr<Session> session = std::make_shared<Session>(this, ioContext_, timeout_);
    if (!session.get()) {
        return;
    }
    session->installCloseCb([this](const std::string &ip) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(ip);
    });
    auto socket = session->getSocket();
    acceptor_.async_accept(*socket, std::bind(&ServerSocket::acceptHandle, this, session, std::placeholders::_1));
}

void ServerSocket::acceptHandle(std::shared_ptr<Session> session, const boost::system::error_code &error) {
    if (!error) {
        addSession(session);
        session->start();
        accept();
    } else {
        session->close(error.what());
    }
}

void ServerSocket::send(const std::string &ip, const std::string &msg) {
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

void ServerSocket::addSession(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[session->ip()] = session;
}

void ServerSocket::delSession(const std::string &ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(ip);
}