#include <iostream>
#include <thread>

#include "../src/ModbusTcpClientBase.h"
#include "src/Tool.h"

using namespace modbus::tcp;

class testClient : public ModbusTcpClientBase {
public:
    testClient(const std::string &ip, int port, int timeout = 0) : ModbusTcpClientBase(ip, port, timeout) {
    }
    ~testClient() {
    }

public:
    virtual void onRead(const std::string &ip, int port, int uuid, const std::string &data, uint8_t errorcode, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onRead error:" << error << std::endl;
            return;
        }
        std::cout << "收到数据:" << Tool::hex2String(data.data(), data.length()) << std::endl;
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
        std::cout << "onTimer,ip" << ip << ",port:" << port << std::endl;
        unsigned char uuid = 0x01;  // 事务ID
        // unsigned char addrNo = 0x01;        // 从站地址
        unsigned char functionCode = 0x03;  // 功能码 (读取保持寄存器)
        unsigned short startAddr = 0x0000;  // 起始地址
        unsigned short numRegs = 0x0002;    // 读取2个寄存器
        send(uuid, 1, functionCode, startAddr, numRegs);
    }

    virtual void onResolver(const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onResolver error:" << error << std::endl;
            return;
        }
    }
};

int main(int argc, char *argv[]) {
    testClient cli("127.0.0.1", 4137, 100);
    cli.start(500);
    while (1) {
        char ch = getchar();
        if (ch == 'q') {
            exit(0);
        } else if (ch == '1') {
            cli.close();
        } else if (ch == '2') {
            cli.start(500);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}