#include "Tool.h"

#include <stdint.h>

void Tool::htonl2(unsigned long host, char* net) {
    for (int i = 0; i < 4; i++) {
        net[i] = (host >> (8 * (3 - i))) & 0xff;
    }
}

long Tool::ntohl2(const char* net) {
    long host = 0;
    for (int i = 0; i < 4; i++) {
        host += ((unsigned char)net[i]) << (8 * (3 - i));
    }
    return host;
}

void Tool::htons2(unsigned short host, char* net) {
    for (int i = 0; i < 2; i++) {
        net[i] = (host >> (8 * (1 - i))) & 0xff;
    }
}

short Tool::ntohs2(const char* net) {
    short host = 0;
    for (int i = 0; i < 2; i++) {
        host += ((unsigned char)net[i]) << (8 * (1 - i));
    }
    return host;
}

void Tool::tolittle(std::string& data) {
    std::string convertedData;
    for (size_t i = 0; i < data.size(); i += 2) {
        if (i + 1 < data.size()) {
            // 获取当前2个字节
            uint16_t original = (static_cast<uint8_t>(data[i]) << 8) | static_cast<uint8_t>(data[i + 1]);
            // 将大端转为小端
            uint16_t converted = Tool::ntohs2(reinterpret_cast<char*>(&original));
            convertedData.push_back(static_cast<char>(converted & 0xFF));         // 低字节
            convertedData.push_back(static_cast<char>((converted >> 8) & 0xFF));  // 高字节
        }
    }
    data = convertedData;
}

uint16_t Tool::modbus_crc16(const std::string& data) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < data.size(); ++i) {
        crc ^= static_cast<uint8_t>(data[i]);
        for (size_t j = 8; j; --j) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}