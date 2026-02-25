#include "request.hpp"
#include <iostream>

int main()
{
    Request req;

    const char* rawRequest =
        "GET      \t     /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    req.parse(rawRequest, strlen(rawRequest));

    if (req.isFinished())
    {
        std::cout << "Method: " << req.getMethod() << "\n";
        std::cout << "Path: " << req.getPath() << "\n";
        std::cout << "Version: " << req.getVersion() << "\n";
        std::cout << "Body: ";
        const std::vector<char>& body = req.getBody();
        for (size_t i = 0; i < body.size(); ++i) std::cout << body[i];
        std::cout << "\n";
    }
    else
    {
        std::cout << "Request not finished\n";
    }

    return 0;
}