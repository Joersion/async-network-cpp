#include <iostream>
#include <thread>

#include "src/GPIO.h"
#include "src/Tool.h"

using namespace gpio;
using namespace std;

class GPIOTest : public GPIO {
public:
    GPIOTest(int timout = 0) : GPIO(timout) {
    }
    ~GPIOTest() = default;

public:
    // 读到数据之后
    virtual void onRead(const std::string &portName, const char *buf, size_t len, const std::string &error) {
        if (!error.empty()) {
            // cout << "onRead,error:" << error << endl;
            return;
        }
        cout << "GPIO收到数据,portName:" << portName << ",data:" << std::string(buf, 1) << ",len:" << len << endl;
    }
    // 数据写入之后
    virtual void onWrite(const std::string &portName, const int len, const std::string &error) {
        if (!error.empty()) {
            cout << "onWrite,error:" << error << endl;
        }
        cout << "GPIO写数据,portName:" << portName << ",len:" << len << endl;
    }
    // 连接上之后
    virtual void onConnect(const std::string &portName, const std::string &error) {
        if (!error.empty()) {
            cout << "onConnect,error:" << error << endl;
        }
        cout << "GPIO打开成功,portName:" << portName << endl;
    }
    // 连接关闭之前
    virtual void onClose(const std::string &portName, const std::string &error) {
        if (!error.empty()) {
            cout << "onClose,error:" << error << endl;
        }
        cout << "GPIO关闭成功,portName:" << portName << endl;
    }
    // 定时器发生之后
    virtual void onTimer(const std::string &portName) {
    }
    // 监听错误发生之后
    void onListenError(const std::string &portName, const std::string &error) {
    }
};

int main(int argc, char *argv[]) {
    std::string name = "/sys/class/gpio-input/DIN3/state";
    GPIOTest port;
    std::string error;
    if (!port.open(error, name)) {
        cout << "GPIO打开失败,name" << name << ",error:" << error << endl;
        return 0;
    }
    cout << "程序开始,GPIO地址:" << name << endl;

    while (1) {
        char ch = getchar();
        if (ch == 'q') {
            exit(0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}