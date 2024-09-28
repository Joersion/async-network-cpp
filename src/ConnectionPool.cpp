#include "ConnectionPool.h"

#include <csignal>
#include <iostream>

ConnectionPool& ConnectionPool::instance() {
    static ConnectionPool ins;
    return ins;
}

ConnectionPool::ConnectionPool(int threadNum)
    : work_(std::make_unique<boost::asio::io_context::work>(io_)), signals_(ioSignal_, SIGINT, SIGTERM), isRunning_(true) {
    std::thread t = std::thread([this]() {
        signals_.async_wait([this](const boost::system::error_code& error, int signal_number) {
            try {
                if (!error) {
                    stop();
                }
            } catch (...) {
                stop();
            }
        });
        ioSignal_.run();
    });
    t.detach();

    for (int i = 0; i < threadNum; i++) {
        ioContexts_.emplace_back([this]() {
            try {
                io_.run();
            } catch (...) {
            }
        });
    }
}

ConnectionPool::~ConnectionPool() {
}

void ConnectionPool::stop() {
    isRunning_.store(false);
    io_.stop();
    work_.reset();
    for (int i = 0; i < ioContexts_.size(); i++) {
        ioContexts_[i].join();
    }
}

boost::asio::io_context& ConnectionPool::getContext() {
    return io_;
}