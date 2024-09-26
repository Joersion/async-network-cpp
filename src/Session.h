#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <queue>
#define BUFFER_MAX_LEN 2048

class Connection {
public:
    Connection() {
    }
    virtual ~Connection() {
    }

public:
    virtual void onRead(const std::string &ip, int port, const char *buf, size_t len) = 0;
    virtual void onWrite(const std::string &ip, int port, std::string &msg) = 0;
    virtual void onConnect(const std::string &ip, int port) = 0;
    virtual void onClose(const std::string &ip, int port, const std::string &error) = 0;
    virtual void onTimer(const std::string &ip, int port) = 0;
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(Connection *conn, boost::asio::io_context &ioContext, int timeout = 0);
    ~Session() {
    }

public:
    std::shared_ptr<boost::asio::ip::tcp::socket> getSocket();
    std::string ip();
    int port();
    void start();
    void send(const char *msg, size_t len);

    void close(const std::string &error);
    void installCloseCb(std::function<void(const std::string &)> func) {
        closeCb_ = func;
    }

private:
    void syncRecv();
    void readHandle(const boost::system::error_code &error, size_t len);

    void syncSend(const std::string &msg);
    void writeHandle(const boost::system::error_code &error, size_t len);

    void startTimer();
    void timerHandle();

private:
    Connection *conn_;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
    char recvBuf_[BUFFER_MAX_LEN];
    std::queue<std::string> sendBuf_;
    std::mutex sendLock_;
    bool isClose_;
    std::function<void(const std::string &)> closeCb_;
    int timeout_;
    boost::asio::deadline_timer timer_;
};