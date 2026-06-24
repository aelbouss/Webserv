#include "../includes/request.hpp"
#include <cstdlib>

static std::string toLowerCopy(const std::string& value)
{
    std::string out = value;

    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

static bool parseContentLength(const std::string& value, unsigned long& out)
{
    char *end_ptr = NULL;
    unsigned long parsed = std::strtoul(value.c_str(), &end_ptr, 10);
    if (end_ptr == NULL || *end_ptr != '\0')
        return false;
    out = parsed;
    return true;
}

Request::Request()
        : _method(UNKNOWN), _state(REQUEST_LINE), _chunkSize(0), _maxBodySize(0),
            _hasBodyLimit(false), _headersComplete(false), _bodyConfigReady(false),
            _errorCode(0), _remoteAddr(""), _bodySink(NULL), _bodyReceived(0),
            _contentLength(0), _isChunked(false), _uploadResultPath(""), _uploadResultUrl("") {}

void Request::setMaxBodySize(unsigned long n)
{
    _maxBodySize = n;
    _hasBodyLimit = (n != 0);

    if (!_hasBodyLimit)
        return;

    // If headers were already parsed, enforce the limit immediately.
    if (_headersComplete)
    {
        if (_state == BODY)
        {
            unsigned long contentLength = 0;
            std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");
            if (it != _headers.end() && parseContentLength(it->second, contentLength))
            {
                if (_maxBodySize > 0 && contentLength > _maxBodySize)
                {
                    _errorCode = 413;
                    _state = ERROR;
                }
            }
        }
        else if (_state == CHUNK_SIZE || _state == CHUNK_DATA)
        {
            size_t current = _bodySink ? _bodyReceived : _body.size();
            if (_maxBodySize > 0 && current > _maxBodySize)
            {
                _errorCode = 413;
                _state = ERROR;
            }
        }
        else if (_state == FINISHED)
        {
            size_t current = _bodySink ? _bodyReceived : _body.size();
            if (_maxBodySize > 0 && current > _maxBodySize)
            {
                _errorCode = 413;
                _state = ERROR;
            }
        }
    }
}

void Request::reset()
{
    _method = UNKNOWN;
    _path.clear();
    _query.clear();
    _version.clear();
    _headers.clear();
    _body.clear();
    _state = REQUEST_LINE;
    _buffer.clear();
    _chunkSize = 0;
    _maxBodySize = 0;
    _hasBodyLimit = false;
    _headersComplete = false;
    _bodyConfigReady = false;
    _errorCode = 0;
    _remoteAddr.clear();
    _bodySink = NULL;
    _bodyReceived = 0;
    _contentLength = 0;
    _isChunked = false;
    _uploadResultPath.clear();
    _uploadResultUrl.clear();
}

bool Request::hasBodyLimit() const
{
    return _hasBodyLimit;
}

bool Request::hasError() const
{
    return _state == ERROR || _errorCode != 0;
}

bool Request::isHeadersComplete() const
{
    return _headersComplete;
}

bool Request::isBodyConfigReady() const
{
    return _bodyConfigReady;
}

void Request::markBodyConfigReady()
{
    _bodyConfigReady = true;
}

int Request::getErrorCode() const
{
    return _errorCode;
}

void Request::setErrorCode(int code)
{
    _errorCode = code;
    _state = ERROR;
}

void Request::setRemoteAddr(const std::string& addr)
{
    _remoteAddr = addr;
}

const std::string& Request::getRemoteAddr() const
{
    return _remoteAddr;
}

void Request::setBodySink(BodySink* sink)
{
    _bodySink = sink;
}

void Request::clearBodySink()
{
    _bodySink = NULL;
}

bool Request::isBodyStreaming() const
{
    return _bodySink != NULL;
}

void Request::setUploadResult(const std::string& path, const std::string& url)
{
    _uploadResultPath = path;
    _uploadResultUrl = url;
}

const std::string& Request::getUploadResultPath() const
{
    return _uploadResultPath;
}

const std::string& Request::getUploadResultUrl() const
{
    return _uploadResultUrl;
}

bool Request::hasUploadResult() const
{
    return !_uploadResultUrl.empty();
}
Request::Method Request::getMethod() const { return _method; }
std::string Request::getMethodStr() const
{
    if (_method == GET)
        return "GET";
    if (_method == POST)
        return "POST";
    if (_method == DELETE)
        return "DELETE";
    return "UNKNOWN";
}
const std::string& Request::getPath() const { return _path; }
const std::string& Request::getQuery() const { return _query; }
const std::string& Request::getVersion() const { return _version; }
const std::vector<char>& Request::getBody() const { return _body; }
const std::map<std::string, std::string>& Request::getHeaders() const { return _headers; }

std::string Request::getHeader(const std::string& name) const
{
    std::string key = toLowerCopy(name);
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);

    if (it != _headers.end())
        return it->second;
    return "";
}

