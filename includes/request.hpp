#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sstream>



# ifndef REQUEST_HPP
# define REQUEST_HPP

class BodySink
{
    public:
        enum Result { BODY_OK = 0, BODY_AGAIN = 1, BODY_ERROR = -1 };
        virtual ~BodySink() {}
        virtual int onBodyData(const char* data, size_t len) = 0;
        virtual void onBodyEnd() = 0;
};

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
        void setMaxBodySize(unsigned long n);
        bool hasBodyLimit() const;
        bool hasError() const;
        bool isHeadersComplete() const;
        bool isBodyConfigReady() const;
        void markBodyConfigReady();
        int getErrorCode() const;
        void setErrorCode(int code);
        void setRemoteAddr(const std::string& addr);
        const std::string& getRemoteAddr() const;
        void setBodySink(BodySink* sink);
        void clearBodySink();
        bool isBodyStreaming() const;
        void setUploadResult(const std::string& path, const std::string& url);
        const std::string& getUploadResultPath() const;
        const std::string& getUploadResultUrl() const;
        bool hasUploadResult() const;
        void reset();

        Method getMethod() const;
        std::string getMethodStr() const;
        const std::string& getPath() const;
        const std::string& getQuery() const;
        const std::string& getVersion() const;
        const std::vector<char>& getBody() const;
        const std::map<std::string, std::string>& getHeaders() const;
        std::string getHeader(const std::string& name) const;
    private:
        Method      _method;
        std::string _path;
        std::string _query;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::vector<char> _body;
        State       _state;
        std::string _buffer;
        size_t      _chunkSize;
        unsigned long _maxBodySize;
        bool _hasBodyLimit;
        bool _headersComplete;
        bool _bodyConfigReady;
        int _errorCode;
        std::string _remoteAddr;
        BodySink* _bodySink;
        size_t _bodyReceived;
        unsigned long _contentLength;
        bool _isChunked;
        std::string _uploadResultPath;
        std::string _uploadResultUrl;

        bool parseRequestLine();
        bool parseChunkData();
        bool parseChunkSize();
        bool parseHeaders();
        bool parseBody();
};

# endif