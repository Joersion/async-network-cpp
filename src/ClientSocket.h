#pragma once

#include <boost/asio.hpp>

#include "Session.h"
#include "SessionFactory.h"

class ClientSocket : public std::enable_shared_from_this<ClientSocket> {
public:
    ClientSocket(const std::string& ip, int port, std::unique_ptr<SessionFactory> factory, int timeout = 0);
    ~ClientSocket() = default;

private:
    void resolver();
    void resolverHandle(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type endpoints);
    void startTimer();
    void timerHandle();
    void start();

private:
    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::endpoint remote_;
    boost::asio::ip::tcp::resolver resolver_;
    std::unique_ptr<SessionFactory> factory_;
    int timeout_;
    boost::asio::deadline_timer reconnectTimer_;
};