void Request::parse(const char* data, size_t size)
{
    _buffer.append(data, size);

    while (1)
    {
       // std::cout << _buffer << std::endl; just to test with
        if (_state == REQUEST_LINE && !parseRequestLine()) return;
        if (_state == HEADERS && !parseHeaders()) return;
        if (_headersComplete && !_bodyConfigReady && (_state == BODY || _state == CHUNK_SIZE)) return;
        if (_state == CHUNK_SIZE && !parseChunkSize()) return;
        if (_state == CHUNK_DATA && !parseChunkData()) return;
        if (_state == BODY && !parseBody()) return;
        if (_state == FINISHED || _state == ERROR) return;

    }
}

bool Request::parseRequestLine()
{
    //_buffer is varible that store whole request content it comes as segment

    // any line ended by /r/n
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos)
        return false;

    //extract line from _buffer
    std::string line = _buffer.substr(0,pos);
    // remove line from _buffer, because next time should be work with next line
    _buffer.erase(0, pos + 2);


    /* split firsat line
    GET SP /home SP http1.1
    why use istingstram instead of  split becuse should be implemnent own split to hundel all white spaces 
    so i use istingstram that implemnt this safe and naturly like cin !!!

    !!!
        if (!(...)) because when >> fail return object itsaelf (false state)
            _state = ERROR
    */
    
    std::istringstream iss(line);
    std::string methodStr;
    if (!(iss >> methodStr >> _path >> _version)) {
        _state = ERROR;
        return false;
    }

    //methodStr that store first word from request line
    if (methodStr == "GET")
        _method = GET;
    else if (methodStr == "POST")
        _method = POST;
    else if (methodStr == "DELETE")
        _method = DELETE;
    else
        _method = UNKNOWN;

    size_t queryPos = _path.find('?');
    if (queryPos != std::string::npos)
    {
        _query = _path.substr(queryPos + 1);
        _path = _path.substr(0, queryPos);
    }
    else
        _query.clear();

    // next state
    _state = HEADERS;
    return true;
}

bool Request::parseHeaders()
{
    while (true)
    {
        size_t pos = _buffer.find("\r\n");
        if (pos == std::string::npos)
            return false;
        std::string line = _buffer.substr(0,pos);
        _buffer.erase(0, pos + 2);

        //If line is empty -> headers finished
        if (line.empty())
        {
            _headersComplete = true;
            _bodyReceived = 0;
            _contentLength = 0;
            _isChunked = false;
            // Check transfer-encoding first (headers are normalized to lowercase).
            if (_headers.count("transfer-encoding") &&
                toLowerCopy(_headers["transfer-encoding"]).find("chunked") != std::string::npos)
            {
                _isChunked = true;
                _state = CHUNK_SIZE; 
            }
            // if there is content length that's mean we have a body else finish baecause no body
            else if (_headers.count("content-length"))
            {
                unsigned long contentLength = 0;
                if (!parseContentLength(_headers["content-length"], contentLength))
                {
                    _errorCode = 400;
                    _state = ERROR;
                    return false;
                }
                _contentLength = contentLength;
                if (_hasBodyLimit && _maxBodySize > 0 && contentLength > _maxBodySize)
                {
                    _errorCode = 413;
                    _state = ERROR;
                    return false;
                }
                _state = BODY;
            }
            else
                _state = FINISHED;
            return true;
        }
        /*
            header store as key:value
            so we search for ':' to make to prats before ':' and after
            before is a key and after is a value

        */
        size_t colon = line.find(':');
        if (colon == std::string::npos)
        {
            _errorCode = 400;
            _state = ERROR;
            return false;
        }
        std::string key = line.substr(0,colon);//strore before :
        std::string value = line.substr(colon + 1);//strore after :
        // remove first char because usually after ':' we find ' ' so we skip it
        if (!value.empty() && value[0] == ' ')
            value.erase(0, 1);
        key = toLowerCopy(key);
        //store 
        _headers[key] = value;
    }
}

