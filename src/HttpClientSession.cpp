#include "HttpClientSession.h"

class HttpClientImpl {
public:
    HttpClientImpl(HttpClientSession* session) : session_(session) {
    }
    ~HttpClientImpl() = default;

private:
    friend class HttpClientSession;

    enum OPTIONS { GET, POST, UPDATE, DELETE };

    struct Packet {
        static void get_request() {
        }
        static void post_request() {
        }
        static void update_request() {
        }
        static void delete_request() {
        }
        static void get_respon() {
        }
        static void post_respon() {
        }
        static void update_respon() {
        }
        static void delete_respon() {
        }
        static void pack(const string& msg, OPTIONS opt, string& data) {
            switch (opt) {
            case GET:
                get_request();
                break;
            case POST:
                post_request();
                break;
            case UPDATE:
                update_request();
                break;
            case DELETE:
                delete_request();
                break;
            default:
                break;
            }
        }
        static void unpack(const string& msg, OPTIONS opt, char* data, int len) {
            switch (opt) {
            case GET:
                get_request();
                break;
            case POST:
                post_request();
                break;
            case UPDATE:
                update_request();
                break;
            case DELETE:
                delete_request();
                break;
            default:
                break;
            }
        }
    };

public:
    void Post(const string& msg) {
        string data;
        Packet::pack(msg, GET, data);
        session_->send(data.data(), data.size());
    }

    void Get() {
        string data;
        Packet::pack("", POST, data);
        session_->send(data.data(), data.size());
    }

    void Update(const string& msg) {
        string data;
        Packet::pack(msg, UPDATE, data);
        session_->send(data.data(), data.size());
    }

    void Delete(const string& msg) {
        string data;
        Packet::pack(msg, DELETE, data);
        session_->send(data.data(), data.size());
    }

private:
    HttpClientSession* session_;
};

void HttpClientSession::GET() {
    impl_->Get();
}
void HttpClientSession::POST(const string& data) {
    impl_->Post(data);
}
void HttpClientSession::UPDATE(const string& msg) {
    impl_->Update("");
}
void HttpClientSession::DELETE(const string& msg) {
    impl_->Delete("");
}