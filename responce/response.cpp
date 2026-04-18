#include "../includes/response.hpp"

Response::Response()
    : _statusCode(200), _statusMessage("OK")
{}

Response::Response(int statusCode, const std::string& body,
                   const std::string& contentType)
    : _statusCode(statusCode),
      _statusMessage(statusMessage(statusCode)),
      _body(body)
{
    _headers["Content-Type"]   = contentType;
    _headers["Content-Length"] = intToStr(body.size());
}

void Response::build(const std::string& method,
                     const std::string& path,
                     const std::vector<char>& requestBody,
                     const std::string& webRoot)
{
    // Resolve path:  "/" -> "/index.html"
    std::string filePath = webRoot + (path == "/" ? "/index.html" : path);

    if (method == "GET")
    {
        serveFile(filePath);
    }
    else if (method == "POST")
    {
        if (filePath.size() >= 3 &&
            filePath.substr(filePath.size() - 3) == ".py")
        {
            serveCgi(filePath, method, "", requestBody);
        }
        else
        {
            setStatus(201);
            setHeader("Content-Type", "text/plain");
            setBody("Created");
        }
    }
    else if (method == "DELETE")
    {
        if (!fileExists(filePath))
        {
            buildErrorPage(404);
        }
        else if (std::remove(filePath.c_str()) == 0)
        {
            setStatus(200);
            setHeader("Content-Type", "text/plain");
            setBody("Deleted");
        }
        else
        {
            buildErrorPage(500);
        }
    }
    else
    {
        buildErrorPage(405);
        _headers["Allow"] = "GET, POST, DELETE";
    }

    _headers["Server"]     = "Webserv/1.0";
    _headers["Connection"] = "close";
}

void Response::serveFile(const std::string& filePath)
{
    if (!fileExists(filePath))
    {
        buildErrorPage(404);
        return;
    }

    std::string content = readFile(filePath);
    if (content.empty())
    {
        buildErrorPage(500);
        return;
    }

    setStatus(200);
    setHeader("Content-Type", mimeType(filePath));
    setBody(content);
}

//hadi
void Response::serveCgi(const std::string& scriptPath,
                        const std::string& method,
                        const std::vector<char>& requestBody)
{
    CgiHandler cgi(scriptPath);
    cgi.setEnv(method, requestBody);
    std::string output = cgi.execute_cgi(requestBody);

    if (output.empty())
    {
        buildErrorPage(500);
        return;
    }
    setStatus(200);
    setHeader("Content-Type", "text/html");
    setBody(output);
}

std::string Response::toString() const
{
    std::string out;

    out += "HTTP/1.1 " + intToStr(_statusCode) + " " + _statusMessage + "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it)
        out += it->first + ": " + it->second + "\r\n";
    out += "\r\n";
    out += _body;

    return out;
}

void Response::setStatus(int code)
{
    _statusCode    = code;
    _statusMessage = statusMessage(code);
}

void Response::setHeader(const std::string& key, const std::string& value)
{
    _headers[key] = value;
}

void Response::setBody(const std::string& body)
{
    _body = body;
    _headers["Content-Length"] = intToStr(body.size());
}

void Response::setBody(const std::vector<char>& body)
{
    _body.assign(body.begin(), body.end());
    _headers["Content-Length"] = intToStr(_body.size());
}


int                Response::getStatusCode()    const { return _statusCode; }
std::string        Response::getStatusMessage() const { return _statusMessage; }
const std::string& Response::getBody()          const { return _body; }

void Response::buildErrorPage(int code)
{
    setStatus(code);
    setHeader("Content-Type", "text/html");

    std::string msg = statusMessage(code);
    std::string body = "<html><body><h1>"
                     + intToStr(code) + " " + msg
                     + "</h1></body></html>";
    setBody(body);
}

std::string Response::statusMessage(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default:  return "Unknown";
    }
}

std::string Response::mimeType(const std::string& path)
{
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot + 1);

    if (ext == "html" || ext == "htm")  return "text/html";
    if (ext == "css")                   return "text/css";
    if (ext == "js")                    return "application/javascript";
    if (ext == "json")                  return "application/json";
    if (ext == "png")                   return "image/png";
    if (ext == "jpg" || ext == "jpeg")  return "image/jpeg";
    if (ext == "gif")                   return "image/gif";
    if (ext == "svg")                   return "image/svg+xml";
    if (ext == "ico")                   return "image/x-icon";
    if (ext == "txt")                   return "text/plain";
    if (ext == "pdf")                   return "application/pdf";

    return "application/octet-stream";//hadi
}


//hadi
bool Response::fileExists(const std::string& path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

std::string Response::readFile(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
    return std::string(
        (std::istreambuf_iterator<char>(file)),
         std::istreambuf_iterator<char>()
    );
}

std::string Response::intToStr(size_t n)
{
    std::ostringstream ss;
    ss << n;
    return ss.str();
}