bool Request::parseChunkSize()
{
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos)
        return false;

    std::string sizeStr = _buffer.substr(0, pos);
    _buffer.erase(0, pos + 2);

    std::stringstream ss;
    ss << std::hex << sizeStr;
    ss >> _chunkSize;

    if (ss.fail())
    {
        _errorCode = 400;
        _state = ERROR;
        return false;
    }

    if (_chunkSize == 0)
    {
        _state = FINISHED;
        if (_bodySink)
            _bodySink->onBodyEnd();
        return true;
    }

    size_t current = _bodySink ? _bodyReceived : _body.size();
    if (_hasBodyLimit && _maxBodySize > 0 && current + _chunkSize > _maxBodySize)
    {
        _errorCode = 413;
        _state = ERROR;
        return false;
    }

    _state = CHUNK_DATA;
    return true;
}


bool Request::parseChunkData()
{
    if (_buffer.size() < _chunkSize + 2)
        return false;

    // Enforce max body size if configured
    size_t current = _bodySink ? _bodyReceived : _body.size();
    if (_hasBodyLimit && _maxBodySize > 0 && current + _chunkSize > _maxBodySize)
    {
        _errorCode = 413; // Payload Too Large
        _state = ERROR;
        return false;
    }

    if (_bodySink)
    {
        int res = _bodySink->onBodyData(_buffer.data(), _chunkSize);
        if (res == BodySink::BODY_AGAIN)
            return false;
        if (res != BodySink::BODY_OK)
        {
            _errorCode = (res < 0 ? -res : 500);
            _state = ERROR;
            return false;
        }
    }
    else
    {
        _body.insert(_body.end(), _buffer.begin(), _buffer.begin() + _chunkSize);
    }
    _bodyReceived += _chunkSize;
    _buffer.erase(0, _chunkSize + 2);

    _state = CHUNK_SIZE;
    return true;
}

bool Request::parseBody()
{
    // Content-Length lookup uses normalized lowercase keys.
    std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");
    if (it == _headers.end())
    {
        _errorCode = 400;
        _state = ERROR;
        return false;
    }

    unsigned long contentLength = 0;
    if (!parseContentLength(it->second, contentLength))
    {
        _errorCode = 400;
        _state = ERROR;
        return false;
    }

    if (_bodyReceived >= contentLength)
    {
        _state = FINISHED;
        if (_bodySink)
            _bodySink->onBodyEnd();
        return true;
    }

    if (_buffer.empty())
        return false;

    size_t remaining = contentLength - _bodyReceived;
    size_t toConsume = _buffer.size() < remaining ? _buffer.size() : remaining;
    // Enforce max body size if configured
    if (_hasBodyLimit && _maxBodySize > 0 && _bodyReceived + toConsume > _maxBodySize)
    {
        _errorCode = 413;
        _state = ERROR;
        return false;
    }

    if (_bodySink)
    {
        int res = _bodySink->onBodyData(_buffer.data(), toConsume);
        if (res == BodySink::BODY_AGAIN)
            return false;
        if (res != BodySink::BODY_OK)
        {
            _errorCode = (res < 0 ? -res : 500);
            _state = ERROR;
            return false;
        }
    }
    else
    {
        _body.insert(_body.end(), _buffer.begin(), _buffer.begin() + toConsume);
    }

    _bodyReceived += toConsume;
    _buffer.erase(0, toConsume);

    if (_bodyReceived >= contentLength)
    {
        _state = FINISHED;
        if (_bodySink)
            _bodySink->onBodyEnd();
    }

    return true;
}

bool Request::isFinished()
{
    return _state == FINISHED;
}