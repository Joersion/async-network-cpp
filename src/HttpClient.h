#pragma once

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

class HttpClient {
public:
    struct RequestOpt {
        std::string host;
        int port;
        std::string path;
        std::string version;
        struct {
            std::string type;
            std::string body;
        } content;
    };

    struct RequestCon {
        std::string body;
        std::string type;
    };

    HttpClient() {
    }
    virtual ~HttpClient() = default;

public:
    virtual void onError(const std::string& error) {
    }
    virtual void onResopne(const boost::beast::flat_buffer& buffer, const boost::beast::http::response<boost::beast::http::string_body> resp) {
    }

public:
    static void GET(const RequestOpt& opt);
    static void POST(const RequestOpt& opt, const RequestCon& content);
    static void PUT(const RequestOpt& opt, const RequestCon& content);
    static void DELETE(const RequestOpt& opt);

    static std::shared_ptr<HttpClient> create();
};