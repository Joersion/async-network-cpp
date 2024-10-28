#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>

class HttpClientImpl;
class HttpClient {
public:
    struct RequestOpt {
        // 域名
        std::string host;
        // 端口
        int port;
        // uri路径
        std::string path;
        // http版本
        std::string version;
        // 超时
        int timout;
        // 自定义头部
        std::map<std::string, std::string> headers;
    };

    struct RequestCon {
        // 请求体
        std::string body;
        // 请求内容格式(默认:application/json)
        std::string type;
    };

    struct Response {
        // 状态码
        int code;
        // 状态信息
        std::string massage;
        // 版本号
        int version;
        // 响应内容格式
        std::string type;
        // 响应体
        std::string body;
    };

public:
    HttpClient() = default;
    virtual ~HttpClient() = default;

public:
    static bool GET(const RequestOpt& opt, std::function<void(const std::string&, const Response&)> cb);
    static bool POST(const RequestOpt& opt, const RequestCon& content, std::function<void(const std::string&, const Response&)> cb);
    static bool PUT(const RequestOpt& opt, const RequestCon& content, std::function<void(const std::string&, const Response&)> cb);
    static bool DELETE(const RequestOpt& opt, std::function<void(const std::string&, const Response&)> cb);

public:
    static std::shared_ptr<HttpClientImpl> create();
};