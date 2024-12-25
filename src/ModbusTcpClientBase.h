#pragma once

#include "Modbus.h"
#include "TcpClient.h"

namespace modbus::tcp {
    struct MbapHeader {
        // 事务ID
        uint16_t uuid;
        // 协议ID为0表示Modbus TCP
        uint16_t protocolId;
        // 请求：后面数据的长度；响应：总长度
        uint16_t len;
        // 从机地址
        uint8_t addrNo;
    };

    struct Request {
        MbapHeader header;
        ResponseBase base;
    };

    struct Response {
        MbapHeader header;
        ResponseBase base;
    };

    class ModbusTcpClientBase : public net::socket::TcpClient {
    public:
        ModbusTcpClientBase(const std::string &ip, int port, int timeout = 0);
        ~ModbusTcpClientBase();

    public:
        // 子类新提供一个虚函数
        virtual void onRead(const std::string &ip, int port, int uuid, const std::string &data, const std::string &error) = 0;
        // 结束原生父类的虚函数
        virtual void onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) override final;

    public:
        bool send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::string &data = "");
        bool send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::vector<uint16_t> &data);

    private:
        void packet(const MbapHeader &header, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen, const std::string &data, std::string &reqData);

        void unpacket(const char *buf, int len, std::vector<Response> &resps);

    private:
        std::string data_;
    };
};  // namespace modbus