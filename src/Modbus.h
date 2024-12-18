#pragma once
#include <stdint.h>

#include <string>

#include "Tool.h"

namespace modbus {
    // 读操作:0x01 0x02 0x03 0x04
    struct ReadRequest {
        uint16_t startAddr;
        uint16_t quantity;
    };
    struct ReadRespone {};

    // 写单个:0x05 0x06
    struct WriteSingleRequest {
        uint16_t startAddr;
        uint16_t value;
    };
    struct WriteSingleRespone {};

    // 写多个:0x0f 0x10
    struct WriteMutiRequest {
        uint16_t wirteAddr;
        uint16_t quantity;
        std::string values;
    };
    struct WriteMutiRespone {};

    // 错误响应 0x81-0xFF
    struct ErrorRespone {};

    class Modbus {
    public:
        static void packet(uint16_t startAddr, uint16_t value, std::string &reqData) {
            char startAddrBuf[2], quantityBuf[2];
            Tool::htons2(startAddr, startAddrBuf);
            Tool::htons2(value, quantityBuf);
            reqData.append(startAddrBuf, 2);
            reqData.append(quantityBuf, 2);
        }

        static void packet(uint16_t wirteAddr, uint16_t quantity, const std::string &values, std::string &reqData) {
            char startAddrBuf[2], quantityBuf[2];
            Tool::htons2(wirteAddr, startAddrBuf);
            Tool::htons2(quantity, quantityBuf);
            reqData.append(startAddrBuf, 2);
            reqData.append(quantityBuf, 2);
            reqData.append(reqData);
        }
    };
};  // namespace modbus