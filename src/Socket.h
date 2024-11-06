#pragma once
#include "IO.h"

using namespace io;

namespace net::socket {
    class Connection {
    public:
        Connection() {
        }
        virtual ~Connection() {
        }

    public:
        // 读到数据之后
        virtual void onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) = 0;
        // 数据写入之后
        virtual void onWrite(const std::string &ip, int port, const int len, const std::string &error) = 0;
        // 连接上之后
        virtual void onConnect(const std::string &ip, int port, const std::string &error) = 0;
        // 连接关闭之前
        virtual void onClose(const std::string &ip, int port, const std::string &error) = 0;
        // 定时器发生之后
        virtual void onTimer(const std::string &ip, int port) = 0;
        // 用于父类
        virtual void doClose(const std::string &ip, int port, const std::string &error) = 0;
    };

    class Session : public SessionBase {
    public:
        Session(Connection *conn, boost::asio::io_context &ioContext, int timeout = 0);
        ~Session() {
        }

    public:
        boost::asio::ip::tcp::socket &getSocket();
        std::string ip();
        int port();

    protected:
        virtual void syncRecv(char *buf, int len, std::function<void(const boost::system::error_code &error, size_t len)> cb) override;
        virtual void syncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) override;
        virtual void closeSession() override;
        virtual void readHandle(const char *buf, size_t len, const std::string &error) override;
        virtual void writeHandle(const int len, const std::string &error) override;
        virtual void timerHandle() override;

    private:
        Connection *conn_;
        boost::asio::ip::tcp::socket socket_;
    };
};  // namespace net::socket
