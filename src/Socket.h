#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <queue>
#define BUFFER_MAX_LEN 4096

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

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(Connection *conn, boost::asio::io_context &ioContext, int timeout = 0);
        ~Session() {
        }

    public:
        boost::asio::ip::tcp::socket &getSocket();
        std::string ip();
        int port();
        void start();
        void send(const char *msg, size_t len);
        void close();

    private:
        void syncRecv();
        void readHandle(const boost::system::error_code &error, size_t len);

        void syncSend(const std::string &msg);
        void writeHandle(const boost::system::error_code &error, size_t len);

        void startTimer();
        void timerHandle();

    private:
        Connection *conn_;
        boost::asio::ip::tcp::socket socket_;
        char recvBuf_[BUFFER_MAX_LEN];
        std::queue<std::string> sendBuf_;
        std::mutex sendLock_;
        std::atomic<bool> isClose_;
        int timeout_;
        boost::asio::deadline_timer timer_;
    };
};  // namespace net::socket
