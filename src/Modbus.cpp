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
        int opt = action(code);
        if (opt == 0) {
            int len = *(data + 1);
            respData.values = std::string(data + 2, len);
        } else if (opt == 1) {
            respData.value = Tool::ntohs2(data + 1);
            respData.values = std::string(data + 1, 4);
        } else if (opt == 2) {
            respData.errorCode = *(data + 1);
        }
        // int size = dataSize(code);
        // if (size == sizeof(WriteResponse)) {
        //     WriteResponse *resp = (WriteResponse *)data;
        //     respData.code = resp->code;
        //     respData.startAddr = Tool::ntohs2((char *)&resp->startAddr);
        //     respData.value = Tool::ntohs2((char *)&resp->value);
        //     respData.values = std::string((char *)&resp->value, 2);
        // } else if (size == sizeof(ErrorResponse)) {
        //     ErrorResponse *resp = (ErrorResponse *)data;
        //     respData.code = resp->code;
        //     respData.errorCode = resp->errorCode;
        // } else {
        //     ReadResponse *resp = (ReadResponse *)data;
        //     respData.code = resp->code;
        //     respData.quantity = resp->quantity;
        //     std::string data = std::string((char *)resp->values, resp->quantity);
        //     respData.values = data;
        // }
    }

    int Modbus::dataSize(uint8_t code) {
        if (code == 0x01 || code == 0x02 || code == 0x03 || code == 0x04) {
            return sizeof(ReadResponse) / 8;
        } else if (code == 0x05 || code == 0x06 || code == 0x0f || code == 0x10) {
            return sizeof(WriteResponse) / 8;
        } else if (code >= 0x80) {
            return sizeof(ErrorResponse) / 8;
        }
        return 0;
    }

    int Modbus::action(uint8_t code) {
        // 0：读 1:写 2：错误 -1:抛弃
        if (code == 0x01 || code == 0x02 || code == 0x03 || code == 0x04) {
            return 0;
        } else if (code == 0x05 || code == 0x06 || code == 0x0f || code == 0x10) {
            return 1;
        } else if (code >= 0x80) {
            return 2;
        } else {
            return -1;
        }
    }
};  // namespace modbus