#include "ClientSocket.h"

using std::string;
class HttpClientImpl;

class HttpClient : public ClientSocket {
public:
    enum OPTIONS { GET, POST, UPDATE, DELETE };
    HttpClient(const std::string &ip, int port, OPTIONS opt, const std::string &msg)
        : ClientSocket(ip, port, 1), impl_(std::make_unique<HttpClientImpl>(this, opt, msg)) {
        //    start()
    }
    ~HttpClient() {
    }

private:
    virtual void onRead(const std::string &ip, int port, const char *buf, size_t len) override;
    virtual void onWrite(const std::string &ip, int port, string &msg) override;
    virtual void onConnect(const std::string &ip, int port) override;
    virtual void onClose(const std::string &ip, int port, const string &error) override;
    virtual void onTimer(const std::string &ip, int port) override;

public:
    virtual void onResponse(const std::string &resp) = 0;

private:
    std::unique_ptr<HttpClientImpl> impl_;
};
