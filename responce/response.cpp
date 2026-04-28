#include "../includes/response.hpp"
#include "../includes/Location.hpp"
#include "../includes/CgiHandler.hpp"

#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cerrno>

static std::string buildAllowHeaderFromMethods(const std::vector<short>& methods)
{
	static const char* names[5] = {"GET", "POST", "DELETE", "PUT", "HEAD"};
	std::string out;

	for (size_t i = 0; i < 5 && i < methods.size(); ++i)
	{
		if (!methods[i])
			continue;
		if (!out.empty())
			out += ", ";
		out += names[i];
	}
	return out;
}

static bool isMethodAllowedByLocation(const std::string& method, const Location& location)
{
	const std::vector<short>& methods = location.getMethods();

	if (methods.size() < 5)
		return false;
	if (method == "GET")
		return methods[0] != 0;
	if (method == "POST")
		return methods[1] != 0;
	if (method == "DELETE")
		return methods[2] != 0;
	if (method == "PUT")
		return methods[3] != 0;
	if (method == "HEAD")
		return methods[4] != 0;
	return false;
}

static std::string resolveRoot(const Location& location, const std::string& serverRoot)
{
	if (!location.getRootLocation().empty())
		return location.getRootLocation();
	return serverRoot;
}

static bool isCgiScriptPath(const std::string& filePath)
{
	if (filePath.size() >= 3 && filePath.substr(filePath.size() - 3) == ".py")
		return true;
	if (filePath.size() >= 3 && filePath.substr(filePath.size() - 3) == ".sh")
		return true;
	return false;
}

static std::string trimCopy(const std::string& s)
{
	size_t begin = 0;
	size_t end = s.size();

	while (begin < end && (s[begin] == ' ' || s[begin] == '\t' || s[begin] == '\r' || s[begin] == '\n'))
		++begin;
	while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n'))
		--end;
	return s.substr(begin, end - begin);
}

static bool writeAll(int fd, const char* data, size_t size)
{
	size_t written = 0;

	while (written < size)
	{
		ssize_t n = write(fd, data + written, size - written);
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			return false;
		}
		written += static_cast<size_t>(n);
	}
	return true;
}

static std::string readAll(int fd)
{
	char buf[4096];
	std::string out;

	while (true)
	{
		ssize_t n = read(fd, buf, sizeof(buf));
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			break;
		}
		if (n == 0)
			break;
		out.append(buf, static_cast<size_t>(n));
	}
	return out;
}

static void applyCgiOutputHeaders(Response& res, const std::string& output)
{
	size_t headersEnd = output.find("\r\n\r\n");
	size_t sepSize = 4;

	if (headersEnd == std::string::npos)
	{
		res.setHeader("Content-Type", "text/html");
		res.setBody(output);
		return;
	}

	std::string headerBlock = output.substr(0, headersEnd);
	std::string body = output.substr(headersEnd + sepSize);
	size_t start = 0;

	while (start < headerBlock.size())
	{
		size_t end = headerBlock.find('\n', start);
		std::string line = (end == std::string::npos)
			? headerBlock.substr(start)
			: headerBlock.substr(start, end - start);
		line = trimCopy(line);

		if (!line.empty())
		{
			size_t colon = line.find(':');
			if (colon != std::string::npos)
			{
				std::string key = trimCopy(line.substr(0, colon));
				std::string value = trimCopy(line.substr(colon + 1));
				if (key == "Status")
				{
					std::istringstream iss(value);
					int code;
					if (iss >> code)
						res.setStatus(code);
				}
				else
					res.setHeader(key, value);
			}
		}
		if (end == std::string::npos)
			break;
		start = end + 1;
	}
	res.setBody(body);
}

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
	std::string requestPath = path;
	std::string queryString;
	size_t queryPos = requestPath.find('?');

	if (queryPos != std::string::npos)
	{
		queryString = requestPath.substr(queryPos + 1);
		requestPath = requestPath.substr(0, queryPos);
	}
	if (requestPath.empty())
		requestPath = "/";

	std::string filePath = webRoot + (requestPath == "/" ? "/index.html" : requestPath);

	if (method == "GET")
	{
		if (isCgiScriptPath(filePath))
			serveCgi(filePath, method, queryString, requestBody);
		else
			serveFile(filePath);
	}
	else if (method == "POST")
	{
		if (isCgiScriptPath(filePath))
			serveCgi(filePath, method, queryString, requestBody);
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
	}

	_headers["Server"]     = "Webserv/1.0";
	_headers["Connection"] = "close";
}

void Response::build(const std::string& method,
					 const std::string& path,
					 const std::vector<char>& requestBody,
					 const Location& location,
					 const std::string& serverRoot)
{
	const std::vector<short>& methods = location.getMethods();

	if (!isMethodAllowedByLocation(method, location))
	{
		buildErrorPage(405);
		_headers["Allow"] = buildAllowHeaderFromMethods(methods);
		_headers["Server"] = "Webserv/1.0";
		_headers["Connection"] = "close";
		return;
	}

	build(method, path, requestBody, resolveRoot(location, serverRoot));
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


void Response::serveCgi(const std::string& scriptPath,
						const std::string& method,
						const std::string& queryString,
						const std::vector<char>& requestBody)
{
    if (!fileExists(scriptPath))
    {
        buildErrorPage(404);
        return;
    }

    CgiHandler cgi(scriptPath);

    short error_code = 0;
    std::string output = cgi.execute(error_code);

    if (error_code != 0)
    {
        buildErrorPage(error_code);
        return;
    }

    setStatus(200);
    applyCgiOutputHeaders(*this, output);
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

	return "application/octet-stream";
}

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
