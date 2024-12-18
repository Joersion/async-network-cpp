#include "Modbus.h"

#include "Tool.h"

namespace modbus {
    void Modbus::packet(const RequestBase &req, std::string &reqData) {
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

    void Modbus::unpack(uint8_t code, char *data, ResponseBase &respData) {
        int size = dataSize(code);
        if (size == sizeof(ReadResponse)) {
            ReadResponse *resp = (ReadResponse *)data;
            respData.code = resp->code;
            respData.quantity = resp->quantity;
            std::string data = resp->values;
            if (code == 0x03 || code == 0x04) {
                Tool::tolittle(data);
            }
            respData.values = data;
        } else if (size == sizeof(WriteResponse)) {
            WriteResponse *resp = (WriteResponse *)data;
            respData.code = resp->code;
            respData.startAddr = Tool::ntohs2((char *)&resp->startAddr);
            respData.value = Tool::ntohs2((char *)&resp->value);
        } else if (size == sizeof(ErrorResponse)) {
            ErrorResponse *resp = (ErrorResponse *)data;
            respData.code = resp->code;
            respData.errorCode = resp->errorCode;
        }
    }

    int Modbus::dataSize(uint8_t code) {
        if (code == 0x01 || code == 0x02 || code == 0x03 || code == 0x04) {
            return sizeof(ReadResponse);
        } else if (code == 0x05 || code == 0x06 || code == 0x0f || code == 0x10) {
            return sizeof(WriteResponse);
        } else if (code >= 0x80) {
            return sizeof(ErrorResponse);
        }
        return 0;
    }
};  // namespace modbus