#include "SocketCAN.h"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/socket.h>

#include <algorithm>

#include "ConnectionPool.h"

namespace can {
    class Frame {
    public:
        Frame(ProtocolType type) : type_(type) {
        }
        ~Frame() = default;

        void setFrame(int canId, const char *data, int len) {
            if (type_ == ProtocolType::CAN2A) {
                frame_.can_id = canId;
            } else {
                frame_.can_id = canId | CAN_EFF_FLAG;
            }
            frame_.can_dlc = len;
            std::memcpy(frame_.data, data, std::min(len, (int)sizeof(frame_.data)));
        }

        can_frame *buf() {
            return &frame_;
        }

        unsigned int canId() {
            unsigned int canId;
            if (type_ == ProtocolType::CAN2A) {
                canId = frame_.can_id;
            } else {
                canId = frame_.can_id & ~CAN_EFF_FLAG;
            }
            return canId;
        }

        ProtocolType type() const {
            return type_;
        }

        unsigned char canDlc() {
            return frame_.can_dlc;
        }

        unsigned char *canData() {
            return frame_.data;
        }

        int len() const {
            return sizeof(can_frame);
        }

        void clear() {
            memset((void *)&frame_, 0, sizeof(can_frame));
        }

    private:
        ProtocolType type_;
        can_frame frame_;
    };

    Session::Session(Connection *conn, ProtocolType type, boost::asio::io_context &ioContext, int timeout)
        : SessionBase(ioContext, timeout), conn_(conn), frame_(std::make_unique<Frame>(type)), socket_(boost::asio::make_strand(ioContext)) {
    }

    Session::~Session() {
    }

    bool Session::open(std::string &err, const std::string &canName) {
        try {
            int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
            struct sockaddr_can addr;
            addr.can_family = AF_CAN;
            addr.can_ifindex = if_nametoindex(canName.c_str());
            if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                ::close(sock);
                err = "端口绑定失败";
                return false;
            }
            canName_ = canName;
            socket_.assign(boost::asio::posix::stream_descriptor::native_handle_type(sock));
            start();
        } catch (...) {
            err = "未知错误";
            return false;
        }
        return true;
    }

    std::string Session::getName() {
        return canName_;
    }

    void Session::asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        boost::asio::async_read(socket_, boost::asio::buffer(frame_->buf(), frame_->len()), cb);
    }

    void Session::readHandle(int len, const std::string &error) {
        conn_->onRead(getName(), frame_->canId(), (char *)frame_->canData(), (int)frame_->canDlc(), error);
        frame_->clear();
    }

    void Session::asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        boost::asio::async_write(socket_, boost::asio::buffer(msg.data(), msg.size()), cb);
    }

    void Session::writeHandle(const int len, const std::string &error) {
        conn_->onWrite(getName(), len, error);
    }

    void Session::closeSession() {
        if (socket_.is_open()) {
            conn_->doClose(getName(), "");
            boost::system::error_code ignored_ec;
            socket_.close(ignored_ec);
        }
    }

    void Session::timerHandle() {
        conn_->onTimer(getName());
    }

    CANTransceiver::CANTransceiver(ProtocolType type, int timeout)
        : ioContext_(net::ConnectionPool::instance().getContext()),
          session_(std::make_shared<Session>(this, type, ioContext_, timeout)),
          type_(type),
          stop_(false) {
    }

    CANTransceiver::~CANTransceiver() {
        stop_ = true;
    }

    bool CANTransceiver::open(std::string &error, const std::string &canName) {
        return session_->open(error, canName);
    }

    bool CANTransceiver::send(const std::string &data, int canId) {
        if (session_.get()) {
            Frame frame(type_);
            frame.setFrame(canId, data.c_str(), data.length());
            return session_->send((char *)frame.buf(), frame.len());
        }
        return false;
    }

    void CANTransceiver::doClose(const std::string &canName, const std::string &error) {
        if (!stop_) {
            onClose(canName, error);
        }
        if (session_.get()) {
            session_.reset();
        }
    }

};  // namespace can