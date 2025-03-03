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
        asyncRecv(cbRead_);
        startTimer();
    }

    bool SessionBase::send(const char *msg, size_t len) {
        if (isClose_) {
            return false;
        }
        std::string data(msg, len);
        {
            std::lock_guard<std::mutex> lock(sendLock_);
            if (sendBuf_.size() > 0) {
                sendBuf_.push(data);
                return true;
            }
        }
        asyncSend(data, cbWrite_);
        return true;
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
            readHandle(len, err);
            asyncRecv(cbRead_);
        } else {
            err = error.what();
            // 程序结束释放资源，不需要回调
            if (error != boost::asio::error::operation_aborted) {
                readHandle(len, err);
            }
            // 遇到读到终止符,对端关闭,管道破裂需要关闭操作
            if (error == boost::asio::error::eof || error == boost::asio::error::broken_pipe || error == boost::asio::error::shut_down) {
                close();
            }
        }
    }

    void SessionBase::doWrite(const boost::system::error_code &error, size_t len) {
        boost::ignore_unused(len);
        std::string data, err;
        if (!error) {
            if (isClose_) {
                return;
            }
            writeHandle(len, err);
            {
                std::lock_guard<std::mutex> lock(sendLock_);
                if (sendBuf_.size() > 0) {
                    data = sendBuf_.front();
                    sendBuf_.pop();
                } else {
                    return;
                }
            }
            asyncSend(data, cbWrite_);
        } else {
            err = error.what();
            if (error != boost::asio::error::operation_aborted) {
                writeHandle(len, err);
            }
            if (error == boost::asio::error::eof || error == boost::asio::error::broken_pipe || error == boost::asio::error::shut_down) {
                close();
            }
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