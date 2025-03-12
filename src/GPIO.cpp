#include "GPIO.h"

#include <sys/inotify.h>

#include <iostream>

#include "ConnectionPool.h"

namespace gpio {
    Session::Session(Connection *conn, boost::asio::io_context &ioContext, int timeout)
        : SessionBase(ioContext, timeout), conn_(conn), fd_(-1), epollFd_(-1), reader_(boost::asio::make_strand(ioContext)) {
    }

    bool Session::open(std::string &err, const std::string &portName, IOType iotype) {
        try {
            if (fd_ != -1) {
                close();
            }
            fd_ = ::open(portName.data(), O_RDWR | O_NONBLOCK);
            if (fd_ == -1) {
                return false;
            }
            iotype_ = iotype;
            portName_ = portName;
            reader_.assign(fd_);
            setEdgeTriggered();
            // 启动
            start();
            return true;
        } catch (const boost::system::system_error &e) {
            err = e.what();
            return false;
        } catch (const std::runtime_error &e) {
            err = e.what();
            return false;
        }
    }

    std::string Session::getName() {
        return portName_;
    }

    void Session::asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        reader_.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                           std::bind(&Session::startAsyncRecv, std::static_pointer_cast<Session>(shared_from_this()), std::placeholders::_1));
    }

    void Session::asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        boost::asio::async_write(reader_, boost::asio::buffer(msg.data(), msg.size()), cb);
    }

    void Session::readHandle(int len, const std::string &error) {
        conn_->onRead(portName_, recvBuf_, len, error);
    }

    void Session::writeHandle(const int len, const std::string &error) {
        conn_->onWrite(portName_, len, error);
    }

    void Session::closeSession() {
        if (fd_ > 0) {
            conn_->doClose(portName_, "");
            boost::system::error_code ignored_ec;
            ::close(fd_);
            ::close(epollFd_);
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

            char buf[2];
            lseek(fd_, 0, SEEK_SET);  // 重置文件指针
            ssize_t len = ::read(fd_, buf, sizeof(buf));
            if (len > 0) {
                int value = std::atoi(buf);
                std::cout << "GPIO state changed: " << value << std::endl;
                asyncRecv(nullptr);  // 继续监听
            } else if (len == -1) {
                std::cerr << "Failed to read GPIO state" << std::endl;
                conn_->onListenError(portName_, "读取错误");
                close();
            }

        } else {
            conn_->onListenError(portName_, error.message());
            close();
        }
    }

    void Session::setEdgeTriggered() {
        if (epollFd_ != -1) {
            ::close(epollFd_);
        }

        epollFd_ = epoll_create1(0);
        if (epollFd_ == -1) {
            throw std::runtime_error("Failed to create epoll instance");
        }

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;  // 边缘触发模式
        ev.data.fd = fd_;

        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd_, &ev) == -1) {
            ::close(epollFd_);
            throw std::runtime_error("Failed to add file descriptor to epoll");
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