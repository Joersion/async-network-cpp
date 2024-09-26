#include "HttpClientSession.h"

class HttpClientImpl {
public:
    HttpClientImpl(HttpClient* cli, HttpClient::OPTIONS opt, const std::string& msg)
        : cli_(cli), opt_(opt), requestMsg_(msg) {
    }
    ~HttpClientImpl() = default;

private:
    friend class HttpClient;

    struct Packet {
        HttpClient::OPTIONS opt;
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
            case HttpClient::OPTIONS::GET:
                get_request(msg, data);
                break;
            case HttpClient::OPTIONS::POST:
                post_request();
                break;
            case HttpClient::OPTIONS::UPDATE:
                update_request();
                break;
            case HttpClient::OPTIONS::DELETE:
                delete_request();
                break;
            default:
                break;
            }
        }
        void unpack(const string& msg, char* data, int len) {
            switch (opt) {
            case HttpClient::OPTIONS::GET:
                // get_request();
                break;
            case HttpClient::OPTIONS::POST:
                post_request();
                break;
            case HttpClient::OPTIONS::UPDATE:
                update_request();
                break;
            case HttpClient::OPTIONS::DELETE:
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
    HttpClient::OPTIONS opt_;
    std::string requestMsg_;
    HttpClient* cli_;
};

// void HttpClientSession::GET() {
//     impl_->Get();
// }
// void HttpClientSession::POST(const string& data) {
//     impl_->Post(data);
// }
// void HttpClientSession::UPDATE(const string& msg) {
//     impl_->Update("");
// }
// void HttpClientSession::DELETE(const string& msg) {
//     impl_->Delete("");
// }