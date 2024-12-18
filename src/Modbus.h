#pragma once
#include <stdint.h>

#include <string>

#include "Tool.h"

namespace modbus {
    // 请求
    struct RequestBase {
        uint8_t code;
        uint16_t startAddr;
        uint16_t quantity;
        std::string values;
    };
    // 响应
    struct ResponseBase {
        uint8_t code;
        uint16_t startAddr;
        uint16_t value;
        uint16_t quantity;
        uint8_t errorCode;
        std::string values;
    };

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

    class Modbus {
    public:
        static void packet(const RequestBase &req, std::string &reqData) {
            char startAddrBuf[2], quantityBuf[2];
            Tool::htons2(req.startAddr, startAddrBuf);
            Tool::htons2(req.quantity, quantityBuf);
            reqData.append(startAddrBuf, 2);
            reqData.append(quantityBuf, 2);
            if (req.code == 0x0f || req.code == 0x10) {
                reqData.append(1, req.values.length());
                reqData.append(req.values);
            }
        }

        static void unpack(uint8_t code, char *data, ResponseBase &respData) {
            if (code == 0x01 || code == 0x02 || code == 0x03 || code == 0x04) {
                ReadResponse *resp = (ReadResponse *)data;
                respData.code = resp->code;
                respData.quantity = resp->quantity;
                std::string data = resp->values;
                if (code == 0x03 || code == 0x04) {
                    Tool::tolittle(data);
                }
                respData.values = data;
            } else if (code == 0x05 || code == 0x06 || code == 0x0f || code == 0x10) {
                WriteResponse *resp = (WriteResponse *)data;
                respData.code = resp->code;
                respData.startAddr = Tool::ntohs2((char *)&resp->startAddr);
                respData.value = Tool::ntohs2((char *)&resp->value);
            } else if (code >= 0x80) {
                ErrorResponse *resp = (ErrorResponse *)data;
                respData.code = resp->code;
                respData.errorCode = resp->errorCode;
            }
        }
    };
};  // namespace modbus