#include "IO.h"

#include <boost/core/ignore_unused.hpp>

#include "ConnectionPool.h"

namespace io {
    SessionBase::SessionBase(boost::asio::io_context &ioContext, int timeout)
        : timeout_(timeout), timer_(boost::asio::make_strand(ioContext), boost::posix_time::microseconds(timeout)) {
    }

    void SessionBase::start() {
        isClose_.store(false);
        cbWrite_ = std::bind(&SessionBase::doWrite, shared_from_this(), std::placeholders::_1, std::placeholders::_2);
        cbRead_ = std::bind(&SessionBase::doRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2);
        syncRecv(recvBuf_, IO_BUFFER_MAX_LEN, cbRead_);
        startTimer();
    }

    void SessionBase::send(const char *msg, size_t len) {
        if (isClose_) {
            return;
        }
        std::string data(msg, len);
        {
            std::lock_guard<std::mutex> lock(sendLock_);
            if (sendBuf_.size() > 0) {
                sendBuf_.push(data);
                return;
            }
        }
        syncSend(data, cbWrite_);
    }

    void SessionBase::close() {
        isClose_.store(true);
        timeout_ = 0;
        timer_.cancel();
        closeSession();

        std::lock_guard<std::mutex> lock(sendLock_);
        sendBuf_ = std::move(std::queue<std::string>());
    }

    void SessionBase::doRead(const boost::system::error_code &error, size_t len) {
        std::string err;
        if (!error) {
            if (isClose_) {
                return;
            }
            readHandle(recvBuf_, len, err);
            syncRecv(recvBuf_, IO_BUFFER_MAX_LEN, cbRead_);
        } else {
            err = error.what();
            if (error != boost::asio::error::operation_aborted) {
                readHandle(recvBuf_, len, err);
            }
            close();
        }
    }

    void SessionBase::doWrite(const boost::system::error_code &error, size_t len) {
        boost::ignore_unused(len);
        std::string data, err;
        if (!error) {
            if (isClose_) {
                return;
            }
            {
                std::lock_guard<std::mutex> lock(sendLock_);
                if (sendBuf_.size() > 0) {
                    data = sendBuf_.front();
                    sendBuf_.pop();
                } else {
                    return;
                }
            }
            writeHandle(len, err);
            syncSend(data, cbWrite_);
        } else {
            err = error.what();
            if (error != boost::asio::error::operation_aborted) {
                writeHandle(len, err);
            }
            close();
        }
    }

    void SessionBase::startTimer() {
        if (timeout_ == 0 || isClose_) {
            return;
        }
        timer_.expires_from_now(boost::posix_time::milliseconds(timeout_));
        timer_.async_wait(std::bind(&SessionBase::doTimer, shared_from_this()));
    }

    void SessionBase::doTimer() {
        if (timeout_ == 0 || isClose_) {
            return;
        }
        timerHandle();
        startTimer();
    }

};  // namespace io