#pragma once

#include "IO.h"

using namespace io;

namespace can {
    enum ProtocolType { CAN2A, CAN2B };

    class Frame;

    class Connection {
    public:
        Connection() {
        }
        virtual ~Connection() {
        }

    public:
        // 读到数据之后
        virtual void onRead(const std::string &canName, unsigned int canId, const char *data, int len, const std::string &error) = 0;
        // 数据写入之后
        virtual void onWrite(const std::string &canName, const int len, const std::string &error) = 0;
        // 连接上之后
        virtual void onConnect(const std::string &canName, const std::string &error) = 0;
        // 连接关闭之前
        virtual void onClose(const std::string &canName, const std::string &error) = 0;
        // 定时器发生之后
        virtual void onTimer(const std::string &canName) = 0;
        // 用于父类
        virtual void doClose(const std::string &canName, const std::string &error) = 0;
    };

    class Session : public SessionBase {
    public:
        Session(Connection *conn, ProtocolType type, boost::asio::io_context &ioContext, int timeout = 0);
        ~Session();

    public:
        bool open(std::string &err, const std::string &canName);
        std::string getName();

    protected:
        virtual void asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) override;
        virtual void readHandle(int len, const std::string &error) override;
        virtual void asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) override;
        virtual void writeHandle(const int len, const std::string &error) override;
        virtual void closeSession() override;
        virtual void timerHandle() override;

    private:
        Connection *conn_;
        std::unique_ptr<Frame> frame_;
        boost::asio::posix::stream_descriptor socket_;
        std::string canName_;
    };

    class CANTransceiver : public Connection {
    public:
        CANTransceiver(ProtocolType type, int timeout = 0);
        virtual ~CANTransceiver();
        CANTransceiver(const CANTransceiver &other) = delete;
        CANTransceiver &operator=(const CANTransceiver &other) = delete;

    public:
        bool open(std::string &error, const std::string &canName);
        void send(const std::string &data, int canId);
        // 重写关闭方法，防止子类继续重写
        virtual void doClose(const std::string &canName, const std::string &error) override final;

    private:
        ProtocolType type_;
        boost::asio::io_context &ioContext_;
        std::shared_ptr<Session> session_;
        bool stop_;
    };
};  // namespace can