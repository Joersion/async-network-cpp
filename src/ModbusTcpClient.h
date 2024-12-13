#pragma once

#include "TcpClient.h"

namespace net::socket::modbus {
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
        // 功能码
        uint8_t funcCode;
        // 起始地址
        uint16_t startAddr;
        // 读取数量
        uint16_t quantity;
    };

    struct Response {
        MbapHeader header;
        // 功能码
        uint8_t funcCode;
        // 响应字节数
        uint8_t count;
        // 响应数据
        std::string data;
    };

    class ModbusTcpClient : public TcpClient {
    public:
        ModbusTcpClient(const std::string &ip, int port, int timeout = 0);
        ~ModbusTcpClient();

    public:
        // 子类新提供一个虚函数
        virtual void onRead(const std::string &ip, int port, int uuid, const std::string &data, const std::string &error) = 0;
        // 结束原生父类的虚函数
        virtual void onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) override final;

    public:
        // 重写发送接口
        void send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen);

    private:
        void packet(const Request &req, std::string &reqData);

        void unpacket(const char *buf, int len, std::vector<Response> &resps);

    private:
        std::string data_;
    };
};  // namespace net::socket