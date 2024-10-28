#include "HttpClient.h"

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "ConnectionPool.h"

class HttpClientImpl : public HttpClient, public std::enable_shared_from_this<HttpClientImpl> {
public:
    HttpClientImpl(boost::asio::io_context& io)
        : ioContext_(io), resolver_(boost::asio::make_strand(ioContext_)), stream_(boost::asio::make_strand(ioContext_)) {
    }

    ~HttpClientImpl() {
    }

    HttpClientImpl(const HttpClientImpl& other) = delete;
    HttpClientImpl& operator=(const HttpClientImpl& other) = delete;

public:
    void request(boost::beast::http::verb action, const HttpClient::RequestOpt& opt, const RequestCon& content,
                 std::function<void(const std::string&, const Response&)> cb) {
        int version = (opt.version != "1.0") ? 10 : 11;
        req_.version(version);
        req_.method(action);
        req_.target(opt.path);
        req_.set(boost::beast::http::field::host, opt.host);
        req_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        for (auto custom : opt.headers) {
            req_.set(custom.first, custom.second);
        }
        timeout_ = opt.timout < 5 ? 5 : opt.timout;
        if (!content.body.empty()) {
            req_.body() = content.body;
            std::string contentType = "application/json";
            if (!content.type.empty()) {
                req_.set(boost::beast::http::field::content_type, content.type);
            }
        }
        cb_ = cb;

        resolver_.async_resolve(opt.host, std::to_string(opt.port),
                                boost::beast::bind_front_handler(&HttpClientImpl::resolverHandle, shared_from_this()));
    }

    void resolverHandle(boost::beast::error_code error, boost::asio::ip::tcp::resolver::results_type results) {
        try {
            if (!error) {
                stream_.expires_after(std::chrono::seconds(timeout_));
                stream_.async_connect(results, boost::beast::bind_front_handler(&HttpClientImpl::connectHandle, shared_from_this()));
            } else if (error != boost::asio::error::operation_aborted) {
                cb_(error.what(), {});
            }
        } catch (...) {
            if (cb_) {
                cb_("http resolverHandle run error", {});
            }
        }
    }

    void connectHandle(boost::beast::error_code error, boost::asio::ip::tcp::resolver::results_type::endpoint_type results) {
        try {
            if (!error) {
                stream_.expires_after(std::chrono::seconds(timeout_));
                boost::beast::http::async_write(stream_, req_, boost::beast::bind_front_handler(&HttpClientImpl::writeHandle, shared_from_this()));
            } else if (error != boost::asio::error::operation_aborted) {
                cb_(error.what(), {});
            }
        } catch (...) {
            if (cb_) {
                cb_("http connectHandle run error", {});
            }
        }
    }

    void writeHandle(boost::beast::error_code error, std::size_t len) {
        try {
            boost::ignore_unused(len);
            std::string err;
            if (!error) {
                boost::beast::http::async_read(stream_, buffer_, resp_,
                                               boost::beast::bind_front_handler(&HttpClientImpl::readHandle, shared_from_this()));
            } else if (error != boost::asio::error::operation_aborted) {
                cb_(error.what(), {});
            }
        } catch (...) {
            if (cb_) {
                cb_("http writeHandle run error", {});
            }
        }
    }

    void readHandle(boost::beast::error_code error, std::size_t len) {
        try {
            boost::ignore_unused(len);
            if (!error) {
                Response respData;
                respData.code = static_cast<int>(resp_.result());
                respData.massage = resp_.reason();
                respData.version = static_cast<int>(resp_.version());
                if (resp_.find(boost::beast::http::field::content_type) != resp_.end()) {
                    respData.type = resp_.find(boost::beast::http::field::content_type)->value();
                }
                respData.body = resp_.body();
                cb_("", respData);
            } else if (error != boost::asio::error::operation_aborted) {
                cb_(error.what(), {});
            }
        } catch (...) {
            if (cb_) {
                cb_("http readHandle run error", {});
            }
        }
    }

private:
    boost::asio::io_context& ioContext_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::beast::tcp_stream stream_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::flat_buffer buffer_;
    boost::beast::http::response<boost::beast::http::string_body> resp_;
    std::function<void(const std::string&, const Response&)> cb_;
    int timeout_;
};

std::shared_ptr<HttpClientImpl> HttpClient::create() {
    return std::make_shared<HttpClientImpl>(ConnectionPool::instance().getContext());
}

bool HttpClient::GET(const RequestOpt& opt, std::function<void(const std::string&, const Response&)> cb) {
    auto impl = std::dynamic_pointer_cast<HttpClientImpl>(HttpClient::create());
    if (impl.get()) {
        impl->request(boost::beast::http::verb::get, opt, {}, cb);
        return true;
    }
    return false;
}

bool HttpClient::POST(const RequestOpt& opt, const RequestCon& content, std::function<void(const std::string&, const Response&)> cb) {
    auto impl = std::dynamic_pointer_cast<HttpClientImpl>(HttpClient::create());
    if (impl.get()) {
        impl->request(boost::beast::http::verb::post, opt, content, cb);
        return true;
    }
    return false;
}

bool HttpClient::PUT(const RequestOpt& opt, const RequestCon& content, std::function<void(const std::string&, const Response&)> cb) {
    auto impl = std::dynamic_pointer_cast<HttpClientImpl>(HttpClient::create());
    if (impl.get()) {
        impl->request(boost::beast::http::verb::put, opt, content, cb);
        return true;
    }
    return false;
}

bool HttpClient::DELETE(const RequestOpt& opt, std::function<void(const std::string&, const Response&)> cb) {
    auto impl = std::dynamic_pointer_cast<HttpClientImpl>(HttpClient::create());
    if (impl.get()) {
        impl->request(boost::beast::http::verb::delete_, opt, {}, cb);
        return true;
    }
    return false;
}