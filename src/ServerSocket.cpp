#include "ServerSocket.h"

ServerSocket::ServerSocket(int port, std::unique_ptr<SessionFactory> factory, int timeout)
    : ioContext_(),
      acceptor_(ioContext_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      factory_(std::move(factory)),
      timeout_(timeout) {
}

void ServerSocket::start() {
    accept();
    ioContext_.run();
}

void ServerSocket::accept() {
    auto session = factory_->create(ioContext_, timeout_);
    session->installCloseCb([this](const std::string &ip) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(ip);
    });
    acceptor_.async_accept(session->socket(),
                           std::bind(&ServerSocket::acceptHandle, this, session, std::placeholders::_1));
}

void ServerSocket::acceptHandle(std::shared_ptr<Session> &session, const boost::system::error_code &error) {
    if (!error) {
        session->start();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            sessions_[session->ip()] = std::move(session);
        }
        accept();
    } else {
        session->close(error.what());
    }
}

void ServerSocket::send(const std::string &ip, const char *msg, int len) {
    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(ip);
        if (it != sessions_.end()) {
            session = sessions_[ip];
        }
    }
    if (session.get()) {
        session->send(msg, len);
    }
}