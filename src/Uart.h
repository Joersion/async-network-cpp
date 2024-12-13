#pragma once
#include "IO.h"

using namespace io;

namespace uart {
    class Connection {
    public:
        Connection() {
        }
        virtual ~Connection() {
        }

    public:
        // 读到数据之后
        virtual void onRead(const std::string &portName, const char *buf, size_t len, const std::string &error) = 0;
        // 数据写入之后
        virtual void onWrite(const std::string &portName, const int len, const std::string &error) = 0;
        // 连接上之后
        virtual void onConnect(const std::string &portName, const std::string &error) = 0;
        // 连接关闭之前
        virtual void onClose(const std::string &portName, const std::string &error) = 0;
        // 定时器发生之后
        virtual void onTimer(const std::string &portName) = 0;
        // 用于父类
        virtual void doClose(const std::string &portName, const std::string &error) = 0;
    };

    class Session : public SessionBase {
    public:
        Session(Connection *conn, const std::string &portName, int baud, boost::asio::io_context &ioContext, int timeout = 0);
        ~Session() {
        }

    public:
        bool open(std::string &err);
        std::string getName();
        int getBaud();

    protected:
        virtual void asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) override;
        virtual void asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) override;
        virtual void closeSession() override;
        virtual void readHandle(int len, const std::string &error) override;
        virtual void writeHandle(const int len, const std::string &error) override;
        virtual void timerHandle() override;

    private:
        char recvBuf_[IO_BUFFER_MAX_LEN];
        Connection *conn_;
        boost::asio::serial_port serialPort_;
        std::string portName_;
        int baud_;
    };

    class SerialPort : public Connection {
    public:
        SerialPort(std::string portName, int baud, int timeout = 0);
        virtual ~SerialPort();
        SerialPort(const SerialPort &other) = delete;
        SerialPort &operator=(const SerialPort &other) = delete;

    public:
        bool open(std::string &error);
        void send(const std::string &data);
        // 重写关闭方法，防止子类继续重写
        virtual void doClose(const std::string &portName, const std::string &error) override final;

    private:
        boost::asio::io_context &ioContext_;
        std::shared_ptr<Session> session_;
        bool stop_;
    };
};  // namespace uart