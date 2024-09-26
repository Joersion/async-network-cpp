#pragma once

#include "Session.h"

class ClientSocket : public Connection {
public:
    ClientSocket(const std::string& ip, int port, int timeout = 0);
    ~ClientSocket() {
    }
    void start(std::function<void(const std::string&)> errorFunc, int reconncetTime = 0);
    void send(const std::string& data);

private:
    void resolver();
    void resolverHandle(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type endpoints);

    void syncConnect(boost::asio::ip::tcp::resolver::results_type endpoints);
    void ConnectHandle(std::shared_ptr<Session> session, const boost::system::error_code& error,
                       const boost::asio::ip::tcp::endpoint& endpoint);

    void startTimer();
    void timerHandle();

private:
    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::endpoint remote_;
    boost::asio::ip::tcp::resolver resolver_;
    int timeout_;
    boost::asio::deadline_timer reconnectTimer_;
    std::shared_ptr<Session> session_;
    std::mutex mutex_;
    int reconnectTimeout_;
    std::function<void(const std::string&)> errorCb_;
};