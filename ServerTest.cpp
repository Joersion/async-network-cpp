#include <iostream>

#include "src/ServerConnection.h"
#include "src/Session.h"

class testServer : public ServerConnection {
public:
    testServer(int port, int timeout = 0) : ServerConnection(port, timeout) {
    }
    ~testServer() {
    }

public:
    virtual void onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onRead error info:" << error << std::endl;
            return;
        }
        std::cout << "收到来自客户端,Ip:" << ip << ",port:" << port << ",data:" << buf << std::endl;
        std::string str(buf, len);
        std::string tmp = "OK!";
        tmp += str;
        std::cout << "发送数据给客户端,Ip:" << ip << ",port:" << port << ",data:" << tmp << std::endl;
        send(ip, tmp);
    }

    virtual void onWrite(const std::string &ip, int port, int len, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onWrite error info:" << error << std::endl;
            return;
        }
        std::cout << "发送数据长度,Ip:" << ip << ",port:" << port << ",len:" << len << std::endl;
    }

    virtual void onConnect(const std::string &ip, int port, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onConnect error info:" << error << std::endl;
            return;
        }
        std::cout << "新客户端连接,ip: " << ip << ",port:" << port << std::endl;
    }

    virtual void onClose(const std::string &ip, int port, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onClose error info:" << error << std::endl;
            return;
        }
        std::cout << "连接已关闭,ip: " << ip << ",port:" << port << std::endl;
    }

    virtual void onTimer(const std::string &ip, int port) override {
    }
};

int main() {
    testServer server(4137);
    server.start();
    getchar();
    return 0;
}