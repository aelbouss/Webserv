#include "request.hpp"
#include <cstdlib>


Request::Request(): _method(UNKNOWN), _state(REQUEST_LINE) {}
Request::Method Request::getMethod() const { return _method; }
const std::string& Request::getPath() const { return _path; }
const std::string& Request::getVersion() const { return _version; }
const std::vector<char>& Request::getBody() const { return _body; }

void Request::parse(const char* data, size_t size)
{
    _buffer.append(data, size);

    while (1)
    {
        if (_state == REQUEST_LINE && !parseRequestLine()) return;
        if (_state == HEADERS && !parseHeaders()) return;
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

    //methodStr that store firrst worda from request line
    if (methodStr == "GET")
        _method = GET;
    else if (methodStr == "POST")
        _method = POST;
    else if (methodStr == "DELETE")
        _method = DELETE;
    else
        _method = UNKNOWN;

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
            // if there is content lenght that's mean we have a body else finish baecause no body
            if (_headers.count("Content-Length"))
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
        //store 
        _headers[key] = value;
    }
}

bool Request::parseBody()
{
    size_t contentLenght = std::atoi(_headers["Content-Length"].c_str()); // convert value of Content-Length from str to int
    if (_buffer.size() < contentLenght) // wait for  _buffer finish all bvbody content bytes we need base on Content-Length
        return false;
    // store all content from 0 to Content-Length in _body
    _body.insert(_body.end(), _buffer.begin(), _buffer.begin() + contentLenght);
    //remove body copntent from _buffer
    _buffer.erase(0, contentLenght);
    //update satate
    _state = FINISHED;

    return true;
}

bool Request::isFinished()
{
    return _state == FINISHED;
}