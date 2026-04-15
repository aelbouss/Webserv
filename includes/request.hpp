#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sstream>


# ifndef REQUEST_HPP
# define REQUEST_HPP

class Request
{
    public:
        enum Method { GET, POST, DELETE, UNKNOWN };
        enum State
        {
            REQUEST_LINE,
            HEADERS,
            BODY,
            // BODY_CONTENT_LENGTH,
            CHUNK_SIZE,          // Looking for the hex number (e.g., "7\r\n")
            CHUNK_DATA,         // Reading the actual chunk data
            FINISHED,
            ERROR
        };

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
        size_t      _chunkSize;

        bool parseRequestLine();
        bool parseChunkData();
        bool parseChunkSize();
        bool parseHeaders();
        bool parseBody();
};

# endif