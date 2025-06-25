#include "GPIO.h"

#include <iostream>

#include "ConnectionPool.h"

namespace gpio {
    Session::Session(Connection *conn, boost::asio::io_context &ioContext, int timeout)
        : SessionBase(ioContext, timeout), conn_(conn), ioer_(boost::asio::make_strand(ioContext)), fd_(-1) {
    }

    Session::~Session() {
    }

    bool Session::open(std::string &err, const std::string &portName, IOType iotype) {
        try {
            if (fd_ != -1) {
                close("fd != -1");
            }
            fd_ = ::open(portName.data(), iotype);
            if (fd_ == -1) {
                err = "fd = -1";
                return false;
            }
            portName_ = portName;
            type_ = iotype;
            ioer_.assign(fd_);
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

    bool Session::read() {
        int status = 0;
        std::string statusStr;
        ssize_t len = ::read(fd_, &status, sizeof(status));
        if (len < 0) {
            std::cout << "Failed to read GPIO state" << std::endl;
            conn_->onRead(portName_, statusStr.data(), statusStr.length(), "读取错误");
            return false;
        }
        statusStr = std::to_string(status);
        std::cout << "GPIO read data,portName_:" << portName_ << ",statusStr.data:" << statusStr.data() << std::endl;
        conn_->onRead(portName_, statusStr.data(), statusStr.length(), "");
        return true;
    }

    IOType Session::getType() {
        return type_;
    }

    int Session::getFd() {
        return fd_;
    }

    std::string Session::getPortName() {
        return portName_;
    }

    void Session::asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        // 已经用epooll替代
    }

    void Session::asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        boost::asio::async_write(ioer_, boost::asio::buffer(msg.data(), msg.size()), cb);
    }

    void Session::readHandle(int len, const std::string &error) {
        // 已经用epooll替代
    }

    void Session::writeHandle(const int len, const std::string &error) {
        conn_->onWrite(portName_, len, error);
    }

    void Session::closeSession(const std::string &err) {
        conn_->doClose(portName_, err);
        boost::system::error_code ignored_ec;
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    void Session::timerHandle() {
        conn_->onTimer(portName_);
    }

    GPIO::GPIO(int timeout)
        : ioContext_(net::ConnectionPool::instance().getContext()),
          stop_(false),
          listener_(boost::asio::make_strand(ioContext_)),
          timeout_(timeout),
          epollFd_(-1) {
    }

    GPIO::~GPIO() {
        stop_ = true;
        ::close(epollFd_);
    }

    bool GPIO::add(std::string &err, const std::string &fileName, IOType type) {
        try {
            std::shared_ptr<Session> session = std::make_shared<Session>(this, ioContext_, timeout_);
            if (!session.get()) {
                return false;
            }
            if (!session->open(err, fileName, type)) {
                return false;
            }

            if (type == readOnly || type == readWrite) {
                addEpollNode(session->getFd());
            }
            std::lock_guard<std::mutex> lock(mutex_);
            sessions_[fileName] = session;
            return true;
        } catch (const std::runtime_error &e) {
            err = e.what();
            return false;
        } catch (...) {
            err = "unknow error";
            return false;
        }
    }

    bool GPIO::del(std::string &err, const std::string &fileName) {
        std::shared_ptr<Session> session = std::shared_ptr<Session>();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (sessions_.find(fileName) != sessions_.end()) {
                session = sessions_[fileName];
            }
        }
        if (!session.get()) {
            return false;
        }
        session->close("GPIO::del");
        return true;
    }

    bool GPIO::send(const std::string &fileName, const std::string &data) {
        std::shared_ptr<Session> session = std::shared_ptr<Session>();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (sessions_.find(fileName) != sessions_.end()) {
                session = sessions_[fileName];
            }
        }
        if (!session.get()) {
            return false;
        }
        return session->send(data.data(), data.length());
    }

    void GPIO::doClose(const std::string &portName, const std::string &error) {
        if (!stop_) {
            onClose(portName, error);
        }

        std::shared_ptr<Session> session = std::shared_ptr<Session>();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (sessions_.find(portName) != sessions_.end()) {
                session = sessions_[portName];
            }
        }
        if (!session.get()) {
            return;
        }
        session.reset();
    }

    void GPIO::start() {
        listener_.async_wait(boost::asio::posix::stream_descriptor::wait_read, std::bind(&GPIO::onEvent, this, std::placeholders::_1));
    }

    void GPIO::onEvent(const boost::system::error_code &error) {
        if (stop_) {
            return;
        }
        std::string portName;
        if (!error) {
            struct epoll_event events[100];
            int nums = epoll_wait(epollFd_, events, 100, 0);
            if (nums == -1 || stop_) {
                std::cout << "startAsyncRecv nums = -1" << std::endl;
                return;
            }

            for (int i = 0; i < nums; ++i) {
                int fd = events[i].data.fd;
                std::shared_ptr<Session> session = std::shared_ptr<Session>();
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    for (const auto &s : sessions_) {
                        if (s.second.get() && s.second->getFd() == fd) {
                            session = s.second;
                        }
                    }
                }
                if (!session.get()) {
                    continue;
                }
                int status = 0;
                std::string statusStr;
                if (!session->read()) {
                    session->close("read error!");
                    std::lock_guard<std::mutex> lock(mutex_);
                    sessions_.erase(session->getPortName());
                }
            }
            start();
        } else {
            onListenError(portName, error.message());
        }
    }

    void GPIO::addEpollNode(int fd) {
        if (epollFd_ == -1) {
            epollFd_ = epoll_create1(0);
            if (epollFd_ == -1) {
                throw std::runtime_error("Failed to create epoll instance");
            }
            listener_.assign(epollFd_);
            start();
        }

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;  // 边缘触发模式
        ev.data.fd = fd;

        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
            ::close(epollFd_);
            throw std::runtime_error("Failed to add file descriptor to epoll");
        }
        std::cout << "Add epoll node,fd:" << fd << std::endl;
    }

    void GPIO::delEpollNode(int fd) {
        if (epollFd_ == -1) {
            throw std::runtime_error("Failed to create epoll instance");
        }
        if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
            ::close(epollFd_);
            throw std::runtime_error("Failed to del file descriptor to epoll");
        }
        std::cout << "Del epoll node,fd:" << fd << std::endl;
    }
};  // namespace gpio