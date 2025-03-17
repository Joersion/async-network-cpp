#include "GPIO.h"

#include <iostream>

#include "ConnectionPool.h"

namespace gpio {
    class Streams {
    public:
        std::shared_ptr<boost::asio::posix::stream_descriptor> get(const std::string &name) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ios_.find(name) != ios_.end()) {
                return ios_[name];
            }
            return std::shared_ptr<boost::asio::posix::stream_descriptor>();
        }

        std::shared_ptr<boost::asio::posix::stream_descriptor> get(int fd) {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto &io : ios_) {
                if (io.second.get() && io.second->native_handle() == fd) {
                    return io.second;
                }
            }
            return std::shared_ptr<boost::asio::posix::stream_descriptor>();
        }

        void add(const std::string &name, int fd, boost::asio::io_context &ioContext) {
            if (exist(name)) {
                del(name);
            }
            std::lock_guard<std::mutex> lock(mutex_);
            ios_[name] = std::make_shared<boost::asio::posix::stream_descriptor>(boost::asio::make_strand(ioContext), fd);
        }

        bool exist(const std::string &name) {
            std::lock_guard<std::mutex> lock(mutex_);
            return ios_.find(name) != ios_.end();
        }

        void del(const std::string &name) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ios_.find(name) != ios_.end()) {
                ::close(ios_[name]->native_handle());
            }
        }

        void del(int fd) {
            std::string name = "";
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (const auto &io : ios_) {
                    if (io.second.get() && io.second->native_handle() == fd) {
                        name = io.first;
                        break;
                    }
                }
            }
            del(name);
        }

        std::string getName(int fd) {
            std::string name = "";
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (const auto &io : ios_) {
                    if (io.second.get() && io.second->native_handle() == fd) {
                        name = io.first;
                        break;
                    }
                }
            }
            return name;
        }

        void closeAll() {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto &io : ios_) {
                ::close(io.second->native_handle());
            }
            ios_.clear();
        }

        IOType getType(const std::string &name) {
            IOType type = knowType;
            std::lock_guard<std::mutex> lock(mutex_);
            if (ios_.find(name) != ios_.end()) {
                if (ios_[name].get()) {
                    int fd = ios_[name]->native_handle();
                    int flags = fcntl(fd, F_GETFL);
                    if (flags != -1) {
                        if (flags & O_RDONLY) {
                            type = readOnly;
                        } else if (flags & O_WRONLY) {
                            type = writeOnly;
                        } else if (flags & O_RDWR) {
                            type = readWrite;
                        }
                    }
                }
            }
            return type;
        }

    private:
        std::map<std::string, std::shared_ptr<boost::asio::posix::stream_descriptor>> ios_;
        std::mutex mutex_;
    };

    Session::Session(Connection *conn, boost::asio::io_context &ioContext, int timeout)
        : SessionBase(ioContext, timeout),
          conn_(conn),
          epollFd_(-1),
          ioContext_(ioContext),
          listener_(boost::asio::make_strand(ioContext)),
          streams_(new Streams) {
        try {
            epollFd_ = epoll_create1(0);
            if (epollFd_ == -1) {
                throw std::runtime_error("Failed to create epoll instance");
            }
        } catch (const std::runtime_error &e) {
            conn_->onListenError("", "init epoll error, epollFd_: -1");
        } catch (...) {
            conn_->onListenError("", "init epoll error");
        }
    }

    Session::~Session() {
        if (streams_ != nullptr) {
            delete streams_;
        }
    }

    bool Session::add(std::string &err, const std::string &portName, IOType iotype) {
        try {
            if (!listener_.is_open()) {
                listener_.assign(epollFd_);
                start();
            }
            int fd = ::open(portName.data(), iotype);
            if (fd == -1) {
                err = "fd = -1";
                return false;
            }
            streams_->add(portName, fd, ioContext_);
            if (iotype == readOnly || iotype == readWrite) {
                setEdgeTriggered(fd);
            }
            return true;
        } catch (const boost::system::system_error &e) {
            err = e.what();
            return false;
        } catch (const std::runtime_error &e) {
            err = e.what();
            return false;
        }
    }

    bool Session::del(std::string &err, const std::string &portName) {
        try {
            if (streams_->exist(portName)) {
                err = "portName not open, portName:" + portName;
                return false;
            }
            std::shared_ptr<boost::asio::posix::stream_descriptor> desc = streams_->get(portName);
            if (!desc.get()) {
                err = "desc is empty, portName:" + portName;
                return false;
            }
            delEpollCtl(desc->native_handle());
            streams_->del(portName);
            return true;
        } catch (const boost::system::system_error &e) {
            err = e.what();
            return false;
        } catch (const std::runtime_error &e) {
            err = e.what();
            return false;
        }
    }

    void Session::asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        listener_.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                             std::bind(&Session::startAsyncRecv, std::static_pointer_cast<Session>(shared_from_this()), std::placeholders::_1));
    }

    void Session::asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) {
        boost::asio::async_write(listener_, boost::asio::buffer(msg.data(), msg.size()), cb);
    }

    void Session::readHandle(int len, const std::string &error) {
        // conn_->onRead(portName_, recvBuf_, len, error);
    }

    void Session::writeHandle(const int len, const std::string &error) {
        conn_->onWrite("", len, error);
    }

    void Session::closeSession() {
        conn_->doClose("", "");
        boost::system::error_code ignored_ec;
        streams_->closeAll();
        if (epollFd_ != -1) {
            ::close(epollFd_);
        }
    }

    void Session::timerHandle() {
        conn_->onTimer("");
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
        std::string portName;
        if (!error) {
            struct epoll_event events[100];
            int nums = epoll_wait(epollFd_, events, 100, 0);
            if (nums == -1) {
                std::cout << "startAsyncRecv nums = -1" << std::endl;
                return;
            }

            for (int i = 0; i < nums; ++i) {
                int fd = events[i].data.fd;  // 获取触发事件的文件描述符
                portName = streams_->getName(fd);
                auto desc = streams_->get(portName);
                if (!desc.get()) {
                    continue;
                }
                int status = 0;
                std::string statusStr;
                ssize_t len = ::read(desc->native_handle(), &status, sizeof(status));
                if (len == 0) {
                    statusStr = std::to_string(status);
                    conn_->onRead(portName, statusStr.data(), statusStr.length(), "");
                } else if (len == -1) {
                    std::cout << "Failed to read GPIO state" << std::endl;
                    conn_->onRead(portName, statusStr.data(), statusStr.length(), "读取错误");
                    streams_->del(portName);
                }
            }
            asyncRecv(nullptr);
        } else {
            conn_->onListenError(portName, error.message());
            close();
        }
    }

    void Session::setEdgeTriggered(int fd) {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;  // 边缘触发模式
        ev.data.fd = fd;

        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
            ::close(epollFd_);
            throw std::runtime_error("Failed to add file descriptor to epoll");
        }
    }

    void Session::delEpollCtl(int fd) {
        if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
            perror("epoll_ctl EPOLL_CTL_DEL");
            ::close(epollFd_);
            throw std::runtime_error("Failed to del file descriptor to epoll");
        }
    }

    GPIO::GPIO(int timeout)
        : ioContext_(net::ConnectionPool::instance().getContext()), session_(std::make_shared<Session>(this, ioContext_, timeout)), stop_(false) {
    }

    GPIO::~GPIO() {
        stop_ = true;
    }

    bool GPIO::add(std::string &err, const std::string &fileName, IOType type) {
        return session_->add(err, fileName, type);
    }

    bool GPIO::del(std::string &err, const std::string &fileName) {
        return session_->del(err, fileName);
    }

    bool GPIO::send(const std::string &data) {
        if (session_.get()) {
            if (session_->getStatus() == readOnly) {
                std::cout << "file is read only." << std::endl;
                return false;
            }
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