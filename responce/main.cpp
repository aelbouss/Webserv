#include "response.cpp"
#include <iostream>
#include <cstring>


int main()
{
    {
        Response r;
        r.setStatus(200);
        r.setHeader("Content-Type", "text/html");
        r.setHeader("X-Custom", "webserv");
        r.setBody("<html><body><h1>Hello!</h1></body></html>");
        std::cout << r.toString();
    }


    {
        Response r(404, "Not found here", "text/plain");
        std::cout << r.toString();
    }


    {
        Response r;
        r.serveFile("www/index.html");
        std::cout << r.toString();
    }


    {
        Response r;
        r.serveFile("www/style.css");
        std::cout << r.toString();
    }


    {
        Response r;
        r.serveFile("www/does_not_exist.html");
        std::cout << r.toString();
    }

    
    {
        Response r;
        std::vector<char> empty;
        r.build("GET", "/index.html", empty, "./www");
        std::cout << r.toString();
    }


    {
        Response r;
        std::vector<char> empty;
        r.build("GET", "/ghost.html", empty, "./www");
        std::cout << r.toString();
    }

    {
        Response r;
        std::string bodyStr = "name=anass&age=21";
        std::vector<char> body(bodyStr.begin(), bodyStr.end());
        r.build("POST", "/submit", body, "./www");
        std::cout << r.toString();
    }

    {
        // Create a temp file first
        std::ofstream tmp("www/tmp_file.txt");
        tmp << "delete me";
        tmp.close();

        Response r;
        std::vector<char> empty;
        r.build("DELETE", "/tmp_file.txt", empty, "./www");
        std::cout << r.toString();
    }

    {
        Response r;
        std::vector<char> empty;
        r.build("DELETE", "/already_gone.txt", empty, "./www");
        std::cout << r.toString();
    }

    
    {
        Response r;
        std::vector<char> empty;
        r.build("PUT", "/index.html", empty, "./www");
        std::cout << r.toString();
    }


    {
        Response r;
        r.setStatus(200);
        r.setHeader("Content-Type", "application/octet-stream");
        std::vector<char> bin;
        for (int i = 0; i < 8; i++) bin.push_back((char)(i * 10));
        r.setBody(bin);
        std::cout << "Status: " << r.getStatusCode()
                  << " " << r.getStatusMessage() << "\n";
        std::cout << "Body size: " << r.getBody().size() << " bytes\n";
    }

    
    {
        Response r;
        r.setStatus(403);
        r.setBody("Forbidden zone");
        std::cout << "Code:    " << r.getStatusCode()    << "\n";
        std::cout << "Message: " << r.getStatusMessage() << "\n";
        std::cout << "Body:    " << r.getBody()          << "\n";
    }

    std::cout << "\n✓ All tests passed.\n";
    return 0;
}