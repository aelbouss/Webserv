#include "../includes/request.hpp"
#include <cstdlib>

static std::string toLowerCopy(const std::string& value)
{
    std::string out = value;

    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}


Request::Request(): _method(UNKNOWN), _state(REQUEST_LINE), _chunkSize(0) {}
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
            // Check transfer-encoding first (headers are normalized to lowercase).
            if (_headers.count("transfer-encoding") &&
                toLowerCopy(_headers["transfer-encoding"]).find("chunked") != std::string::npos)
                _state = CHUNK_SIZE; 
            // if there is content lenght that's mean we have a body else finish baecause no body
            else if (_headers.count("content-length"))
                _state = BODY;
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
        _state = ERROR;
        return false;
    }

    if (_chunkSize == 0)
    {
        _state = FINISHED;
        return true;
    }

    _state = CHUNK_DATA;
    return true;
}


bool Request::parseChunkData()
{
    if (_buffer.size() < _chunkSize + 2)
        return false;

    _body.insert(_body.end(), _buffer.begin(), _buffer.begin() + _chunkSize);
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
        _state = ERROR;
        return false;
    }

    char *end_ptr = NULL;
    unsigned long contentLength = std::strtoul(it->second.c_str(), &end_ptr, 10);
    if (*end_ptr != '\0')
    {
        _state = ERROR;
        return false;
    }

    if (_buffer.size() < contentLength) // wait for  _buffer finish all bvbody content bytes we need base on Content-Length
        return false;
    // store all content from 0 to Content-Length in _body
    _body.insert(_body.end(), _buffer.begin(), _buffer.begin() + contentLength);
    //remove body copntent from _buffer
    _buffer.erase(0, contentLength);
    //update satate
    _state = FINISHED;

    return true;
}

bool Request::isFinished()
{
    return _state == FINISHED;
}