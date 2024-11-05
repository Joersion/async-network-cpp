#include <iostream>
#include <thread>

#include "src/Socket.h"
#include "src/TcpClient.h"

std::string gContent = "hello!";

using namespace net::socket;

class testClient : public TcpClient {
public:
    testClient(const std::string &ip, int port, int timeout = 0) : TcpClient(ip, port, timeout) {
    }
    ~testClient() {
    }

public:
    virtual void onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onRead error:" << error << std::endl;
            return;
        }
        std::cout << "客户端接收数据:" << buf << std::endl;
    }

    virtual void onWrite(const std::string &ip, int port, int len, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onWrite error:" << error << std::endl;
            return;
        }
        std::cout << "客户端发送数据,len:" << len << std::endl;
    }

    virtual void onConnect(const std::string &ip, int port, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onConnect error:" << error << std::endl;
            return;
        }
        std::cout << "已连接上服务器,ip: " << ip << ",port:" << port << std::endl;
    }

    virtual void onClose(const std::string &ip, int port, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onClose error:" << error << std::endl;
            return;
        }
        std::cout << "连接已断开,ip" << ip << ",port:" << port << std::endl;
    }

    virtual void onTimer(const std::string &ip, int port) override {
        std::cout << "客户端发送数据,data:" << gContent << std::endl;
        this->send(gContent);
    }

    virtual void onResolver(const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onResolver error:" << error << std::endl;
            return;
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        gContent = argv[1];
    }
    testClient cli("127.0.0.1", 4137, 2000);
    cli.start(1000);
    while (1) {
        char ch = getchar();
        if (ch == 'q') {
            exit(0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}