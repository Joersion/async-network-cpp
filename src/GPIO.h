#pragma once
#include <map>

#include "IO.h"

using namespace io;

namespace gpio {
    enum IOType { unknowType = -1, readOnly = O_RDONLY, writeOnly = O_WRONLY, readWrite = O_RDWR };
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
        // 发生监听错误
        virtual void onListenError(const std::string &portName, const std::string &error) = 0;
        // 用于父类
        virtual void doClose(const std::string &portName, const std::string &error) = 0;
    };

    class Session : public SessionBase {
    public:
        Session(Connection *conn, boost::asio::io_context &ioContext, int timeout = 0);
        ~Session();

    public:
        bool open(std::string &err, const std::string &portName, IOType iotype);
        bool read();
        IOType getType();
        int getFd();
        std::string getPortName();

    protected:
        virtual void asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) override;
        virtual void asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) override;
        virtual void closeSession(const std::string &err) override;
        virtual void readHandle(int len, const std::string &error) override;
        virtual void writeHandle(const int len, const std::string &error) override;
        virtual void timerHandle() override;

    private:
        Connection *conn_;
        int fd_;
        boost::asio::posix::stream_descriptor ioer_;
        std::string portName_;
        IOType type_;
    };

    class GPIO : public Connection {
    public:
        GPIO(int timeout = 0);
        virtual ~GPIO();
        GPIO(const GPIO &other) = delete;
        GPIO &operator=(const GPIO &other) = delete;

    public:
        bool add(std::string &err, const std::string &fileName, IOType type = readOnly);
        bool del(std::string &err, const std::string &fileName);
        bool send(const std::string &fileName, const std::string &data);
        // 重写关闭方法，防止子类继续重写
        virtual void doClose(const std::string &portName, const std::string &error) override final;

    private:
        void start();
        void onEvent(const boost::system::error_code &error);
        void addEpollNode(int fd);
        void delEpollNode(int fd);

    private:
        boost::asio::io_context &ioContext_;
        bool stop_;
        int timeout_;

        std::mutex mutex_;
        std::map<std::string, std::shared_ptr<Session>> sessions_;
        int epollFd_;
        boost::asio::posix::stream_descriptor listener_;
    };
};  // namespace gpio