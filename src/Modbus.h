#pragma once

#include <stdint.h>

#include <string>

namespace modbus {
    // 请求
    struct RequestBase {
        // 功能码
        uint8_t code;
        // 起始地址
        uint16_t startAddr;
        // 读：数量 ; 写单个：值
        uint16_t quantity;
        // 写多个:长度和数据
        std::string values;
    };
    // 响应
    struct ResponseBase {
        // 功能码
        uint8_t code;
        // 起始地址
        uint16_t startAddr;
        // 写单个：值 ; 写多个：数量
        uint16_t value;
        // 读：数量
        uint8_t quantity;
        // 错误码
        uint8_t errorCode;
        // 数据
        std::string values;
    };

    class Modbus {
        // 读响应
        struct ReadResponse {
            uint8_t code;
            uint8_t quantity;
            std::string values;
        };

        // 写响应
        struct WriteResponse {
            uint8_t code;
            uint16_t startAddr;
            uint16_t value;
        };

        // 错误响应 0x81-0xFF
        struct ErrorResponse {
            uint8_t code;
            uint8_t errorCode;
        };

    public:
        static void packet(const RequestBase &req, std::string &reqData);

        static void unpack(uint8_t code, char *data, ResponseBase &respData);

        static int dataSize(uint8_t code);
    };
};  // namespace modbus