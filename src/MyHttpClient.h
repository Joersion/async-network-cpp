#include "ClientConnection.h"

using std::string;
class MyHttpClientImpl;

class MyHttpClient : public ClientConnection {
public:
    enum OPTIONS { GET, POST, UPDATE, DELETE };
    MyHttpClient(const std::string &ip, int port, OPTIONS opt, const std::string &msg)
        : ClientConnection(ip, port, 1), impl_(std::make_unique<MyHttpClientImpl>(this, opt, msg)) {
        //    start()
    }
    ~MyHttpClient() {
    }

private:
    virtual void onRead(const std::string &ip, int port, const char *buf, size_t len, const std::string &error) override;
    virtual void onWrite(const std::string &ip, int port, const string &msg, const std::string &error) override;
    virtual void onConnect(const std::string &ip, int port, const std::string &error) override;
    virtual void onClose(const std::string &ip, int port, const std::string &error) override;
    virtual void onTimer(const std::string &ip, int port) override;

public:
    virtual void onResponse(const std::string &resp) = 0;

private:
    std::unique_ptr<MyHttpClientImpl> impl_;
};
