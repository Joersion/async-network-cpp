#pragma once

#include "Modbus.h"
#include "Uart.h"

namespace modbus::rtu {
    struct Request {
        uint8_t uuid;
        RequestBase base;
        uint16_t crc;
    };

    struct Response {
        uint8_t uuid;
        ResponseBase base;
        uint16_t crc;
        std::string src;
    };

    class ModbusRTUClientBase : public uart::SerialPort {
    public:
        ModbusRTUClientBase(int timeout = 0);
        ~ModbusRTUClientBase();

    public:
        // 子类新提供一个虚函数
        virtual void onRead(const std::string &src, const std::string &portName, int uuid, const std::string &data, uint8_t errorResp,
                            const std::string &error) = 0;
        // 结束原生父类的虚函数
        virtual void onRead(const std::string &portName, const char *buf, size_t len, const std::string &error) override final;

    public:
        bool send(uint8_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::string &data = "");
        bool send(uint8_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::vector<uint16_t> &data);

    private:
        void packet(uint8_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen, const std::string &data, std::string &reqData);
        void unpacket(const char *buf, int len, std::vector<Response> &resps);

    private:
        std::string currentbuf_;
        std::string readBuf_;
    };
};  // namespace modbus::rtu