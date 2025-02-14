#include <iostream>
#include <thread>

#include "src/Tool.h"
#include "src/Uart.h"

using namespace uart;
using namespace std;

class SerialPortTest : public SerialPort {
public:
    SerialPortTest(int timout = 0) : SerialPort(timout) {
    }
    ~SerialPortTest() = default;

public:
    // 读到数据之后
    virtual void onRead(const std::string &portName, const char *buf, size_t len, const std::string &error) {
        if (!error.empty()) {
            cout << "onRead,error:" << error << endl;
        }
        cout << "串口收到数据,portName:" << portName << ",data:" << Tool::hex2String(buf, len) << ",len:" << len << endl;
    }
    // 数据写入之后
    virtual void onWrite(const std::string &portName, const int len, const std::string &error) {
        if (!error.empty()) {
            cout << "onWrite,error:" << error << endl;
        }
        cout << "串口写数据,portName:" << portName << ",len:" << len << endl;
    }
    // 连接上之后
    virtual void onConnect(const std::string &portName, const std::string &error) {
        if (!error.empty()) {
            cout << "onConnect,error:" << error << endl;
        }
        cout << "串口打开成功,portName:" << portName << endl;
    }
    // 连接关闭之前
    virtual void onClose(const std::string &portName, const std::string &error) {
        if (!error.empty()) {
            cout << "onClose,error:" << error << endl;
        }
        cout << "串口关闭成功,portName:" << portName << endl;
    }
    // 定时器发生之后
    virtual void onTimer(const std::string &portName) {
        cout << "定时器触发,portName:" << portName << endl;
        send("hello...");
    }
};

int main(int argc, char *argv[]) {
    std::string name = "/dev/ttyS";
    if (argc > 1 && !std::string(argv[1]).empty()) {
        name += argv[1];
    } else {
        name += "9";
    }

    SerialPortTest port(2000);
    std::string error;
    uart::Config config;
    config.baudRate = 9600;
    if (!port.open(error, name, config)) {
        cout << "串口打开失败" << endl;
        return 0;
    }
    cout << "程序开始,串口名:" << name << endl;

    while (1) {
        char ch = getchar();
        if (ch == 'q') {
            exit(0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}