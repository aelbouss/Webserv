#include <iostream>
#include <map>
#include <algorithm>




class Request
{
    private:
        Method      _method;
        std::string _path;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::vector<char> _body;
        State       _state;
        std::string _buffer;

    public:
        request();
        enum Method { GET, POST, DELETE, UNKNOWN };
        enum State { REQUEST_LINE, HEADERS, BODY, FINISHED, ERROR };

        void parse(const char* data, size_t size);
        bool isFinished();
        
};

