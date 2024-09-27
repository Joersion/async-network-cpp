#pragma once

#include "Session.h"

class ClientConnection : public Connection {
public:
    ClientConnection(const std::string& ip, int port, int timeout = 0);
    ~ClientConnection() {
    }
    void start(int reconncetTime = 0);
    void send(const std::string& data);

public:
    virtual void onResolver(const std::string& error) = 0;
    // 重写关闭方法，防止子类继续重写
    virtual void doClose(const std::string& ip, int port, const std::string& error) override final;

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
};