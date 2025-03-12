#include "ModbusTcpClientBase.h"

#include <iostream>

#include "Tool.h"

namespace modbus::tcp {
    ModbusTcpClientBase::ModbusTcpClientBase(const std::string &ip, int port, int timeout) : TcpClient(ip, port, timeout) {
    }

    ModbusTcpClientBase::~ModbusTcpClientBase() {
    }

    void ModbusTcpClientBase::onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) {
        std::string data;
        if (!error.empty()) {
            onRead(ip, port, 0, data, 0x00, error);
            currentbuf_.clear();
            readBuf_.clear();
            return;
        }
        // std::cout << "modbus-tcp onRead:" << Tool::hex2String(buf, len) << std::endl;
        std::vector<Response> resps;
        unpacket(buf, len, resps);
        for (int i = 0; i < resps.size(); i++) {
            uint16_t uuid = Tool::ntohs2((char *)&resps[i].header.uuid);
            onRead(ip, port, uuid, resps[i].base.values, resps[i].base.errorCode, error);
        }
    }

    bool ModbusTcpClientBase::send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::string &data) {
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
        // std::cout << "modbus-tcp Send:" << Tool::hex2String(reqData.data(), reqData.length()) << std::endl;
        return TcpClient::send(reqData);
    }

    bool ModbusTcpClientBase::send(uint16_t uuid, uint8_t addrNo, uint8_t funcCode, uint16_t startAddr, uint16_t value,
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

    void ModbusTcpClientBase::packet(const MbapHeader &header, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen, const std::string &data,
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

        RequestBase reqBase;
        reqBase.code = funcCode;
        reqBase.startAddr = startAddr;
        reqBase.quantity = reqLen;
        reqBase.values = data;
        Modbus::packet(reqBase, reqData);
    }

    void ModbusTcpClientBase::unpacket(const char *buf, int len, std::vector<Response> &resps) {
        readBuf_ += std::string(buf, len);
        while (readBuf_.size() != 0) {
            if (currentbuf_.size() < 7) {
                int addlen = 7 - currentbuf_.size();
                if (readBuf_.size() < addlen) {
                    currentbuf_.append(readBuf_);
                    readBuf_.clear();
                    continue;
                }
                currentbuf_.append(readBuf_.substr(0, addlen));
                readBuf_.erase(0, addlen);
            }
            MbapHeader *header = (MbapHeader *)currentbuf_.data();
            uint16_t totalLen = Tool::ntohs2((char *)&header->len);
            // std::cout << "unpacket.len:" << len << std::endl;
            if (currentbuf_.size() < totalLen) {
                int addlen = totalLen - currentbuf_.size();
                if (readBuf_.size() < addlen) {
                    currentbuf_.append(readBuf_);
                    readBuf_.clear();
                    continue;
                }
                currentbuf_.append(readBuf_.substr(0, addlen));
                readBuf_.erase(0, addlen);
            }
            char *data = currentbuf_.data() + sizeof(MbapHeader);
            ResponseBase respBase;
            Modbus::unpack(*data, data, respBase);
            Response resp;
            resp.header = *header;
            resp.base = respBase;
            resps.emplace_back(resp);
            currentbuf_.clear();
        }
    }
};  // namespace modbus::tcp