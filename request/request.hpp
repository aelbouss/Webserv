#include <iostream>
#include <map>
#include <algorithme>




class request
{
    public:
        enum Method { GET, POST, DELETE, UNKNOWN };
        enum State { REQUEST_LINE, HEADERS, BODY, FINISHED, ERROR };
    private:
        Method      _method;
        std::string _path;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::vector<char> _body;
        State       _state;
        std::string _buffer;
    public:
        void parse(const char* data, size_t size);
        bool isFinished();
};

