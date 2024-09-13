#include "ServerSocket.h"

ServerSocket::ServerSocket(int port, std::unique_ptr<SessionFactory> factory, int timeout)
    : ioContext(),
      acceptor_(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      factory_(std::move(factory)),
      timeout_(timeout) {
}

void ServerSocket::start() {
    accept();
    ioContext.run();
}

void ServerSocket::accept() {
    auto session = factory_->create(ioContext, timeout_);
    session->installCloseCb([this](const std::string &ip) {
        std::lock_guard<std::mutex> lock();
        sessions_.erase(ip);
    });
    acceptor_.async_accept(session->socket(),
                           std::bind(&ServerSocket::acceptHandle, this, session, std::placeholders::_1));
}

void ServerSocket::acceptHandle(std::shared_ptr<Session> &session, const boost::system::error_code &error) {
    if (!error) {
        session->start();
        {
            std::lock_guard<std::mutex> lock();
            sessions_[session->ip()] = std::move(session);
        }
        accept();
    } else {
        std::cout << "handle acceptHandle failed, error is " << error.what() << std::endl;
    }
}