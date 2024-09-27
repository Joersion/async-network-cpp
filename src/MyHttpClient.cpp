#include "MyHttpClient.h"

class MyHttpClientImpl {
public:
    MyHttpClientImpl(MyHttpClient* cli, MyHttpClient::OPTIONS opt, const std::string& msg)
        : cli_(cli), opt_(opt), requestMsg_(msg) {
    }
    ~MyHttpClientImpl() = default;

private:
    friend class HttpClient;

    struct Packet {
        MyHttpClient::OPTIONS opt;
        void get_request(const string& msg, string& data) {
        }
        void post_request() {
        }
        void update_request() {
        }
        void delete_request() {
        }
        void get_respon() {
        }
        void post_respon() {
        }
        void update_respon() {
        }
        void delete_respon() {
        }
        void pack(const string& msg, string& data) {
            switch (opt) {
            case MyHttpClient::OPTIONS::GET:
                get_request(msg, data);
                break;
            case MyHttpClient::OPTIONS::POST:
                post_request();
                break;
            case MyHttpClient::OPTIONS::UPDATE:
                update_request();
                break;
            case MyHttpClient::OPTIONS::DELETE:
                delete_request();
                break;
            default:
                break;
            }
        }
        void unpack(const string& msg, char* data, int len) {
            switch (opt) {
            case MyHttpClient::OPTIONS::GET:
                // get_request();
                break;
            case MyHttpClient::OPTIONS::POST:
                post_request();
                break;
            case MyHttpClient::OPTIONS::UPDATE:
                update_request();
                break;
            case MyHttpClient::OPTIONS::DELETE:
                delete_request();
                break;
            default:
                break;
            }
        }
    };

public:
    void doRequest() {
        string data;
        Packet pack;
        pack.opt = opt_;
        pack.pack(requestMsg_, data);
        cli_->send(data);
    }

    void doResponse(const std::string& msg) {
        string resp;
        Packet pack;
        pack.opt = opt_;
        pack.pack(msg, resp);
        cli_->onResponse(resp);
    }

private:
    MyHttpClient::OPTIONS opt_;
    std::string requestMsg_;
    MyHttpClient* cli_;
};
void MyHttpClient::onRead(const std::string& ip, int port, const char* buf, size_t len, const std::string& error) {
    if (!error.empty()) {
        return;
    }
    std::cout << buf << std::endl;
    std::string msg(buf, len);
    impl_->doResponse(msg);
}
void MyHttpClient::onWrite(const std::string& ip, int port, const string& msg, const std::string& error) {
    if (!error.empty()) {
        return;
    }
}
void MyHttpClient::onConnect(const std::string& ip, int port, const std::string& error) {
    if (!error.empty()) {
        return;
    }
    impl_->doRequest();
}
void MyHttpClient::onClose(const std::string& ip, int port, const string& error) {
    if (!error.empty()) {
        return;
    }
}
void MyHttpClient::onTimer(const std::string& ip, int port) {
}