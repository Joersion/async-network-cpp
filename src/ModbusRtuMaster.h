#pragma once

#include "Uart.h"

namespace uart::modbus {

    // class ModbusRTUMaster : public SerialPort {
    // public:
    //     ModbusRTUMaster(std::string portName, int baud, int timeout = 0) : SerialPort(portName, baud, timeout) {};
    //     ~ModbusRTUMaster() {
    //     }

    // public:
    //     // 子类新提供一个虚函数
    //     virtual void onRead(const std::string &ip, int port, int uuid, const std::string &data, const std::string &error) = 0;
    //     // 结束原生父类的虚函数
    //     virtual void onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) override final;

    // public:
    //     // 重写发送接口
    //     void send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen);

    // private:
    //     void packet(const Request &req, std::string &reqData);

    //     void unpacket(const char *buf, int len, std::vector<Response> &resps);

    // private:
    //     std::string data_;
    // };
};  // namespace uart::modbus