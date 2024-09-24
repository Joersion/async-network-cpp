#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <queue>
#define BUFFER_MAX_LEN 2048

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_context &ioContext, int timeout = 0);
    ~Session() {
    }

public:
    boost::asio::ip::tcp::socket &socket() {
        return socket_;
    }

    std::string ip() {
        return socket_.remote_endpoint().address().to_string();
    }

    int port() {
        return socket_.remote_endpoint().port();
    }

    void start();
    void start(boost::asio::ip::tcp::resolver::results_type endpoints);
    void send(const char *msg, size_t len);

    void close(const std::string &error);
    void installCloseCb(std::function<void(const std::string &)> func) {
        closeCb_ = func;
    }

protected:
    virtual void onRead(const char *buf, size_t len) = 0;
    virtual void onWrite(std::string &msg) = 0;
    virtual void onConnect() = 0;
    virtual void onClose(const std::string &error) = 0;
    virtual void onTimer() = 0;

private:
    void syncRecv();
    void readHandle(const boost::system::error_code &error, size_t len);

    void syncSend(const std::string &msg);
    void writeHandle(const boost::system::error_code &error, size_t len);

    void syncConnect(boost::asio::ip::tcp::resolver::results_type endpoints);
    void ConnectHandle(const boost::system::error_code &error, const boost::asio::ip::tcp::endpoint &endpoint);

    void startTimer();
    void timerHandle();

private:
    boost::asio::ip::tcp::socket socket_;
    char recvBuf_[BUFFER_MAX_LEN];
    std::queue<std::string> sendBuf_;
    std::mutex sendLock_;
    bool isClose_;
    std::function<void(const std::string &)> closeCb_;
    int timeout_;
    boost::asio::deadline_timer timer_;
};