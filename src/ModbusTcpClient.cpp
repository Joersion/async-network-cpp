#include "ModbusTcpClient.h"

#include <iostream>

#include "Tool.h"

namespace net::socket::modbus {
    ModbusTcpClient::ModbusTcpClient(const std::string &ip, int port, int timeout) : TcpClient(ip, port, timeout) {
    }

    ModbusTcpClient::~ModbusTcpClient() {
    }

    void ModbusTcpClient::onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) {
        std::string data;
        if (!error.empty()) {
            onRead(ip, port, 0, data, error);
            data_.clear();
            return;
        }
        std::vector<Response> resps;
        unpacket(buf, len, resps);
        for (int i = 0; i < resps.size(); i++) {
            uint16_t uuid = Tool::ntohs2((char *)&resps[i].header.uuid);
            uint8_t code = resps[i].funcCode;
            data = std::string(resps[i].data.data(), resps[i].count);
            if (code == 0x03 || code == 0x04 || code == 0x06 || code == 0x10) {
                std::string convertedData;
                for (size_t i = 0; i < data.size(); i += 2) {
                    if (i + 1 < data.size()) {
                        // 获取当前2个字节
                        uint16_t original = (static_cast<uint8_t>(data[i]) << 8) | static_cast<uint8_t>(data[i + 1]);
                        // 将大端转为小端
                        uint16_t converted = Tool::ntohs2(reinterpret_cast<char *>(&original));
                        convertedData.push_back(static_cast<char>(converted & 0xFF));         // 低字节
                        convertedData.push_back(static_cast<char>((converted >> 8) & 0xFF));  // 高字节
                    }
                }
                data = convertedData;
            }
            onRead(ip, port, uuid, data, error);
        }
    }

    void ModbusTcpClient::send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen) {
        Request request;
        request.header.uuid = uuid;
        request.header.protocolId = 0x0000;
        request.header.len = 0x0006;
        request.header.addrNo = addrNo;
        request.funcCode = funcCode;
        request.startAddr = startAddr;
        request.quantity = reqLen;
        std::string reqData;
        packet(request, reqData);
        TcpClient::send(reqData);
    }

    void ModbusTcpClient::packet(const Request &req, std::string &reqData) {
        char dataLenBuf[2], uuidBuf[2], protocolIdBuf[2], startAddrBuf[2], quantityBuf[2];
        Tool::htons2(req.header.len, dataLenBuf);
        Tool::htons2(req.header.uuid, uuidBuf);
        Tool::htons2(req.header.protocolId, protocolIdBuf);
        Tool::htons2(req.startAddr, startAddrBuf);
        Tool::htons2(req.quantity, quantityBuf);

        reqData.append(uuidBuf, 2);
        reqData.append(protocolIdBuf, 2);
        reqData.append(dataLenBuf, 2);
        reqData.append(1, req.header.addrNo);
        reqData.append(1, req.funcCode);
        reqData.append(startAddrBuf, 2);
        reqData.append(quantityBuf, 2);
    }

    void ModbusTcpClient::unpacket(const char *buf, int len, std::vector<Response> &resps) {
        std::string bufStr(buf, len);
        while (bufStr.size() != 0) {
            if (data_.size() < sizeof(MbapHeader)) {
                int addlen = sizeof(MbapHeader) - data_.size();
                if (bufStr.size() < addlen) {
                    data_.append(bufStr);
                    return;
                }
                data_.append(bufStr.substr(0, addlen));
                bufStr.erase(0, addlen);
            }
            MbapHeader *header = (MbapHeader *)data_.data();
            uint16_t totalLen = Tool::ntohs2((char *)&header->len);
            std::cout << "unpacket.len:" << len << std::endl;
            if (data_.size() < totalLen) {
                int addlen = totalLen - data_.size();
                if (bufStr.size() < addlen) {
                    data_.append(bufStr);
                    return;
                }
                data_.append(bufStr.substr(0, addlen));
                bufStr.erase(0, addlen);
            }
            Response *response = (Response *)data_.data();
            resps.emplace_back(*response);
            data_.clear();
        }
    }
};  // namespace net::socket::modbus