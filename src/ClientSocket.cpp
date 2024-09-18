#include "ClientSocket.h"

ClientSocket::ClientSocket(const std::string& ip, int port, std::unique_ptr<SessionFactory> factory, int timeout)
    : ioContext_(),
      remote_(boost::asio::ip::address::from_string(ip), port),
      resolver_(ioContext_),
      factory_(std::move(factory)),
      timeout_(timeout),
      reconnectTimer_(ioContext_) {
}

void ClientSocket::resolver() {
    resolver_.async_resolve(remote_, std::bind(&ClientSocket::resolverHandle, this, std::placeholders::_1,
                                               std::placeholders::_2));
}

void ClientSocket::start() {
    resolver();
    ioContext_.run();
}

void ClientSocket::resolverHandle(const boost::system::error_code& error,
                                  boost::asio::ip::tcp::resolver::results_type endpoints) {
    auto session = factory_->create(ioContext_, timeout_);
    if (!error) {
        session->start(endpoints);
    } else {
        session->close(error.what());
        startTimer();
    }
}

void ClientSocket::startTimer() {
    reconnectTimer_.expires_from_now(boost::posix_time::milliseconds(timeout_));
    reconnectTimer_.async_wait(std::bind(&ClientSocket::timerHandle, this));
}

void ClientSocket::timerHandle() {
    resolver();
}