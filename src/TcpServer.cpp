#include "TcpServer.h"

#include "ConnectionPool.h"

namespace net::socket {
    TcpServer::TcpServer(int port, int timeout)
        : ioContext_(net::ConnectionPool::instance().getContext()),
          acceptor_(boost::asio::make_strand(ioContext_), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          timeout_(timeout),
          stop_(false) {
    }

    TcpServer::~TcpServer() {
        stop_ = true;
        std::vector<std::shared_ptr<Session>> tmps;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto session : sessions_) {
                tmps.emplace_back(session.second);
            }
        }
        for (auto session : tmps) {
            session->close();
        }
    }

    void TcpServer::start() {
        accept();
    }

    void TcpServer::doClose(const std::string &ip, int port, const std::string &error) {
        if (!stop_) {
            onClose(ip, port, error);
        }
        delSession(ip, port);
    }

    void TcpServer::accept() {
        std::shared_ptr<Session> session = std::make_shared<Session>(this, ioContext_, timeout_);
        if (!session.get()) {
            return;
        }
        acceptor_.async_accept(session->getSocket(), std::bind(&TcpServer::acceptHandle, this, session, std::placeholders::_1));
    }

    void TcpServer::acceptHandle(std::shared_ptr<Session> session, const boost::system::error_code &error) {
        std::string ip = session->ip();
        int port = session->port();
        std::string err;
        if (!error) {
            addSession(session);
            session->start();
            onConnect(ip, port, err);
            accept();
        } else {
            err = error.what();
            if (error != boost::asio::error::operation_aborted) {
                onConnect(ip, port, err);
            }
            session->close();
        }
    }

    bool TcpServer::send(const std::string &ip, int port, const std::string &msg) {
        std::string key = ipPort(ip, port);
        std::shared_ptr<Session> session;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sessions_.find(key);
            if (it != sessions_.end()) {
                session = sessions_[key];
            }
        }
        if (session.get()) {
            return session->send(msg.data(), msg.length());
        }
        return false;
    }

    void TcpServer::close(const std::string &ip, int port) {
        std::string key = ipPort(ip, port);
        std::shared_ptr<Session> session;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sessions_.find(key);
            if (it != sessions_.end()) {
                session = sessions_[key];
            }
        }
        if (session.get()) {
            session->close();
        }
    }

    std::string TcpServer::ipPort(const std::string &ip, int port) {
        return ip + ":" + std::to_string(port);
    }

    void TcpServer::addSession(std::shared_ptr<Session> session) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = ipPort(session->ip(), session->port());
        sessions_[key] = session;
    }

    void TcpServer::delSession(const std::string &ip, int port) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = ipPort(ip, port);
        sessions_.erase(key);
    }

};  // namespace net::socket