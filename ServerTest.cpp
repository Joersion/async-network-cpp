#include <iostream>

#include "src/ServerSocket.h"
#include "src/Session.h"

class testServer : public ServerSocket {
public:
    testServer(int port, int timeout = 0) : ServerSocket(port, timeout) {
    }
    ~testServer() {
    }

public:
    virtual void onRead(const std::string &ip, int port, const char *buf, size_t len,
                        const std::string &error) override {
        if (!error.empty()) {
            std::cout << "error info:" << error << std::endl;
            return;
        }
        std::string str(buf, len);
        std::cout << str << std::endl;
        std::string tmp = "OK!";
        tmp += str;
        send(ip, tmp);
    }

    virtual void onWrite(const std::string &ip, int port, const std::string &msg, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "error info:" << error << std::endl;
            return;
        }
    }

    virtual void onConnect(const std::string &ip, int port, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "error info:" << error << std::endl;
            return;
        }
        std::cout << "对端已连接,ip: " << ip << ",port:" << port << std::endl;
    }

    virtual void onClose(const std::string &ip, int port, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "error info:" << error << std::endl;
            return;
        }
        std::cout << "对端已断开,ip: " << ip << ",port:" << port << std::endl;
    }

    virtual void onTimer(const std::string &ip, int port) override {
    }
};

int main() {
    auto t = std::thread([&]() {
        testServer server(4137);
        server.start();
    });
    t.join();
    return 0;
}