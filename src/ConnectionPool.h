#pragma once
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>
#include <vector>

namespace net {
    class ConnectionPool {
    public:
        static ConnectionPool& instance();

    public:
        void stop();

    private:
        ConnectionPool(int threadNum = std::thread::hardware_concurrency());
        ~ConnectionPool();
        ConnectionPool(const ConnectionPool& other) = delete;
        ConnectionPool& operator=(const ConnectionPool& other) = delete;

    public:
        boost::asio::io_context& getContext();

    private:
        std::atomic<bool> isRunning_;
        std::vector<std::thread> ioContexts_;
        boost::asio::io_context io_;
        boost::asio::io_context ioSignal_;
        boost::asio::signal_set signals_;
        std::unique_ptr<boost::asio::io_context::work> work_;
    };
};  // namespace net
