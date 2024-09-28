#include <iostream>

#include "src/HttpClient.h"
#define LINES "----------------"

int main() {
    HttpClient::GET({"www.example.com", 80, "/", "1.0", 30}, [](const std::string& err, const HttpClient::Response& resp) {
        if (err.empty()) {
            std::cout << LINES << "GET" << LINES << std::endl;
            std::cout << "resp.code:" << resp.code << std::endl;
            std::cout << "resp.massage:" << resp.massage << std::endl;
            std::cout << "resp.type:" << resp.type << std::endl;
            std::cout << "resp.version:" << resp.version << std::endl;
            std::cout << "resp.body:" << resp.body << std::endl;
        }
    });
    HttpClient::POST({"jsonplaceholder.typicode.com", 80, "/posts", "1.0", 30}, {R"({"title": "foo", "body": "bar", "userId": 1})", ""},
                     [](const std::string& err, const HttpClient::Response& resp) {
                         if (err.empty()) {
                             std::cout << LINES << "POST" << LINES << std::endl;
                             std::cout << "resp.code:" << resp.code << std::endl;
                             std::cout << "resp.massage:" << resp.massage << std::endl;
                             std::cout << "resp.type:" << resp.type << std::endl;
                             std::cout << "resp.version:" << resp.version << std::endl;
                             std::cout << "resp.body:" << resp.body << std::endl;
                         }
                     });

    getchar();
}