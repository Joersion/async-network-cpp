#include "ModbusTcpClientBase.h"

#include <iostream>

#include "Tool.h"

#define MBAP_LEN 7

namespace modbus::tcp {
    ModbusTcpClientBase::ModbusTcpClientBase(const std::string &ip, int port, int timeout) : TcpClient(ip, port, timeout) {
    }

    ModbusTcpClientBase::~ModbusTcpClientBase() {
    }

    void ModbusTcpClientBase::onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) {
        std::string data;
        if (!error.empty()) {
            onRead(ip, port, 0, data, 0x00, error);
            std::lock_guard<std::mutex> lock(mutex_);
            currentbuf_.clear();
            readBuf_.clear();
            return;
        }
        // std::cout << "modbus-tcp onRead:" << Tool::hex2String(buf, len) << std::endl;
        std::vector<Response> resps;
        unpacket(buf, len, resps);
        for (int i = 0; i < resps.size(); i++) {
            int uuid = resps[i].header.uuid;
            onRead(ip, port, uuid, resps[i].base.values, resps[i].base.errorCode, error);
            std::cout << "uuid:" << uuid << ",values:" << Tool::hex2String(resps[i].base.values.data(), resps[i].base.values.length()) << std::endl;
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
        std::cout << "modbus-tcp Send:" << Tool::hex2String(reqData.data(), reqData.length()) << std::endl;
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
        std::lock_guard<std::mutex> lock(mutex_);
        readBuf_ += std::string(buf, len);
        while (readBuf_.size() != 0) {
            if (currentbuf_.size() < MBAP_LEN) {
                int addlen = MBAP_LEN - currentbuf_.size();
                if (readBuf_.size() < addlen) {
                    currentbuf_.append(readBuf_);
                    readBuf_.clear();
                    continue;
                }
                currentbuf_.append(readBuf_.substr(0, addlen));
                readBuf_.erase(0, addlen);
            }
            MbapHeader *header = (MbapHeader *)currentbuf_.data();
            int uuid = Tool::ntohs2((char *)&header->uuid);
            int protocolId = Tool::ntohs2((char *)&header->protocolId);
            int addrNo = Tool::ntohs2((char *)&header->addrNo);
            // dataLen 有包含了MBAP一个字节的Modbus 设备地址,需要扣掉
            int dataLen = Tool::ntohs2((char *)&header->len) - 1;
            // 总长度 = 数据长度 + 头部 MBAP 长度
            uint16_t totalLen = dataLen + MBAP_LEN;
            // std::cout << "totalLen:" << totalLen << ",dataLen:" << dataLen << ",uuid:" << uuid << ",protocolId:" << protocolId << std::endl;
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
            // std::cout << "currentbuf_:" << Tool::hex2String(currentbuf_.data(), currentbuf_.length()) << std::endl;
            char *data = (char *)currentbuf_.data() + MBAP_LEN;
            ResponseBase respBase;
            Modbus::unpack(*data, data, dataLen, respBase);
            Response resp;
            resp.header.addrNo = addrNo;
            resp.header.len = dataLen;
            resp.header.protocolId = protocolId;
            resp.header.uuid = uuid;
            resp.base = respBase;
            resps.emplace_back(resp);
            currentbuf_.clear();
        }
    }
};  // namespace modbus::tcp