# 基于c++ 和 boost.asio/boost.beast的跨平台异步网络库

## 目录

- [说明](#说明)
- [安装](#安装)
- [简单示例](#简单示例)
- [关于](#关于)

## 说明

这是一个基于boost.asio/boost.beast库，采用c++17开发的，支持跨平台的异步网络库，具体实现功能：

* tcp 异步客户端/服务端：基于boost.asio，抽象了读/写/连接/关闭/定时器功能，服务端抽象tcp连接管理，客户端抽象断线重连。用户层需自行封包、解包以及数据处理
* http 异步客户端/服务端：基于boost.beast，抽象了http应用层，用户仅需关心消息递达时的数据处理，和错误处理

构建工具：
* 建议采用 cmake 3.10 以上，本人采用 3.28.3

编译器：
* 建议采用 GCC 7.1 以上，本人采用 13.2

boost库：
* 本人采用 1.81.0，其他版本需支持boost::asio::io_context，早期该类被替换成boost::asio::io_server

boost.beast库：
* 本人采用 1.75.0

openSSL库（boost.beast依赖）：
* 本人采用 1.1.1

## 安装
boost:

* 本地编译(本人采用静态库)：
```boost：
wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
tar -xzf boost_1_81_0.tar.gz
cd boost_1_81_0
./bootstrap.sh --with-toolset=gcc
./b2 link=static cxxflags="-fPIC" install --prefix=../x86
```

* 交叉编译(以aarch64为例)：
```boost：
wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
tar -xzf boost_1_81_0.tar.gz
cd boost_1_81_0
./bootstrap.sh --with-toolset=gcc
vim project-config.jam
if ! gcc in [ feature.values <toolset> ]

{

   using gcc : arm : /home/joersion/tool/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-rockchip1031-linux-gnu-gcc ;

}
./b2 link=static cxxflags="-fPIC" install --prefix=../aarch64
```
## 简单示例
* tcp client:
```tcp client
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
    testClient cli("127.0.0.1", 4137, 100);
    cli.start(1000);
    getchar();
    return 0;
}
```
* tcp server:
```tcp server
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
```
* http client:
```
#include <iostream>

#include "src/HttpClient.h"

int main() {
    HttpClient::GET({"www.example.com", 80, "/", "1.0", 30}, [](const std::string& err, const HttpClient::Response& resp) {
        if (err.empty()) {
            std::cout << "resp.code:" << resp.code << std::endl;
            std::cout << "resp.massage:" << resp.massage << std::endl;
            std::cout << "resp.type:" << resp.type << std::endl;
            std::cout << "resp.version:" << resp.version << std::endl;
            std::cout << "resp.body:" << resp.body << std::endl;
        }
    });
    HttpClient::POST({"jsonplaceholder.typicode.com", 80, "/posts", "1.0", 30}, {R"({"title": "foo", "body": "bar", "userId": 1})", ""},
                     [](const std::string& err, const HttpClient::Response& resp) {
                         if (err.empty()) {
                             std::cout << LINES << "POST" << LINES << std::endl;
                             std::cout << "resp.code:" << resp.code << std::endl;
                             std::cout << "resp.massage:" << resp.massage << std::endl;
                             std::cout << "resp.type:" << resp.type << std::endl;
                             std::cout << "resp.version:" << resp.version << std::endl;
                             std::cout << "resp.body:" << resp.body << std::endl;
                         }
                     });
}
```

## 关于

目前cmake构建仅支持linux，后续可能会cmake新增windows兼容


**************************
- 👋 I’m Joersion (WuJiaXiang)
- 👀 I’m interested in code
- 🌱 learning C++ and python and golang
- 📫 e-mail : 1539694343@qq.com

**************************



