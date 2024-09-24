#include "Session.h"
#include "SessionFactory.h"

using std::string;
class HttpClientImpl;

class HttpClientSession : public Session {
public:
    HttpClientSession(boost::asio::io_context &ioContext, int timeout)
        : Session(ioContext, timeout), impl_(std::make_unique<HttpClientImpl>(this)) {
    }
    ~HttpClientSession() {
    }

public:
    void POST(const string &data);
    void GET();
    void UPDATE(const string &msg);
    void DELETE(const string &msg);

protected:
    virtual void onRead(const char *buf, size_t len) override {
        std::cout << buf << std::endl;
    }
    virtual void onWrite(string &msg) override {
    }
    virtual void onConnect() override {
        std::cout << "连接建立成功,ip: " << ip() << ",port:" << port() << std::endl;
    }
    virtual void onClose(const string &error) override {
        std::cout << "error info:" << error;
    }
    virtual void onTimer() override {
    }

private:
    std::unique_ptr<HttpClientImpl> impl_;
};

class HttpClientSessionFactory : public SessionFactory {
public:
    HttpClientSessionFactory(const string &opt_) {

    };
    virtual ~HttpClientSessionFactory() override {
    }

    virtual std::shared_ptr<Session> create(boost::asio::io_context &ioContext, int timeout = 0) {
        return std::make_shared<HttpClientSession>(ioContext, timeout);
    };
};