#include <iostream>

#include "src/ClientSocket.h"
#include "src/Session.h"

std::string gContent = "hello!";

class testClient : public ClientSocket {
public:
    testClient(const std::string &ip, int port, int timeout = 0) : ClientSocket(ip, port, timeout) {
    }
    ~testClient() {
    }

public:
    virtual void onRead(const std::string &ip, int port, const char *buf, size_t len) override {
        std::cout << buf << std::endl;
    }

    virtual void onWrite(const std::string &ip, int port, std::string &msg) override {
    }

    virtual void onConnect(const std::string &ip, int port) override {
        std::cout << "对端已连接,ip: " << ip << ",port:" << port << std::endl;
    }

    virtual void onClose(const std::string &ip, int port, const std::string &error) override {
        std::cout << "error info:" << error << std::endl;
    }

    virtual void onTimer(const std::string &ip, int port) override {
        this->send(gContent);
    }
};

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        gContent = argv[1];
    }
    auto t = std::thread([&]() {
        testClient cli("127.0.0.1", 4137, 5000);
        cli.start([](const std::string &error) { std::cout << error << std::endl; }, 5000);
    });
    t.join();
    return 0;
}