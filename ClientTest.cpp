#include <iostream>

#include "src/ClientSocket.h"
#include "src/Session.h"
#include "src/SessionFactory.h"

std::string gContent = "hello!";

class testClient : public Session {
public:
    testClient(boost::asio::io_context &ioContext, int timeout) : Session(ioContext, timeout) {
    }
    ~testClient() {
    }

protected:
    virtual void onRead(const char *buf, size_t len) override {
        std::cout << buf << std::endl;
    }
    virtual void onWrite() override {
    }
    virtual void onConnect() override {
        std::cout << "对端已连接,ip: " << ip() << ",port:" << port() << std::endl;
    }
    virtual void onClose(const std::string &error) override {
        std::cout << "对端已断开,ip: " << ip() << ",port:" << port() << std::endl;
    }
    virtual void onTimer() override {
        send(gContent.data(), gContent.length());
    }
};

class testClientFactory : public SessionFactory {
public:
    testClientFactory() = default;
    ~testClientFactory() = default;
    virtual std::shared_ptr<Session> create(boost::asio::io_context &ioContext, int timeout = 0) {
        return std::make_shared<testClient>(ioContext, timeout);
    }
};

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        gContent = argv[1];
    }
    auto t = std::thread([&]() {
        ClientSocket client("127.0.0.1", 4137, std::make_unique<testClientFactory>(), 5);
        client.start();
    });
    t.join();
    return 0;
}