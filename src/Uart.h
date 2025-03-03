#pragma once
#include "IO.h"

using namespace io;

namespace uart {
    // 停止位
    enum class StopBits { one, onepointfive, two };
    // 校验位
    enum class Parity { none, odd, even };
    // 控制流
    enum class FlowControl { none, software, hardware };

    struct Config {
        int baudRate = 9600;
        int characterSize = 8;
        StopBits stopBits = StopBits::one;
        Parity parity = Parity::none;
        FlowControl flowControl = FlowControl::none;
    };

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
        Session(Connection *conn, boost::asio::io_context &ioContext, int timeout = 0);
        ~Session() {
        }

    public:
        bool open(std::string &err, const std::string &portName, const Config &config);
        std::string getName();
        Config getConfig();

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
        Config config_;
    };

    class SerialPort : public Connection {
    public:
        SerialPort(int sendInterval = 0, int timeout = 0);
        virtual ~SerialPort();
        SerialPort(const SerialPort &other) = delete;
        SerialPort &operator=(const SerialPort &other) = delete;

    public:
        bool open(std::string &err, const std::string &portName, const Config &config);
        bool send(const std::string &data);
        bool setSendInterval(int interval);
        // 重写关闭方法，防止子类继续重写
        virtual void doClose(const std::string &portName, const std::string &error) override final;

    private:
        void startSendTimer();
        void doSendTimer();

    private:
        boost::asio::io_context &ioContext_;
        std::shared_ptr<Session> session_;
        bool stop_;

        // 发送间隔
        boost::asio::deadline_timer sendIntervalTimer_;
        std::atomic<int> sendInterval_;
        std::queue<std::string> sendBuf_;
        std::mutex sendLock_;
    };
};  // namespace uart