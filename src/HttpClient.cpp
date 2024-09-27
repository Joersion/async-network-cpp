#include "HttpClient.h"

#include <iostream>
#include <thread>

class HttpClientImpl : public HttpClient, public std::enable_shared_from_this<HttpClientImpl> {
public:
    HttpClientImpl() : ioContext_(), resolver_(boost::asio::make_strand(ioContext_)), stream_(boost::asio::make_strand(ioContext_)) {
    }

    void start(boost::beast::http::verb action, const HttpClient::RequestOpt& opt, const RequestCon& content) {
        int version = (opt.version != "1.0") ? 10 : 11;
        req_.version(version);
        req_.method(action);
        req_.target(opt.path);
        req_.set(boost::beast::http::field::host, opt.host);
        req_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        if (!content.body.empty()) {
            req_.body() = content.body;
            std::string contentType = "application/json";
            if (!content.type.empty()) {
                req_.set(boost::beast::http::field::content_type, content.type);
            }
        }

        resolver_.async_resolve(opt.host, std::to_string(opt.port),
                                boost::beast::bind_front_handler(&HttpClientImpl::resolverHandle, shared_from_this()));

        std::thread t(&HttpClientImpl::run, shared_from_this());
        t.detach();
    }

    void resolverHandle(boost::beast::error_code error, boost::asio::ip::tcp::resolver::results_type results) {
        if (!error) {
            stream_.expires_after(std::chrono::seconds(30));
            stream_.async_connect(results, boost::beast::bind_front_handler(&HttpClientImpl::connectHandle, shared_from_this()));
        } else {
            onError(error.what());
        }
    }

    void connectHandle(boost::beast::error_code error, boost::asio::ip::tcp::resolver::results_type::endpoint_type results) {
        if (!error) {
            stream_.expires_after(std::chrono::seconds(30));
            boost::beast::http::async_write(stream_, req_, boost::beast::bind_front_handler(&HttpClientImpl::writeHandle, shared_from_this()));
        } else {
            onError(error.what());
        }
    }

    void writeHandle(boost::beast::error_code error, std::size_t len) {
        boost::ignore_unused(len);
        std::string err;
        if (!error) {
            boost::beast::http::async_read(stream_, buffer_, resp_,
                                           boost::beast::bind_front_handler(&HttpClientImpl::readHandle, shared_from_this()));
        } else {
            onError(error.what());
        }
    }

    void readHandle(boost::beast::error_code error, std::size_t len) {
        boost::ignore_unused(len);
        if (!error) {
            stream_.expires_after(std::chrono::seconds(30));
            onResopne(buffer_, resp_);
        } else {
            onError(error.what());
        }
    }

private:
    void run() {
        try {
            ioContext_.run();
            std::cout << "iocontext exit" << std::endl;
        } catch (...) {
            onError("http iocontext run error");
        }
    }

private:
    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::beast::tcp_stream stream_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::flat_buffer buffer_;
    boost::beast::http::response<boost::beast::http::string_body> resp_;
};

std::shared_ptr<HttpClient> HttpClient::create() {
    return std::make_shared<HttpClientImpl>();
}

void HttpClient::GET(const RequestOpt& opt) {
    std::shared_ptr<HttpClientImpl> impl = std::dynamic_pointer_cast<HttpClientImpl>(create());
    impl->start(boost::beast::http::verb::get, opt, {});
}

void HttpClient::POST(const RequestOpt& opt, const RequestCon& content) {
    std::shared_ptr<HttpClientImpl> impl = std::dynamic_pointer_cast<HttpClientImpl>(create());
    impl->start(boost::beast::http::verb::post, opt, content);
}

void HttpClient::PUT(const RequestOpt& opt, const RequestCon& content) {
    std::shared_ptr<HttpClientImpl> impl = std::dynamic_pointer_cast<HttpClientImpl>(create());
    impl->start(boost::beast::http::verb::put, opt, content);
}

void HttpClient::DELETE(const RequestOpt& opt) {
    std::shared_ptr<HttpClientImpl> impl = std::dynamic_pointer_cast<HttpClientImpl>(create());
    impl->start(boost::beast::http::verb::delete_, opt, {});
}