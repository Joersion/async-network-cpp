#include "ModbusTcpClient.h"

#include <iostream>

#include "Modbus.h"
#include "Tool.h"

namespace modbus {
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
                Tool::tolittle(data);
            }
            onRead(ip, port, uuid, data, error);
        }
    }

    bool ModbusTcpClient::send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::string &data) {
        if (data.length() > 250) {
            return false;
        }
        MbapHeader header;
        header.uuid = uuid;
        header.protocolId = 0x0000;
        header.len = 0x0006 + data.length();
        header.addrNo = addrNo;

        std::string reqData;
        packet(header, funcCode, startAddr, value, data, reqData);
        TcpClient::send(reqData);
        return true;
    }

    bool ModbusTcpClient::send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t value,
                               const std::vector<uint16_t> &data) {
        std::string str;
        char buf[2];
        for (int i = 0; i < data.size(); i++) {
            Tool::htons2(data[i], buf);
            str.append(buf, 2);
        }
        Tool::tolittle(str);
        return send(uuid, addrNo, funcCode, startAddr, value, str);
    }

    void ModbusTcpClient::packet(const MbapHeader &header, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen, const std::string &data,
                                 std::string &reqData) {
        char dataLenBuf[2], uuidBuf[2], protocolIdBuf[2];
        Tool::htons2(header.len, dataLenBuf);
        Tool::htons2(header.uuid, uuidBuf);
        Tool::htons2(header.protocolId, protocolIdBuf);

        reqData.append(uuidBuf, 2);
        reqData.append(protocolIdBuf, 2);
        reqData.append(dataLenBuf, 2);
        reqData.append(1, header.addrNo);
        reqData.append(1, funcCode);
        if (funcCode == 0x0f || funcCode == 0x10) {
            Modbus::packet(startAddr, reqLen, data, reqData);
        } else {
            Modbus::packet(startAddr, reqLen, reqData);
        }
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
};  // namespace modbus