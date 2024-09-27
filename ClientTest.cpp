#include <iostream>

#include "src/ClientConnection.h"
#include "src/Session.h"

std::string gContent = "hello!";

class testClient : public ClientConnection {
public:
    testClient(const std::string &ip, int port, int timeout = 0) : ClientConnection(ip, port, timeout) {
    }
    ~testClient() {
    }

public:
    virtual void onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "error info:" << error << std::endl;
            return;
        }
        std::cout << buf << std::endl;
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
        std::cout << "error info:" << error << std::endl;
    }

    virtual void onTimer(const std::string &ip, int port) override {
        this->send(gContent);
    }

    virtual void onResolver(const std::string &error) override {
        if (!error.empty()) {
            std::cout << "error info:" << error << std::endl;
            return;
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        gContent = argv[1];
    }
    auto t = std::thread([&]() {
        testClient cli("127.0.0.1", 4137, 5000);
        cli.start(5000);
    });
    t.join();
    return 0;
}