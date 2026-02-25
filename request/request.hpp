#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sstream>




class Request
{
    public:
        enum Method { GET, POST, DELETE, UNKNOWN };
        enum State { REQUEST_LINE, HEADERS, BODY, FINISHED, ERROR };

        Request();
        void parse(const char* data, size_t size);
        bool isFinished();

        Method getMethod() const;
        const std::string& getPath() const;
        const std::string& getVersion() const;
        const std::vector<char>& getBody() const;
    private:
        Method      _method;
        std::string _path;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::vector<char> _body;
        State       _state;
        std::string _buffer;

        bool parseRequestLine();
        bool parseHeaders();
        bool parseBody();
};

