// #include <iostream>
// #include <thread>

// #include "src/GPIO.h"
// #include "src/Tool.h"

// using namespace gpio;
// using namespace std;

// class GPIOTest : public GPIO {
// public:
//     GPIOTest(int timout = 0) : GPIO(timout) {
//     }
//     ~GPIOTest() = default;

// public:
//     // 读到数据之后
//     virtual void onRead(const std::string &portName, const char *buf, size_t len, const std::string &error) {
//         if (!error.empty()) {
//             // cout << "onRead,error:" << error << endl;
//             return;
//         }
//         cout << "GPIO收到数据,portName:" << portName << ",data:" << std::string(buf, 1) << ",len:" << len << endl;
//     }
//     // 数据写入之后
//     virtual void onWrite(const std::string &portName, const int len, const std::string &error) {
//         if (!error.empty()) {
//             cout << "onWrite,error:" << error << endl;
//         }
//         cout << "GPIO写数据,portName:" << portName << ",len:" << len << endl;
//     }
//     // 连接上之后
//     virtual void onConnect(const std::string &portName, const std::string &error) {
//         if (!error.empty()) {
//             cout << "onConnect,error:" << error << endl;
//         }
//         cout << "GPIO打开成功,portName:" << portName << endl;
//     }
//     // 连接关闭之前
//     virtual void onClose(const std::string &portName, const std::string &error) {
//         if (!error.empty()) {
//             cout << "onClose,error:" << error << endl;
//         }
//         cout << "GPIO关闭成功,portName:" << portName << endl;
//     }
//     // 定时器发生之后
//     virtual void onTimer(const std::string &portName) {
//     }
//     // 监听错误发生之后
//     void onListenError(const std::string &portName, const std::string &error) {
//     }
// };

// int main(int argc, char *argv[]) {
//     std::string name = "/sys/class/gpio-input/DIN3/state";
//     GPIOTest port;
//     std::string error;
//     if (!port.open(error, name)) {
//         cout << "GPIO打开失败,name" << name << ",error:" << error << endl;
//         return 0;
//     }
//     cout << "程序开始,GPIO地址:" << name << endl;

//     while (1) {
//         char ch = getchar();
//         if (ch == 'q') {
//             exit(0);
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     }
// }

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <atomic>
#include <boost/asio.hpp>
#include <iostream>
#include <system_error>
#include <thread>

namespace asio = boost::asio;

class GPIOListener {
public:
    GPIOListener(asio::io_context& io_context, const std::string& gpio_file)
        : io_context_(io_context), gpio_file_(gpio_file), fd_(-1), epoll_fd_(-1) {
        // 打开 GPIO 文件（非阻塞模式）
        fd_ = ::open(gpio_file_.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd_ == -1) {
            std::cerr << "Failed to open GPIO file: " << gpio_file_ << ", error: " << strerror(errno) << std::endl;
            throw std::system_error(errno, std::system_category(), "Failed to open GPIO file");
        }
        std::cout << "GPIO file opened successfully: " << gpio_file_ << std::endl;

        // 创建并配置 epoll 实例（边缘触发模式）
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1) {
            std::cerr << "Failed to create epoll instance, error: " << strerror(errno) << std::endl;
            ::close(fd_);
            throw std::system_error(errno, std::system_category(), "Failed to create epoll");
        }
        std::cout << "epoll instance created successfully" << std::endl;

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;  // 边缘触发
        ev.data.fd = fd_;

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd_, &ev) == -1) {
            std::cerr << "Failed to add fd to epoll, error: " << strerror(errno) << std::endl;
            ::close(epoll_fd_);
            ::close(fd_);
            throw std::system_error(errno, std::system_category(), "Failed to add fd to epoll");
        }
        std::cout << "fd added to epoll successfully" << std::endl;

        // 启动异步监听线程
        startEpollThread();
    }

    ~GPIOListener() {
        stop_ = true;
        if (epoll_thread_.joinable())
            epoll_thread_.join();
        if (epoll_fd_ != -1)
            ::close(epoll_fd_);
        if (fd_ != -1)
            ::close(fd_);
    }

private:
    void startEpollThread() {
        std::cout << "Starting epoll thread" << std::endl;
        epoll_thread_ = std::thread([this]() {
            std::cout << "Epoll thread started" << std::endl;
            while (!stop_) {
                struct epoll_event events[1];
                int n = epoll_wait(epoll_fd_, events, 1, -1);
                if (n == -1) {
                    if (errno == EINTR)
                        continue;  // 被信号中断，继续等待
                    std::cerr << "epoll_wait failed, error: " << strerror(errno) << std::endl;
                    break;
                }

                for (int i = 0; i < n; ++i) {
                    if (events[i].data.fd == fd_) {
                        std::cout << "GPIO event detected" << std::endl;
                        io_context_.post([this]() { handleGPIOEvent(); });
                    }
                }
            }
            std::cout << "Epoll thread exited" << std::endl;
        });
    }

    void handleGPIOEvent() {
        std::cout << "Handling GPIO event" << std::endl;
        char buf[2];
        while (true) {
            lseek(fd_, 0, SEEK_SET);  // 重置文件指针
            ssize_t len = ::read(fd_, buf, sizeof(buf));
            if (len > 0) {
                int value = std::atoi(buf);
                std::cout << "GPIO state changed: " << value << std::endl;
            } else if (len == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;  // 没有更多数据可读
                } else {
                    std::cerr << "Read error: " << strerror(errno) << std::endl;
                    break;
                }
            } else {
                break;  // len == 0（文件关闭？）
            }
        }
    }

private:
    asio::io_context& io_context_;
    std::string gpio_file_;
    int fd_;
    int epoll_fd_;
    std::thread epoll_thread_;
    std::atomic<bool> stop_{false};
};

int main() {
    asio::io_context io_context;
    // 创建 work_guard 保持 io_context 活跃
    auto work_guard = asio::make_work_guard(io_context);  // 关键修改

    try {
        GPIOListener listener(io_context, "/sys/class/gpio-input/DIN3/state");
        std::cout << "Starting io_context.run()" << std::endl;
        io_context.run();  // 此时会持续阻塞，直到 work_guard 被释放
        std::cout << "io_context.run() exited" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}