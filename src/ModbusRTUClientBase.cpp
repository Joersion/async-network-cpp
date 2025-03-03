#include "ModbusRTUClientBase.h"

#include <iostream>

#include "Tool.h"

namespace modbus::rtu {

    ModbusRTUClientBase::ModbusRTUClientBase(int timeout) : SerialPort(timeout) {
    }

    ModbusRTUClientBase::~ModbusRTUClientBase() {
    }

    void ModbusRTUClientBase::onRead(const std::string &portName, const char *buf, size_t len, const std::string &error) {
        std::cout << "modbusRTU readData:" << Tool::hex2String(buf, len) << std::endl;
        std::string data;
        if (!error.empty()) {
            onRead("", portName, 0, data, 0x00, error);
            readBuf_.clear();
            currentbuf_.clear();
            return;
        }
        std::vector<Response> resps;
        unpacket(buf, len, resps);
        for (int i = 0; i < resps.size(); i++) {
            uint8_t uuid = resps[i].uuid;
            std::cout << "modbusRTU onRead, data:" << Tool::hex2String(resps[i].base.values.data(), resps[i].base.values.length()) << std::endl;
            onRead(resps[i].src, portName, (int)uuid, resps[i].base.values, resps[i].base.errorCode, error);
        }
    }

    bool ModbusRTUClientBase::send(uint8_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::string &data) {
        if (data.length() > 250) {
            return false;
        }

        std::string reqData = "";
        packet(uuid, funcCode, startAddr, value, data, reqData);
        std::cout << "modbusRTU sendData:" << Tool::hex2String(reqData.data(), reqData.length()) << std::endl;
        return SerialPort::send(reqData);
    }

    bool ModbusRTUClientBase::send(uint8_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t value, const std::vector<uint16_t> &data) {
        std::string str;
        char buf[2];
        for (int i = 0; i < data.size(); i++) {
            Tool::htons2(data[i], buf);
            str.append(buf, 2);
        }
        Tool::tolittle(str);
        return send(uuid, funcCode, startAddr, value, str);
    }

    void ModbusRTUClientBase::packet(uint8_t uuid, uint8_t funcCode, uint16_t startAddr, uint16_t reqLen, const std::string &data,
                                     std::string &reqData) {
        char crcBuf[2];
        reqData.append(1, uuid);
        reqData.append(1, funcCode);

        RequestBase reqBase;
        reqBase.code = funcCode;
        reqBase.startAddr = startAddr;
        reqBase.quantity = reqLen;
        reqBase.values = data;
        Modbus::packet(reqBase, reqData);

        uint16_t crc = Tool::modbus_crc16(reqData);
        reqData.append((char *)&crc, sizeof(crc));
    }

