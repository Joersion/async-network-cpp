#include "TcpClient.h"

#include "ConnectionPool.h"

namespace net::socket {
    TcpClient::TcpClient(const std::string& ip, int port, int timeout)
        : ioContext_(net::ConnectionPool::instance().getContext()),
          remote_(boost::asio::ip::address::from_string(ip), port),
          resolver_(boost::asio::make_strand(ioContext_)),
          timeout_(timeout),
          reconnectTimer_(boost::asio::make_strand(ioContext_)),
          stop_(false) {
    }

    TcpClient::~TcpClient() {
        stop_ = true;
        if (session_.get()) {
            session_->close();
        }
    }

    void TcpClient::start(int reconncetTime) {
        reconnectTimeout_ = reconncetTime;
        resolver();
    }

    bool TcpClient::send(const std::string& data) {
        if (session_) {
            return session_->send(data.data(), data.length());
        }
        return false;
    }

    void TcpClient::close() {
        if (session_) {
            session_->close();
        }
    }

    void TcpClient::resolver() {
        resolver_.async_resolve(remote_, std::bind(&TcpClient::resolverHandle, this, std::placeholders::_1, std::placeholders::_2));
    }

    void TcpClient::resolverHandle(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type endpoints) {
        std::string err;
        if (!error) {
            syncConnect(endpoints);
        } else {
            startTimer();
        }
        if (error != boost::asio::error::operation_aborted) {
            onResolver(err);
        }
    }

    void TcpClient::doClose(const std::string& ip, int port, const std::string& error) {
        if (!stop_) {
            startTimer();
        }
        if (!stop_ && !ip.empty()) {
            onClose(ip, port, error);
        }

        std::lock_guard<std::mutex> lock(mutex_);
        session_.reset();
    }

    void TcpClient::syncConnect(boost::asio::ip::tcp::resolver::results_type endpoints) {
        std::shared_ptr<Session> session = std::make_shared<Session>(this, ioContext_, timeout_);
        if (!session.get()) {
            return;
        }
        boost::asio::async_connect(session->getSocket(), endpoints,
                                   std::bind(&TcpClient::ConnectHandle, this, session, std::placeholders::_1, std::placeholders::_2));
    }

    void TcpClient::ConnectHandle(std::shared_ptr<Session> session, const boost::system::error_code& error,
                                  const boost::asio::ip::tcp::endpoint& endpoint) {
        std::string ip = session->ip();
        int port = session->port();
        std::string err;
        if (!error) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                session_ = session;
            }
            session->start();
        } else {
            err = error.what();
            session->close();
        }
        if (error != boost::asio::error::operation_aborted) {
            onConnect(ip, port, err);
        }
    }

    void TcpClient::startTimer() {
        if (reconnectTimeout_ > 0) {
            reconnectTimer_.cancel();
            reconnectTimer_.expires_from_now(boost::posix_time::milliseconds(reconnectTimeout_));
            reconnectTimer_.async_wait(std::bind(&TcpClient::timerHandle, this));
        }
    }

    void TcpClient::timerHandle() {
        resolver();
    }

};  // namespace net::socket