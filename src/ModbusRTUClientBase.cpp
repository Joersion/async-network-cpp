#include "ModbusRTUClientBase.h"

#include <iostream>

#include "Tool.h"

namespace modbus::rtu {

    ModbusRTUClientBase::ModbusRTUClientBase(int timeout) : SerialPort(timeout) {
    }

    ModbusRTUClientBase::~ModbusRTUClientBase() {
    }

    void ModbusRTUClientBase::onRead(const std::string &portName, const char *buf, size_t len, const std::string &error) {
        std::string data;
        if (!error.empty()) {
            onRead(portName, 0, data, error);
            data_.clear();
            return;
        }
        std::vector<Response> resps;
        unpacket(buf, len, resps);
        for (int i = 0; i < resps.size(); i++) {
            uint8_t uuid = resps[i].uuid;
            onRead(portName, (int)uuid, resps[i].base.values, error);
        }
    }

    bool ModbusRTUClientBase::send(uint16_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::string &data) {
        if (data.length() > 250) {
            return false;
        }

        std::string reqData;
        packet(uuid, funcCode, startAddr, value, data, reqData);
        return SerialPort::send(reqData);
    }

    bool ModbusRTUClientBase::send(uint16_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::vector<uint16_t> &data) {
        std::string str;
        char buf[2];
        for (int i = 0; i < data.size(); i++) {
            Tool::htons2(data[i], buf);
            str.append(buf, 2);
        }
        Tool::tolittle(str);
        return send(uuid, funcCode, startAddr, value, str);
    }

    void ModbusRTUClientBase::packet(uint16_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen, const std::string &data,
                                     std::string &reqData) {
        char uuidBuf[2], crcBuf[2];

        Tool::htons2(uuid, uuidBuf);
        reqData.append(uuidBuf, 2);

        RequestBase reqBase;
        reqBase.code = funcCode;
        reqBase.startAddr = startAddr;
        reqBase.quantity = reqLen;
        reqBase.values = data;
        Modbus::packet(reqBase, reqData);

        Tool::htons2(Tool::modbus_crc16(reqData), crcBuf);
        reqData.append(crcBuf, 2);
    }

    void ModbusRTUClientBase::unpacket(const char *buf, int len, std::vector<Response> &resps) {
        std::string bufStr(buf, len);
        while (bufStr.size() != 0) {
            if (data_.size() < 2) {
                int addlen = 2 - data_.size();
                if (bufStr.size() < addlen) {
                    data_.append(bufStr);
                    return;
                }
                data_.append(bufStr.substr(0, addlen));
                bufStr.erase(0, addlen);
            }
            uint8_t uuid = data_[0];
            uint8_t code = data_[1];
            int len = Modbus::dataSize(code);
            int totalLen = len + 3;
            std::cout << "unpacket.code:" << code << std::endl;
            std::cout << "unpacket.len:" << len << std::endl;
            if (data_.size() < len + 3) {
                int addlen = totalLen - data_.size();
                if (bufStr.size() < addlen) {
                    data_.append(bufStr);
                    return;
                }
                data_.append(bufStr.substr(0, addlen));
                bufStr.erase(0, addlen);
            }
            uint16_t crc = Tool::modbus_crc16(std::string(data_.data(), 1 + len));
            if (Tool::ntohs2(data_.data() + 1 + len) != crc) {
                std::cout << "crc error ,local crc:" << crc << ",remote crc:" << Tool::ntohs2(data_.data() + 1 + len) << std::endl;
                data_.clear();
                return;
            }
            char *data = data_.data() + 1;
            ResponseBase respBase;
            Modbus::unpack(*data, data, respBase);
            Response resp;
            resp.uuid = uuid;
            resp.base = respBase;
            resp.crc = crc;
            resps.emplace_back(resp);
            data_.clear();
        }
    }
}  // namespace modbus::rtu