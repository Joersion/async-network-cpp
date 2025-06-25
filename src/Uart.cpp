#include "Uart.h"

#include "ConnectionPool.h"

namespace uart {
    Session::Session(Connection *conn, boost::asio::io_context &ioContext, int timeout)
        : SessionBase(ioContext, timeout), conn_(conn), serialPort_(boost::asio::make_strand(ioContext)) {
    }

    bool Session::open(std::string &err, const std::string &portName, const Config &config) {
        try {
            if (serialPort_.is_open()) {
                close("");
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

    void Session::closeSession(const std::string &err) {
        if (serialPort_.is_open()) {
            conn_->doClose(portName_, err);
            boost::system::error_code ignored_ec;
            serialPort_.close(ignored_ec);
        }
    }

    void Session::timerHandle() {
        conn_->onTimer(portName_);
    }

    SerialPort::SerialPort(int timeout)
        : ioContext_(net::ConnectionPool::instance().getContext()),
          session_(std::make_shared<Session>(this, ioContext_, timeout)),
          stop_(false),
          sendInterval_(0),
          sendIntervalTimer_(boost::asio::make_strand(ioContext_)) {
    }

    SerialPort::~SerialPort() {
        stop_ = true;
    }

    bool SerialPort::open(std::string &err, const std::string &portName, const Config &config) {
        return session_->open(err, portName, config);
    }

    bool SerialPort::send(const std::string &data) {
        if (session_.get()) {
            if (sendInterval_.load() == 0) {
                return session_->send(data.data(), data.length());
            }
            std::lock_guard<std::mutex> lock(sendLock_);
            sendBuf_.push(data);
            return true;
        }
        return session_->send(data.data(), data.length());
    }

    bool SerialPort::setSendInterval(int interval) {
        sendInterval_.store(interval, std::memory_order_relaxed);
        startSendTimer();
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

    void SerialPort::startSendTimer() {
        sendIntervalTimer_.expires_from_now(boost::posix_time::milliseconds(sendInterval_.load()));
        sendIntervalTimer_.async_wait(std::bind(&SerialPort::doSendTimer, this));
    }

    void SerialPort::doSendTimer() {
        std::string data;
        {
            std::lock_guard<std::mutex> lock(sendLock_);
            if (sendBuf_.size() > 0) {
                data = sendBuf_.front();
                sendBuf_.pop();
            }
        }
        if (!data.empty()) {
            session_->send(data.data(), data.length());
        }
        startSendTimer();
    }

};  // namespace uart