#include <iostream>

#include "src/ServerSocket.h"
#include "src/Session.h"
#include "src/SessionFactory.h"

class testServer : public Session {
public:
    testServer(boost::asio::io_context &ioContext, int timeout) : Session(ioContext, timeout) {
    }
    ~testServer() {
    }

protected:
    virtual void onRead(const char *buf, size_t len) override {
        std::string str(buf, len);
        std::cout << str << std::endl;
        std::string tmp = "OK!";
        tmp += str;
        send(tmp.data(), tmp.length());
    }
    virtual void onWrite(std::string &msg) override {
    }
    virtual void onConnect() override {
        std::cout << "对端已连接,ip: " << ip() << ",port:" << port() << std::endl;
    }
    virtual void onClose(const std::string &error) override {
        std::cout << "对端已断开,ip: " << ip() << ",port:" << port() << std::endl;
    }
    virtual void onTimer() override {
    }
};

class testServerFactory : public SessionFactory {
public:
    testServerFactory() = default;
    ~testServerFactory() = default;
    virtual std::shared_ptr<Session> create(boost::asio::io_context &ioContext, int timeout = 0) {
        return std::make_shared<testServer>(ioContext, timeout);
    }
};

int main() {
    auto t = std::thread([&]() {
        ServerSocket server(4137, std::make_unique<testServerFactory>());
        server.start();
    });
    t.join();
    return 0;
}