#include <iostream>

#include "src/ServerSocket.h"
#include "src/Session.h"
#include "src/SessionFactory.h"

class testSession : public Session {
public:
    testSession(boost::asio::io_context &ioContext, int timeout) : Session(ioContext, timeout) {
    }
    ~testSession() {
    }

protected:
    virtual void onRead(const char *buf, size_t len) override {
        std::cout << buf << std::endl;
        send(buf, len);
    }
    virtual void onWrite() override {
    }
    virtual void onConnect() override {
        std::cout << "对端已连接,ip: " << ip() << ",port:" << port() << std::endl;
    }
    virtual void onClose(const std::string &error) override {
        std::cout << "对端已连接,ip: " << ip() << ",port:" << port() << std::endl;
    }
    virtual void onTimer() override {
    }
};

class testSessionFactory : public SessionFactory {
public:
    testSessionFactory() = default;
    ~testSessionFactory() = default;
    virtual std::shared_ptr<Session> create(boost::asio::io_context &ioContext, int timeout = 0) {
        return std::make_shared<testSession>(ioContext, timeout);
    }
};

int main() {
    ServerSocket server(4137, std::make_unique<testSessionFactory>());
    server.start();
    while (1) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    return 0;
}