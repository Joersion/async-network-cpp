#include "Socket.h"

#include <boost/core/ignore_unused.hpp>

namespace net::socket {

    Session::Session(Connection *conn, boost::asio::io_context &ioContext, int timeout)
        : SessionBase(ioContext, timeout), conn_(conn), socket_((boost::asio::make_strand(ioContext))) {
    }

    boost::asio::ip::tcp::socket &Session::getSocket() {
        return socket_;
    }

    std::string Session::ip() {
        if (!socket_.is_open()) {
            return "";
        }
        try {
            return socket_.remote_endpoint().address().to_string();
        } catch (const boost::system::system_error &e) {
            return "";
        }
    }

    int Session::port() {
        if (!socket_.is_open()) {
            return 0;
        }
        try {
            return socket_.remote_endpoint().port();
        } catch (const boost::system::system_error &e) {
            return 0;
        }
    }

    void Session::asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        socket_.async_read_some(boost::asio::buffer(recvBuf_, IO_BUFFER_MAX_LEN), cb);
    }

    void Session::readHandle(int len, const std::string &error) {
        conn_->onRead(ip(), port(), recvBuf_, len, error);
    }

    void Session::asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        boost::asio::async_write(socket_, boost::asio::buffer(msg.data(), msg.size()), cb);
    }

    void Session::writeHandle(const int len, const std::string &error) {
        conn_->onWrite(ip(), port(), len, error);
    }

    void Session::closeSession() {
        if (socket_.is_open()) {
            conn_->doClose(ip(), port(), "");
            boost::system::error_code ignored_ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket_.close(ignored_ec);
        }
    }

    void Session::timerHandle() {
        conn_->onTimer(ip(), port());
    }

};  // namespace net::socket