#pragma once

#include "Socket.h"

namespace net::socket {
    class TcpClient : public Connection {
    public:
        TcpClient(const std::string& ip, int port, int timeout = 0);
        virtual ~TcpClient();
        TcpClient(const TcpClient& other) = delete;
        TcpClient& operator=(const TcpClient& other) = delete;

        void start(int reconncetTime = 0);
        bool send(const std::string& data);
        void close();

    public:
        // 域名解析之后
        virtual void onResolver(const std::string& error) = 0;
        // 重写关闭方法，防止子类继续重写
        virtual void doClose(const std::string& ip, int port, const std::string& error) override final;

    private:
        void resolver();
        void resolverHandle(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type endpoints);

        void syncConnect(boost::asio::ip::tcp::resolver::results_type endpoints);
        void ConnectHandle(std::shared_ptr<Session> session, const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint);

        void startTimer();
        void timerHandle();

    private:
        boost::asio::io_context& ioContext_;
        boost::asio::ip::tcp::endpoint remote_;
        boost::asio::ip::tcp::resolver resolver_;
        int timeout_;
        boost::asio::deadline_timer reconnectTimer_;
        std::shared_ptr<Session> session_;
        std::mutex mutex_;
        int reconnectTimeout_;
        bool stop_;
    };
};  // namespace net::socket