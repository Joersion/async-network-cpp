#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <queue>

#define IO_BUFFER_MAX_LEN 4096

namespace io {
    class SessionBase : public std::enable_shared_from_this<SessionBase> {
    public:
        SessionBase(boost::asio::io_context &ioContext, int timeout);
        virtual ~SessionBase() = default;

    public:
        void start();
        bool send(const char *msg, size_t len);
        void close(const std::string& err);

    protected:
        // 错误关闭，有默认函数
        virtual bool errorClose(const boost::system::error_code &error);

    protected:
        virtual void asyncRecv(std::function<void(const boost::system::error_code &error, size_t len)> cb) = 0;
        virtual void asyncSend(const std::string &msg, std::function<void(const boost::system::error_code &error, size_t len)> cb) = 0;
        virtual void closeSession(const std::string& err) = 0;
        virtual void readHandle(int len, const std::string &error) = 0;
        virtual void writeHandle(const int len, const std::string &error) = 0;
        virtual void timerHandle() = 0;

    private:
        void doRead(const boost::system::error_code &error, size_t len);
        void doWrite(const boost::system::error_code &error, size_t len);
        void doTimer();
        void startTimer();

    private:
        std::queue<std::string> sendBuf_;
        std::mutex sendLock_;
        std::atomic<bool> isClose_;
        int timeout_;
        boost::asio::deadline_timer timer_;

    protected:
        std::function<void(const boost::system::error_code &error, size_t len)> cbWrite_;
        std::function<void(const boost::system::error_code &error, size_t len)> cbRead_;
    };
};  // namespace io
