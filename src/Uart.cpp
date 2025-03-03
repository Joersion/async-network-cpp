#include "Uart.h"

#include "ConnectionPool.h"

namespace uart {
    Session::Session(Connection *conn, boost::asio::io_context &ioContext, int timeout)
        : SessionBase(ioContext, timeout), conn_(conn), serialPort_(boost::asio::make_strand(ioContext)) {
    }

    bool Session::open(std::string &err, const std::string &portName, const Config &config) {
        try {
            if (serialPort_.is_open()) {
                close();
            }
            portName_ = portName;
            serialPort_.open(portName);
            serialPort_.set_option(boost::asio::serial_port_base::baud_rate(config.baudRate));
            serialPort_.set_option(boost::asio::serial_port_base::character_size(config.characterSize));
            serialPort_.set_option(boost::asio::serial_port_base::stop_bits((boost::asio::serial_port_base::stop_bits::type)config.stopBits));
            serialPort_.set_option(boost::asio::serial_port_base::parity((boost::asio::serial_port_base::parity::type)config.parity));
            serialPort_.set_option(
                boost::asio::serial_port_base::flow_control((boost::asio::serial_port_base::flow_control::type)config.flowControl));

            start();
            return true;
        } catch (const boost::system::system_error &e) {
            err = e.what();
            return false;
        }
    }

    std::string Session::getName() {
        return portName_;
    }

    Config Session::getConfig() {
        return config_;
    }

    void Session::asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        serialPort_.async_read_some(boost::asio::buffer(recvBuf_, IO_BUFFER_MAX_LEN), cb);
    }

    void Session::readHandle(int len, const std::string &error) {
        conn_->onRead(portName_, recvBuf_, len, error);
    }

    void Session::asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        boost::asio::async_write(serialPort_, boost::asio::buffer(msg.data(), msg.size()), cb);
    }

    void Session::writeHandle(const int len, const std::string &error) {
        conn_->onWrite(portName_, len, error);
    }

    void Session::closeSession() {
        if (serialPort_.is_open()) {
            conn_->doClose(portName_, "");
            boost::system::error_code ignored_ec;
            serialPort_.close(ignored_ec);
        }
    }

    void Session::timerHandle() {
        conn_->onTimer(portName_);
    }

    SerialPort::SerialPort(int sendInterval, int timeout)
        : ioContext_(net::ConnectionPool::instance().getContext()), session_(std::make_shared<Session>(this, ioContext_, timeout)), stop_(false) {
    }

    SerialPort::~SerialPort() {
        stop_ = true;
    }

    bool SerialPort::open(std::string &err, const std::string &portName, const Config &config) {
        return session_->open(err, portName, config);
    }

    bool SerialPort::send(const std::string &data) {
        if (session_.get()) {
            return session_->send(data.data(), data.length());
        }
        return false;
    }

    bool SerialPort::setSendInterval(int interval) {
        if (session_.get()) {
            return session_->setSendInterval(interval);
        }
        return false;
    }

    void SerialPort::doClose(const std::string &portName, const std::string &error) {
        if (!stop_) {
            onClose(portName, error);
        }
        if (session_.get()) {
            session_.reset();
        }
    }

};  // namespace uart