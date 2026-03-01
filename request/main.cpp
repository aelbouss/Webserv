#include "request.hpp"
#include <iostream>
#include <cstring>

int main()
{
    Request req;

    const char* rawRequest1 =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    req.parse(rawRequest1, strlen(rawRequest1));

    if (req.isFinished())
    {
        std::cout << "=== Normal Body ===\n";
        std::cout << "Method: " << req.getMethod() << "\n";
        std::cout << "Path: " << req.getPath() << "\n";
        std::cout << "Version: " << req.getVersion() << "\n";
        std::cout << "Body: ";
        const std::vector<char>& body = req.getBody();
        for (size_t i = 0; i < body.size(); ++i) std::cout << body[i];
        std::cout << "\n\n";
    }
    else
    {
        std::cout << "Request 1 not finished\n";
    }

    // Example 2: Chunked body
    Request chunkedReq;
    const char* rawRequest2 =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "4\r\n"
        "Wiki\r\n"
        "5\r\n"
        "pedia\r\n"
        "0\r\n"
        "\r\n";

    chunkedReq.parse(rawRequest2, strlen(rawRequest2));

    if (chunkedReq.isFinished())
    {
        std::cout << "=== Chunked Body ===\n";
        std::cout << "Method: " << chunkedReq.getMethod() << "\n";
        std::cout << "Path: " << chunkedReq.getPath() << "\n";
        std::cout << "Version: " << chunkedReq.getVersion() << "\n";
        std::cout << "Body: ";
        const std::vector<char>& body2 = chunkedReq.getBody();
        for (size_t i = 0; i < body2.size(); ++i) std::cout << body2[i];
        std::cout << "\n";
    }
    else
    {
        std::cout << "Chunked request not finished\n";
    }

    return 0;
}