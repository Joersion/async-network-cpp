#pragma once

#include <map>

#include "Socket.h"

namespace net::socket {
    class TcpServer : public Connection {
    public:
        TcpServer(int port, int timeout = 0);
        virtual ~TcpServer();
        TcpServer(const TcpServer& other) = delete;
        TcpServer& operator=(const TcpServer& other) = delete;

    public:
        void start();
        bool send(const std::string& ip, int port, const std::string& msg);
        void close(const std::string& ip, int port);
        std::string ipPort(const std::string& ip, int port);
        // 重写关闭方法，防止子类继续重写
        virtual void doClose(const std::string& ip, int port, const std::string& error) override final;

    private:
        void accept();
        void acceptHandle(std::shared_ptr<Session> session, const boost::system::error_code& error);
        void addSession(std::shared_ptr<Session> session);
        void delSession(const std::string& ip, int port);

    private:
        boost::asio::io_context& ioContext_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::map<std::string, std::shared_ptr<Session>> sessions_;
        std::mutex mutex_;
        int timeout_;
        bool stop_;
    };
};  // namespace net::socket
