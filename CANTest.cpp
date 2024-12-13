#include <iostream>
#include <thread>

#include "src/SocketCAN.h"

using namespace can;
using namespace std;

class CANTest : public CANTransceiver {
public:
    CANTest(ProtocolType type, int timout = 0) : CANTransceiver(type, timout) {
    }
    ~CANTest() = default;

public:
    // 读到数据之后
    virtual void onRead(const std::string &canName, unsigned int canId, const char *data, int len, const std::string &error) {
        if (!error.empty()) {
            cout << "onRead,error:" << error << endl;
            return;
        }
        cout << "Received CAN Frame:" << canName << endl;
        cout << "ID:0x " << std::hex << canId << endl;
        cout << "Data: ";
        for (int i = 0; i < len; ++i) {
            printf("%02X ", data[i]);
        }
        cout << endl;
    }
    // 数据写入之后
    virtual void onWrite(const std::string &canName, const int len, const std::string &error) {
        if (!error.empty()) {
            cout << "onWrite,error:" << error << endl;
            return;
        }
        cout << "CAN onWrite,canName:" << canName << ",len:" << len << endl;
    }
    // 连接上之后
    virtual void onConnect(const std::string &canName, const std::string &error) {
        if (!error.empty()) {
            cout << "onConnect,error:" << error << endl;
        }
        cout << "CAN Connect successful,canName:" << canName << endl;
    }
    // 连接关闭之前
    virtual void onClose(const std::string &canName, const std::string &error) {
        if (!error.empty()) {
            cout << "onClose,error:" << error << endl;
            return;
        }
        cout << "CAN close successful,canName:" << canName << endl;
    }
    // 定时器发生之后
    virtual void onTimer(const std::string &canName) {
        cout << "onTimer,canName:" << canName << endl;
        int canId = 0x123;
        char data[] = {0x01, 0x02, 0x03, 0x04};
        send(std::string(data, sizeof(data)), canId);
    }
};

int main(int argc, char *argv[]) {
    std::string name = "can";
    if (argc > 1 && !std::string(argv[1]).empty()) {
        name += argv[1];
    } else {
        name += "0";
    }
    CANTest can(ProtocolType::CAN2B, 2000);
    std::string error;
    if (!can.open(error, name)) {
        cout << "CAN open error:" << error << ",name:" << name << endl;
        return 0;
    }
    cout << "begin!canName:" << name << endl;

    while (1) {
        char ch = getchar();
        if (ch == 'q') {
            exit(0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}