#pragma once
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include "Session.h"
#include "SessionFactory.h"

class ServerSocket {
public:
    ServerSocket(int port, std::unique_ptr<SessionFactory> factory, int timeout = 0);
    ~ServerSocket() = default;
    void start();

private:
    void accept();
    void acceptHandle(std::shared_ptr<Session> &session, const boost::system::error_code &error);

private:
    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::map<std::string, std::shared_ptr<Session>> sessions_;
    std::unique_ptr<SessionFactory> factory_;
    std::mutex mutex_;
    int timeout_;
};
