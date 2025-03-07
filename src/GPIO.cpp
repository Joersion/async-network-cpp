#include "GPIO.h"

#include <sys/inotify.h>

#include <iostream>

#include "ConnectionPool.h"

namespace gpio {
    Session::Session(Connection *conn, boost::asio::io_context &ioContext, int timeout)
        : SessionBase(ioContext, timeout),
          conn_(conn),
          listenfd_(inotify_init1(IN_NONBLOCK)),
          readfd_(-1),
          listener_(boost::asio::make_strand(ioContext), listenfd_),
          reader_(boost::asio::make_strand(ioContext)) {
    }

    bool Session::open(std::string &err, const std::string &portName, IOType iotype) {
        try {
            if (listenfd_ == -1) {
                err = "inotify init failed,fd = -1";
                return false;
            }
            iotype_ = iotype;
            portName_ = portName;
            // 给这个fd添加监听文件修改
            int ret = inotify_add_watch(listenfd_, portName.data(), IN_MODIFY);
            if (ret == -1) {
                err = "inotify_add_watch failed,fd = " + listenfd_;
                ::close(listenfd_);
                return false;
            }
            // 启动
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

    void Session::asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        listener_.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                             std::bind(&Session::startAsyncRecv, std::static_pointer_cast<Session>(shared_from_this()), std::placeholders::_1));
    }

    void Session::readHandle(int len, const std::string &error) {
        conn_->onRead(portName_, recvBuf_, len, error);
    }

    void Session::asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        boost::asio::async_write(listener_, boost::asio::buffer(msg.data(), msg.size()), cb);
    }

    void Session::writeHandle(const int len, const std::string &error) {
        conn_->onWrite(portName_, len, error);
    }

    void Session::closeSession() {
        if (listenfd_ > 0) {
            conn_->doClose(portName_, "");
            boost::system::error_code ignored_ec;
            ::close(listenfd_);
        }
    }

    void Session::timerHandle() {
        conn_->onTimer(portName_);
    }

    bool Session::errorClose(const boost::system::error_code &error) {
        // 遇到读到终止符,对端关闭,管道破裂需要关闭操作
        if (error == boost::asio::error::broken_pipe || error == boost::asio::error::shut_down) {
            close();
            return false;
        }
        return true;
    }

    void Session::startAsyncRecv(const boost::system::error_code &error) {
        if (!error) {
            // 调用 async_read_some 或其他方法时需要确保 cbRead_ 类型正确
            std::cout << "监听到数据:" << std::endl;
            readfd_ = ::open(portName_.data(), iotype_);
            if (readfd_ == -1) {
                conn_->onListenError(portName_, "file open error,fileName:" + portName_);
            }
            // reader_.async_read_some(boost::asio::buffer(recvBuf_, IO_BUFFER_MAX_LEN), cbRead_);
        } else {
            conn_->onListenError(portName_, error.message());
            close();
        }
    }

    GPIO::GPIO(int timeout)
        : ioContext_(net::ConnectionPool::instance().getContext()), session_(std::make_shared<Session>(this, ioContext_, timeout)), stop_(false) {
    }

    GPIO::~GPIO() {
        stop_ = true;
    }

    bool GPIO::open(std::string &err, const std::string &fileName, IOType type) {
        return session_->open(err, fileName, type);
    }

    bool GPIO::send(const std::string &data) {
        if (session_.get()) {
            return session_->send(data.data(), data.length());
        }
        return false;
    }

    void GPIO::doClose(const std::string &portName, const std::string &error) {
        if (!stop_) {
            onClose(portName, error);
        }
        if (session_.get()) {
            session_.reset();
        }
    }

};  // namespace gpio