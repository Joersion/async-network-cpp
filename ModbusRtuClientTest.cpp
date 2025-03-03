#include <iostream>
#include <thread>

#include "../src/ModbusRTUClientBase.h"
#include "../src/Tool.h"

using namespace modbus::rtu;

class testClient : public ModbusRTUClientBase {
public:
    testClient(int timeout = 0) : ModbusRTUClientBase(timeout) {
    }
    ~testClient() {
    }

public:
    virtual void onRead(const std::string &src, const std::string &portName, int uuid, const std::string &data, uint8_t errorcode,
                        const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onRead error:" << error << std::endl;
            return;
        }
    }

    virtual void onWrite(const std::string &portName, int len, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onWrite error:" << error << std::endl;
            return;
        }
        std::cout << "客户端发送数据,len:" << len << std::endl;
    }

    virtual void onConnect(const std::string &portName, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onConnect error:" << error << std::endl;
            return;
        }
        std::cout << "已连接上服务器,portName: " << portName << std::endl;
    }

    virtual void onClose(const std::string &portName, const std::string &error) override {
        if (!error.empty()) {
            std::cout << "onClose error:" << error << std::endl;
            return;
        }
        std::cout << "连接已断开,portName:" << portName << std::endl;
    }

    virtual void onTimer(const std::string &portName) override {
        std::cout << "onTimer,portName:" << portName << std::endl;
        unsigned char slaveAddr = 0x01;     // 从站地址
        unsigned char functionCode = 0x03;  // 功能码 (读取保持寄存器)
        unsigned short startAddr = 0x0000;  // 起始地址
        unsigned short numRegs = 0x0002;    // 读取2个寄存器
        send(slaveAddr, functionCode, startAddr, numRegs);
    }
};

int main(int argc, char *argv[]) {
    testClient cli(10000);
    std::string name = "/dev/ttyS";
    if (argc > 1 && !std::string(argv[1]).empty()) {
        name += argv[1];
    } else {
        name += "9";
    }
    std::string error;
    uart::Config config;
    config.baudRate = 9600;
    // 半双工485需要设置发送间隔
    cli.setSendInterval(10);
    if (!cli.open(error, name, config)) {
        std::cout << "串口打开失败" << std::endl;
        return 0;
    }
    std::cout << "程序开始,串口名:" << name << std::endl;

    while (1) {
        char ch = getchar();
        if (ch == 'q') {
            exit(0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}