    void ModbusRTUClientBase::unpacket(const char *buf, int len, std::vector<Response> &resps) {
        readBuf_ += std::string(buf, len);
        while (readBuf_.length() != 0) {
            if (currentbuf_.length() < 2) {
                // 获取前两个字节代表 uuid和功能码
                int addlen = 2 - currentbuf_.length();
                if (readBuf_.length() < addlen) {
                    currentbuf_.append(readBuf_);
                    readBuf_.clear();
                    continue;
                }
                currentbuf_.append(readBuf_.substr(0, addlen));
                readBuf_.erase(0, addlen);
            }
            uint8_t uuid = currentbuf_[0];
            uint8_t code = currentbuf_[1];
            // 0：读 1:写 2：错误 -1:抛弃
            int action = Modbus::action(code);
            if (action == 0) {
                // 说明读操作响应，需要再读一个字节，代表数据长度
                if (currentbuf_.length() < 3) {
                    int addlen = 3 - currentbuf_.length();
                    if (readBuf_.length() < addlen) {
                        currentbuf_.append(readBuf_);
                        readBuf_.clear();
                        continue;
                    }
                    currentbuf_.append(readBuf_.substr(0, addlen));
                    readBuf_.erase(0, addlen);
                }
                int dataLen = (int)currentbuf_[2];
                // 读取读数据响应和最后两个crc字节
                if (currentbuf_.length() < dataLen + 5) {
                    int addlen = dataLen + 5 - currentbuf_.length();
                    if (readBuf_.length() < addlen) {
                        currentbuf_.append(readBuf_);
                        readBuf_.clear();
                        continue;
                    }
                    currentbuf_.append(readBuf_.substr(0, addlen));
                    readBuf_.erase(0, addlen);
                }
                uint16_t crc = Tool::modbus_crc16(std::string(currentbuf_.data(), currentbuf_.length() - 2));
                if (currentbuf_.length() > 2 && *(uint16_t *)(currentbuf_.data() + currentbuf_.length() - 2) != crc) {
                    std::cout << "crc error ,local crc:" << std::hex << crc << ",remote crc:" << Tool::ntohs2(readBuf_.data() + dataLen)
                              << ",readLen:" << readBuf_.length() << std::endl;

                    std::cout << "crc error ,currentDta:" << Tool::hex2String(currentbuf_.data(), currentbuf_.length()) << std::endl;
                    currentbuf_.clear();
                    readBuf_.clear();
                    continue;
                }
                Response resp;
                resp.uuid = uuid;
                resp.base.code = code;
                resp.base.values = std::string(currentbuf_.data() + 3, dataLen);
                resp.base.quantity = dataLen;
                resp.crc = crc;
                resp.src = currentbuf_;
                std::cout << "modbusRTU unpacket:" << Tool::hex2String(currentbuf_.data(), currentbuf_.length()) << std::endl;
                resps.emplace_back(resp);
                currentbuf_.clear();
            } else if (action > 0) {
                // 写响应抓6个字节(起始地址2字节 + 值2字节（写入值或写入数量） +CRC 2字节 )
                int needReadLen = 8;
                // 错误响应5字节(错误码1字节 + CRC校验2字节)
                if (action == 2) {
                    needReadLen = 5;
                }

                if (currentbuf_.length() < needReadLen) {
                    int addlen = needReadLen - currentbuf_.length();
                    if (readBuf_.length() < addlen) {
                        currentbuf_.append(readBuf_);
                        readBuf_.clear();
                        continue;
                    }
                    currentbuf_.append(readBuf_.substr(0, addlen));
                    readBuf_.erase(0, addlen);
                }

                uint16_t crc = Tool::modbus_crc16(std::string(currentbuf_.data(), currentbuf_.length() - 2));
                if (currentbuf_.length() > 2 && *(uint16_t *)(currentbuf_.data() + currentbuf_.length() - 2) != crc) {
                    std::cout << "crc error ,local crc:" << std::hex << crc
                              << ",remote crc:" << Tool::ntohs2(currentbuf_.data() + currentbuf_.length() - 2) << ",readLen:" << readBuf_.length()
                              << std::endl;

                    std::cout << "crc error ,currentDta:" << Tool::hex2String(currentbuf_.data(), currentbuf_.length()) << std::endl;
                    currentbuf_.clear();
                    readBuf_.clear();
                    continue;
                }

                Response resp;
                resp.uuid = uuid;
                resp.base.code = code;
                if (action == 1) {
                    resp.base.values = std::string(currentbuf_.data() + currentbuf_.length() - 4, 2);
                } else if (action == 2) {
                    resp.base.errorCode = currentbuf_[2];
                }
                resp.src = currentbuf_;
                std::cout << "modbusRTU unpacket:" << Tool::hex2String(currentbuf_.data(), currentbuf_.length()) << std::endl;
                resps.emplace_back(resp);
                currentbuf_.clear();
            } else {
                // 抛弃继续寻找下一个帧
                currentbuf_.clear();
                readBuf_.clear();
                continue;
            }
        }
    }
}  // namespace modbus::rtu