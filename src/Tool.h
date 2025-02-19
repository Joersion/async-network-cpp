#pragma once
#include <cstdint>
#include <string>

class Tool {
public:
    // 主机字节序转网络字节序，4字节
    static void htonl2(unsigned long host, char* net);
    // 网络字节序转主机字节序，4字节
    static long ntohl2(const char* net);
    // 主机字节序转网络字节序，2字节
    static void htons2(unsigned short host, char* net);
    // 网络字节序转主机字节序，2字节
    static short ntohs2(const char* net);
    // 大端数据转小端数据
    static void tolittle(std::string& data);
    // modbus crc16
    static uint16_t modbus_crc16(const std::string& data);
    // 转16进制
    static std::string hex2String(const char* bytes, size_t len);
};
