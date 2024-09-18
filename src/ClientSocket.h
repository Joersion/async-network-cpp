#pragma once

#include <boost/asio.hpp>

#include "Session.h"
#include "SessionFactory.h"

class ClientSocket {
public:
    ClientSocket(const std::string& ip, int port, std::unique_ptr<SessionFactory> factory, int timeout = 0);
    ~ClientSocket() = default;
    void start();

private:
    void resolver();
    void resolverHandle(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type endpoints);
    void startTimer();
    void timerHandle();

private:
    boost::asio::ip::tcp::endpoint remote_;
    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::resolver resolver_;
    std::unique_ptr<SessionFactory> factory_;
    int timeout_;
    boost::asio::deadline_timer reconnectTimer_;
};