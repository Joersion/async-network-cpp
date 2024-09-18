#include "ClientSocket.h"

ClientSocket::ClientSocket(const std::string& ip, int port, std::unique_ptr<SessionFactory> factory, int timeout)
    : ioContext_(),
      remote_(boost::asio::ip::address::from_string(ip), port),
      resolver_(ioContext_),
      factory_(std::move(factory)),
      timeout_(timeout) {
}

void ClientSocket::start() {
    resolver_.async_resolve(remote_, std::bind(&ClientSocket::resolverHandle, this, std::placeholders::_1,
                                               std::placeholders::_2));
    ioContext_.run();
}

void ClientSocket::resolverHandle(const boost::system::error_code& error,
                                  boost::asio::ip::tcp::resolver::results_type endpoints) {
    auto session = factory_->create(ioContext_, timeout_);
    if (!error) {
        session->start(endpoints);
    } else {
        session->close(error.what());
    }
}