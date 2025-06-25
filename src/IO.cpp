#include "IO.h"

#include <boost/core/ignore_unused.hpp>
#include <set>

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

    void SessionBase::close(const std::string &err) {
        isClose_.store(true);
        timeout_ = 0;
        timer_.cancel();
        closeSession(err);

        std::lock_guard<std::mutex> lock(sendLock_);
        sendBuf_ = std::move(std::queue<std::string>());
    }

    bool SessionBase::errorClose(const boost::system::error_code &error) {
        // 无需关闭的错误列表（临时性错误，可恢复）
        static const std::set<boost::system::error_code> errors = {
            boost::asio::error::message_size,  // 数据包过大（若协议允许分片可不关闭）
            boost::asio::error::try_again,     // 资源暂时不可用（如缓冲区满）
            boost::asio::error::would_block,   // 非阻塞操作未就绪（EAGAIN）
            boost::asio::error::interrupted,   // 系统调用被信号中断
            boost::asio::error::in_progress    // 非阻塞连接进行中
        };

        // 检查是否为可恢复错误
        if (errors.count(error)) {
            return true;  // 不关闭连接
        }

        // 其他情况视为致命错误，关闭连接
        close(error.what());
        return false;
    }

    void SessionBase::doRead(const boost::system::error_code &error, size_t len) {
        std::string err;
        if (!error) {
            if (isClose_) {
                std::cout << "SessionBase::doRead isclose" << std::endl;
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
            if (errorClose(error)) {
                asyncRecv(cbRead_);
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
            if (errorClose(error)) {
                asyncSend(data, cbWrite_);
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