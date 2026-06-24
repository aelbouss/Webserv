#include "../includes/response.hpp"
#include "../includes/ServerConfig.hpp"

#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
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

static std::string joinPath(const std::string& left, const std::string& right)
{
	if (left.empty())
		return right;
	if (right.empty())
		return left;
	if (left[left.size() - 1] == '/' && right[0] == '/')
		return left + right.substr(1);
	if (left[left.size() - 1] != '/' && right[0] != '/')
		return left + "/" + right;
	return left + right;
}

static bool isDirectoryPath(const std::string& path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

static std::string stripLocationPrefix(const std::string& requestPath, const std::string& locationPath)
{
	if (locationPath.empty() || locationPath == "/")
		return requestPath;
	if (requestPath.compare(0, locationPath.size(), locationPath) != 0)
		return requestPath;
	std::string out = requestPath.substr(locationPath.size());
	return out.empty() ? "/" : out;
}

/*

dash w89 djndhgffdjfdjhkdsndfb

*/
static std::string buildAutoindexPage(const std::string& dirPath, const std::string& uriPath)
{
	DIR *dir = opendir(dirPath.c_str());
	if (!dir)
		return "";

	std::string base = uriPath;
	if (base.empty())
		base = "/";
	if (base[base.size() - 1] != '/')
		base += "/";

	std::string body = "<html><body><h1>Index of " + base + "</h1><ul>";
	for (struct dirent *entry = readdir(dir); entry; entry = readdir(dir))
	{
		std::string name = entry->d_name;
		if (name == ".")
			continue;
		std::string href = base + name;
		body += "<li><a href=\"" + href + "\">" + name + "</a></li>";
	}
	closedir(dir);
	body += "</ul></body></html>";
	return body;
}

static bool isCgiByLocation(const Location& location, const std::string& filePath)
{
	const std::vector<std::string>& exts = location.getCgiExtension();
	if (exts.empty())
		return false;
	for (size_t i = 0; i < exts.size(); ++i)
	{
		const std::string& ext = exts[i];
		if (ext.size() > 0 && ext[0] == '*')
		{
			std::string clean = ext.substr(1);
			if (filePath.size() >= clean.size() && filePath.substr(filePath.size() - clean.size()) == clean)
				return true;
		}
		else if (filePath.size() >= ext.size() && filePath.substr(filePath.size() - ext.size()) == ext)
			return true;
	}
	return false;
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

static std::string toLowerCopy(const std::string& value)
{
	std::string out = value;

	for (size_t i = 0; i < out.size(); ++i)
		out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
	return out;
}

static std::string basenameCopy(const std::string& path)
{
	size_t slash = path.find_last_of('/');
	std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
	slash = name.find_last_of('\\');
	if (slash != std::string::npos)
		name = name.substr(slash + 1);
	return name;
}

static std::string addSuffixToFilename(const std::string& filename, size_t suffix)
{
	if (suffix == 0)
		return filename;
	size_t dot = filename.find_last_of('.');
	std::ostringstream oss;
	if (dot == std::string::npos || dot == 0)
		oss << filename << '_' << suffix;
	else
		oss << filename.substr(0, dot) << '_' << suffix << filename.substr(dot);
	return oss.str();
}

struct UploadPart
{
	std::string filename;
	std::vector<char> data;
};

static std::string vectorSliceToString(const std::vector<char>& data, size_t start, size_t end)
{
	if (start >= data.size() || start >= end)
		return "";
	if (end > data.size())
		end = data.size();
	return std::string(data.begin() + start, data.begin() + end);
}

static bool findSequence(const std::vector<char>& haystack,
						 const std::vector<char>& needle,
						 size_t from,
						 size_t& position)
{
	if (needle.empty() || from > haystack.size())
		return false;
	std::vector<char>::const_iterator begin = haystack.begin() + from;
	std::vector<char>::const_iterator it = std::search(begin, haystack.end(), needle.begin(), needle.end());
	if (it == haystack.end())
		return false;
	position = static_cast<size_t>(it - haystack.begin());
	return true;
}

static bool parseBoundaryParameter(const std::string& contentType, std::string& boundary)
{
	std::string lower = toLowerCopy(contentType);
	size_t pos = lower.find("boundary=");
	if (pos == std::string::npos)
		return false;
	pos += 9;
	while (pos < contentType.size() && (contentType[pos] == ' ' || contentType[pos] == '\t'))
		++pos;
	if (pos >= contentType.size())
		return false;
	if (contentType[pos] == '"')
	{
		size_t end = contentType.find('"', pos + 1);
		if (end == std::string::npos)
			return false;
		boundary = contentType.substr(pos + 1, end - pos - 1);
	}
	else
	{
		size_t end = contentType.find(';', pos);
		boundary = contentType.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
	}
	boundary = trimCopy(boundary);
	return !boundary.empty();
}

static bool parseContentDispositionFilename(const std::string& disposition, std::string& filename)
{
	std::string lower = toLowerCopy(disposition);
	size_t pos = lower.find("filename=");
	if (pos == std::string::npos)
		return false;
	pos += 9;
	while (pos < disposition.size() && (disposition[pos] == ' ' || disposition[pos] == '\t'))
		++pos;
	if (pos >= disposition.size())
		return false;
	if (disposition[pos] == '"')
	{
		size_t end = disposition.find('"', pos + 1);
		if (end == std::string::npos)
			return false;
		filename = disposition.substr(pos + 1, end - pos - 1);
	}
	else
	{
		size_t end = disposition.find(';', pos);
		filename = disposition.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
	}
	filename = trimCopy(filename);
	filename = basenameCopy(filename);
	return !filename.empty();
}

static bool extractHeadersBlock(const std::vector<char>& body,
							   size_t start,
							   size_t end,
							   std::string& contentDisposition)
{
	std::string headers = vectorSliceToString(body, start, end);
	std::istringstream stream(headers);
	std::string line;
	while (std::getline(stream, line))
	{
		line = trimCopy(line);
		if (line.empty())
			continue;
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;
		std::string key = trimCopy(line.substr(0, colon));
		std::string value = trimCopy(line.substr(colon + 1));
		if (toLowerCopy(key) == "content-disposition")
			contentDisposition = value;
	}
	return !contentDisposition.empty();
}

static bool extractNextBoundary(const std::vector<char>& body,
							   const std::vector<char>& boundaryLine,
							   size_t searchFrom,
							   size_t& boundaryPos)
{
	size_t pos = searchFrom;
	while (findSequence(body, boundaryLine, pos, boundaryPos))
	{
		if (boundaryPos == 0)
			return true;
		if (boundaryPos >= 2 && body[boundaryPos - 2] == '\r' && body[boundaryPos - 1] == '\n')
			return true;
		pos = boundaryPos + 1;
	}
	return false;
}

// FIX: removed the strict "\r\n must precede first boundary" check.
// RFC 2046 defines the preamble before the first boundary as ignorable,
// so we simply find the first boundary wherever it appears in the body.
static bool parseMultipartBody(const std::vector<char>& body,
							 const std::string& boundary,
							 std::vector<UploadPart>& files)
{
	if (boundary.empty())
		return false;

	std::vector<char> boundaryLine;
	boundaryLine.push_back('-');
	boundaryLine.push_back('-');
	boundaryLine.insert(boundaryLine.end(), boundary.begin(), boundary.end());

	size_t boundaryPos = 0;
	// RFC 2046: preamble before the first boundary is ignorable — just locate it.
	if (!findSequence(body, boundaryLine, 0, boundaryPos))
		return false;

	size_t partStart = boundaryPos + boundaryLine.size();
	if (partStart + 2 <= body.size() && body[partStart] == '-' && body[partStart + 1] == '-')
		return true;
	if (partStart + 2 > body.size() || body[partStart] != '\r' || body[partStart + 1] != '\n')
		return false;
	partStart += 2;

	std::vector<char> headerMarker;
	headerMarker.push_back('\r');
	headerMarker.push_back('\n');
	headerMarker.push_back('\r');
	headerMarker.push_back('\n');

	while (partStart < body.size())
	{
		size_t headersEnd = 0;
		if (!findSequence(body, headerMarker, partStart, headersEnd))
			return false;

		std::string contentDisposition;
		extractHeadersBlock(body, partStart, headersEnd, contentDisposition);

		size_t dataStart = headersEnd + 4;
		size_t nextBoundary = 0;
		if (!extractNextBoundary(body, boundaryLine, dataStart, nextBoundary))
			return false;
		if (nextBoundary < 2 || body[nextBoundary - 2] != '\r' || body[nextBoundary - 1] != '\n')
			return false;

		size_t dataEnd = nextBoundary - 2;
		if (dataEnd < dataStart)
			return false;

		if (!contentDisposition.empty())
		{
			std::string filename;
			if (parseContentDispositionFilename(contentDisposition, filename))
			{
				UploadPart part;
				part.filename = filename;
				part.data.assign(body.begin() + dataStart, body.begin() + dataEnd);
				files.push_back(part);
			}
		}

		partStart = nextBoundary + boundaryLine.size();
		if (partStart + 2 <= body.size() && body[partStart] == '-' && body[partStart + 1] == '-')
			return true;
		if (partStart + 2 > body.size() || body[partStart] != '\r' || body[partStart + 1] != '\n')
			return false;
		partStart += 2;
	}
	return true;
}

static bool ensureDirectoryRecursive(const std::string& directory)
{
	if (directory.empty())
		return false;

	std::string clean = directory;
	while (clean.size() > 1 && clean[clean.size() - 1] == '/')
		clean.erase(clean.size() - 1);

	struct stat st;
	if (stat(clean.c_str(), &st) == 0)
		return S_ISDIR(st.st_mode);

	size_t slash = clean.find_last_of('/');
	if (slash != std::string::npos && slash > 0)
	{
		std::string parent = clean.substr(0, slash);
		if (!ensureDirectoryRecursive(parent))
			return false;
	}
	if (mkdir(clean.c_str(), 0755) < 0 && errno != EEXIST)
		return false;
	return true;
}

// FIX: replace unsafe characters with '_' instead of rejecting the filename outright.
// The old code returned "" for any filename containing spaces or special chars (e.g.
// "my photo.jpg", "file (1).txt"), which caused handleUploadPost to return 400.
std::string Response::sanitizeFilename(const std::string& name)
{
	std::string base = basenameCopy(trimCopy(name));
	if (base.empty() || base == "." || base == "..")
		return "";
	std::string result;
	for (size_t i = 0; i < base.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(base[i]);
		if (std::isalnum(c) || c == '.' || c == '-' || c == '_')
			result += static_cast<char>(c);
		else
			result += '_';
	}
	if (result.empty() || result == "." || result == "..")
		return "";
	return result;
}

bool Response::writeFile(const std::string& path, const std::vector<char>& data)
{
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd < 0)
		return false;

	size_t written = 0;
	while (written < data.size())
	{
		ssize_t chunk = write(fd, &data[written], data.size() - written);
		if (chunk <= 0)
		{
			close(fd);
			unlink(path.c_str());
			return false;
		}
		written += static_cast<size_t>(chunk);
	}
	if (close(fd) < 0)
	{
		unlink(path.c_str());
		return false;
	}
	return true;
}

bool Response::extractMultipartFile(const Request& request, std::string& filename, std::vector<char>& data)
{
	std::vector<UploadPart> files;
	std::string boundary;
	if (!parseBoundaryParameter(request.getHeader("Content-Type"), boundary))
		return false;
	if (!parseMultipartBody(request.getBody(), boundary, files))
		return false;
	if (files.empty())
		return false;
	filename = files[0].filename;
	data = files[0].data;
	return true;
}

int Response::handleUploadPost(const std::string& requestPath,
							  const std::vector<char>& requestBody,
							  const Location& location,
							  const Request* request)
{
	std::string uploadDir = location.getUploadStore();
	if (uploadDir.empty())
		return 403;
	if (!ensureDirectoryRecursive(uploadDir))
		return 500;

	std::vector<UploadPart> parts;
	if (request)
	{
		std::string contentType = request->getHeader("Content-Type");
		std::string lowerType = toLowerCopy(contentType);
		if (lowerType.find("multipart/form-data") != std::string::npos)
		{
			std::string boundary;
			if (!parseBoundaryParameter(contentType, boundary))
				return 400;
			if (!parseMultipartBody(requestBody, boundary, parts))
				return 400;
		}
		else
		{
			UploadPart part;
			part.filename = basenameCopy(requestPath);
			part.data = requestBody;
			parts.push_back(part);
		}
	}
	else
	{
		UploadPart part;
		part.filename = basenameCopy(requestPath);
		part.data = requestBody;
		parts.push_back(part);
	}

	if (parts.empty())
	{
		setStatus(201);
		setHeader("Content-Type", "text/plain");
		setBody("Created: no files uploaded");
		return 201;
	}

	std::vector<std::string> savedPaths;
	for (size_t i = 0; i < parts.size(); ++i)
	{
		std::string filename = sanitizeFilename(parts[i].filename);
		if (filename.empty())
			return 400;

		bool saved = false;
		for (size_t suffix = 0; suffix < 1000; ++suffix)
		{
			std::string candidate = joinPath(uploadDir, addSuffixToFilename(filename, suffix));
			if (writeFile(candidate, parts[i].data))
			{
				savedPaths.push_back(candidate);
				saved = true;
				break;
			}
			if (errno != EEXIST)
				return 500;
		}
		if (!saved)
			return 500;
	}

	setStatus(201);
	setHeader("Content-Type", "text/plain");
	std::string body;
	for (size_t i = 0; i < savedPaths.size(); ++i)
	{
		if (!body.empty())
			body += "\n";
		std::string urlPath = savedPaths[i];
		if (!uploadDir.empty() && urlPath.compare(0, uploadDir.size(), uploadDir) == 0)
    		urlPath = urlPath.substr(uploadDir.size());
		// Make sure it starts with /
		if (urlPath.empty() || urlPath[0] != '/')
    		urlPath = "/" + urlPath;
		body += "Created: " + urlPath;

	}
	setBody(body);
	return 201;
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
				std::string keyLower = toLowerCopy(key);
				if (keyLower == "status")
				{
					std::istringstream iss(value);
					int code;
					if (iss >> code)
						res.setStatus(code);
				}
				else if (keyLower == "set-cookie")
					res.addSetCookie(value);
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
	: _statusCode(200), _statusMessage("OK"),
	  _cgiPending(false), _cgi(NULL),
	  _file_path(""), _file_size(0), _file_range_start(0)
{
}

Response::Response(int statusCode, const std::string& body,
				   const std::string& contentType)
	: _statusCode(statusCode),
	  _statusMessage(statusMessage(statusCode)),
	  _body(body),
	  _cgiPending(false), _cgi(NULL),
	  _file_size(0), _file_range_start(0)
{
	_headers["Content-Type"]   = contentType;
	_headers["Content-Length"] = intToStr(body.size());
}

Response::~Response()
{
	// If the CGI handler was never detached (e.g. an error before hand-off),
	// destroy it here so the child is reaped and memory freed.
	if (_cgi)
		delete _cgi;
}

bool Response::isCgiPending() const { return _cgiPending; }

CgiHandler* Response::detachCgi()
{
	CgiHandler* tmp = _cgi;
	_cgi = NULL;
	_cgiPending = false;
	return tmp;
}

bool Response::cgiExpectsInput() const { return !_cgiInput.empty(); }

const std::string& Response::getCgiInput() const { return _cgiInput; }

void Response::addSetCookie(const std::string& value)
{
	_setCookies.push_back(value);
}

void Response::cgiErrorPage(int code, const ServerConfig* server)
{
	buildErrorPage(code, server);
}

void Response::applyCgiOutput(const std::string& output)
{
	if (output.empty())
	{
		// CGI produced nothing -> bad gateway.
		setStatus(502);
		setHeader("Content-Type", "text/html");
		setBody("<html><body><h1>502 Bad Gateway</h1></body></html>");
		return;
	}
	setStatus(200);
	applyCgiOutputHeaders(*this, output);
}

void Response::setDefaultHeaders(const Request* request)
{
    _headers["Server"] = "Webserv/1.0";

    std::string conn = request ? request->getHeader("connection") : "close";
    std::string ver  = request ? request->getVersion() : "HTTP/1.0";

    bool keepAlive = false;
    if (ver == "HTTP/1.1")
        keepAlive = (toLowerCopy(conn) != "close");
    else
        keepAlive = (toLowerCopy(conn) == "keep-alive");

    _headers["Connection"] = keepAlive ? "keep-alive" : "close";
}

void Response::error(int code, const ServerConfig* server)
{
	setStatus(code);
	buildErrorPage(code, server);
	if (code == 405 && !_headers.count("Allow"))
		_headers["Allow"] = "GET, POST, DELETE";
}

bool Response::executeCgiHandler(const std::string& filePath,
								  const std::string& requestPath,
								  const std::string& method,
								  const std::string& queryString,
								  const std::vector<char>& requestBody,
								  const Location* location,
								  const ServerConfig* server,
								  const Request* request)
{
	Request req;
	std::string target = requestPath.empty() ? "/" : requestPath;
	if (!queryString.empty())
		target += "?" + queryString;

	std::string raw = method + " " + target + " HTTP/1.1\r\n";
	raw += "Host: localhost\r\n";
	if (request && !request->getHeader("content-type").empty())
		raw += "Content-Type: " + request->getHeader("content-type") + "\r\n";
	if (method == "POST" || method == "PUT" || !requestBody.empty())
		raw += "Content-Length: " + intToStr(requestBody.size()) + "\r\n";
	if (request && !request->getHeader("cookie").empty())
		raw += "Cookie: " + request->getHeader("cookie") + "\r\n";
	raw += "\r\n";
	if (!requestBody.empty())
		raw.append(requestBody.begin(), requestBody.end());
	req.parse(raw.c_str(), raw.size());
	req.markBodyConfigReady();
	req.parse("", 0);

	if (!fileExists(filePath))
	{
		error(404, server);
		return false;
	}

	CgiHandler* cgi = new CgiHandler(filePath);
	if (location)
		cgi->initEnvFromLocation(req, *location);
	else
		cgi->initEnvBasic(req, filePath, requestPath.empty() ? "/" : requestPath, queryString);

	short error_code = cgi->start(req);
	if (error_code != 0)
	{
		delete cgi;
		error(error_code, server);
		return false;
	}

	// Hand off to the poll loop: record pending CGI + the POST body to feed.
	_cgi = cgi;
	_cgiPending = true;
	if (!requestBody.empty())
		_cgiInput.assign(requestBody.begin(), requestBody.end());
	return true;
}

void Response::build(const std::string& method,
					 const std::string& path,
					 const std::vector<char>& requestBody,
					 const Location* location,
					 const ServerConfig* server,
					 const std::string& defaultRoot,
					 const Request* request)
{
	std::string requestPath = path.empty() ? "/" : path;
	std::string queryString;
	size_t queryPos = requestPath.find('?');
	if (queryPos != std::string::npos)
	{
		queryString = requestPath.substr(queryPos + 1);
		requestPath = requestPath.substr(0, queryPos);
	}

	if (request)
	{
		if (request->hasError())
		{
			int code = request->getErrorCode();
			error(code ? code : 400, server);
			setDefaultHeaders(request);;
			return;
		}
		if (request->getVersion() == "HTTP/1.1" && request->getHeader("host").empty())
		{
			error(400, server);
			setDefaultHeaders(request);;
			return;
		}
	}

	// If the method is not one of the implemented verbs, return 501 Not Implemented.
	if (!(method == "GET" || method == "POST" || method == "DELETE"))
	{
		error(501, server);
		setDefaultHeaders(request);;
		return;
	}

	size_t bodyLimit = 0;
	if (location)
		bodyLimit = location->getMaxBodySize();
	else if (server)
		bodyLimit = server->getClientMaxBodySize();
	if (bodyLimit != 0 && requestBody.size() > bodyLimit)
	{
		error(413, server);
		setDefaultHeaders(request);;
		return;
	}

	if (location)
	{
		const std::vector<short>& methods = location->getMethods();

		if (!isMethodAllowedByLocation(method, *location))
		{
			error(405, server);
			_headers["Allow"] = buildAllowHeaderFromMethods(methods);
			setDefaultHeaders(request);;
			return;
		}

		if (!location->getReturn().empty())
		{
			setStatus(302);
			setHeader("Location", location->getReturn());
			setBody("");
			setDefaultHeaders(request);;
			return;
		}

		std::string rel = stripLocationPrefix(requestPath, location->getPath());
		std::string baseRoot = location->getAlias().empty()
			? (location->getRootLocation().empty() ? defaultRoot : location->getRootLocation())
			: location->getAlias();
		std::string filePath = joinPath(baseRoot, rel);

		if (method == "GET" && isDirectoryPath(filePath))
		{
			bool servedIndex = false;
			if (!location->getIndexLocation().empty())
			{
				std::string indexPath = joinPath(filePath, location->getIndexLocation());
				if (fileExists(indexPath))
				{
					filePath = indexPath;
					servedIndex = true;
				}
			}
			if (!servedIndex)
			{
				if (location->getAutoindex())
				{
					setStatus(200);
					setHeader("Content-Type", "text/html");
					setBody(buildAutoindexPage(filePath, requestPath));
					setDefaultHeaders(request);;
					return;
				}
				else
				{
					error(403, server);
					setDefaultHeaders(request);;
					return;
				}
			}
		}

		bool useCgi = isCgiByLocation(*location, filePath) || isCgiScriptPath(filePath);

		if (method == "GET")
		{
			if (useCgi)
			{
				if (location->getExtensionPath().empty())
					serveCgi(filePath, requestPath, method, queryString, requestBody, request , server);
				else
					executeCgiHandler(filePath, requestPath, method, queryString, requestBody, location, server, request);
			}
			else
				serveFile(filePath, server, request ? request->getHeader("range") : "");
		}
		else if (method == "POST")
		{
			if (useCgi)
			{
				if (location->getExtensionPath().empty())
					serveCgi(filePath, requestPath, method, queryString, requestBody, request , server);
				else
					executeCgiHandler(filePath, requestPath, method, queryString, requestBody, location, server, request);
			}
				else if (!location->getUploadStore().empty())
				{
					if (request && request->isBodyStreaming())
					{
						if (request->hasUploadResult())
						{
							setStatus(201);
							setHeader("Content-Type", "text/plain");
							setBody("Created: " + request->getUploadResultUrl());
							setDefaultHeaders(request);;
							return;
						}
						error(500, server);
						setDefaultHeaders(request);;
						return;
					}
					int uploadStatus = handleUploadPost(requestPath, requestBody, *location, request);
					if (uploadStatus != 201)
					{
						error(uploadStatus, server);
						setDefaultHeaders(request);;
						return;
					}
				}
			else
			{
				error(403, server);
				setDefaultHeaders(request);;
				return;
			}
		}
		else if (method == "DELETE")
		{
			if (isDirectoryPath(filePath))
				error(403, server);
			else if (!fileExists(filePath))
				error(404, server);
			else if (std::remove(filePath.c_str()) == 0)
			{
				setStatus(200);
				setHeader("Content-Type", "text/plain");
				setBody("Deleted");
			}
			else
				error(500, server);
		}
		else
		{
			error(405, server);
		}
	}
	else
	{
		std::string filePath = defaultRoot + (requestPath == "/" ? "/index.html" : requestPath);

		if (method == "GET")
		{
			if (isCgiScriptPath(filePath))
				serveCgi(filePath, requestPath, method, queryString, requestBody, request , server);
			else
				serveFile(filePath, server, request ? request->getHeader("range") : "");
		}
		else if (method == "POST")
		{
			if (isCgiScriptPath(filePath))
				serveCgi(filePath, requestPath, method, queryString, requestBody, request , server);
			else
			{
				error(404, server);
				setDefaultHeaders(request);;
				return;
			}
		}
		else if (method == "DELETE")
		{
			if (!fileExists(filePath))
				error(404, server);
			else if (std::remove(filePath.c_str()) == 0)
			{
				setStatus(200);
				setHeader("Content-Type", "text/plain");
				setBody("Deleted");
			}
			else
				error(500, server);
		}
		else
		{
			error(405, server);
		}
	}

	setDefaultHeaders(request);;
}

void Response::serveFile(const std::string& filePath, const ServerConfig* server,
						  const std::string& rangeHeader)
{
	if (!fileExists(filePath))
	{
		buildErrorPage(404, server);
		return;
	}

	struct stat st;
	if (stat(filePath.c_str(), &st) != 0)
	{
		buildErrorPage(500, NULL);
		return;
	}

	size_t totalSize = static_cast<size_t>(st.st_size);
	size_t start = 0;
	size_t end = (totalSize > 0) ? totalSize - 1 : 0;
	bool isRange = false;

	if (!rangeHeader.empty() && rangeHeader.compare(0, 6, "bytes=") == 0)
	{
		std::string spec = rangeHeader.substr(6);
		size_t dash = spec.find('-');
		if (dash != std::string::npos)
		{
			std::string startStr = spec.substr(0, dash);
			std::string endStr = spec.substr(dash + 1);
			char* endptr;

			if (!startStr.empty())
			{
				unsigned long parsedStart = std::strtoul(startStr.c_str(), &endptr, 10);
				if (*endptr == '\0')
				{
					start = parsedStart;
					isRange = true;
					if (!endStr.empty())
					{
						unsigned long parsedEnd = std::strtoul(endStr.c_str(), &endptr, 10);
						if (*endptr == '\0')
							end = parsedEnd;
					}
				}
			}
			else if (!endStr.empty())
			{
				// Suffix range: "bytes=-500" means last 500 bytes
				unsigned long parsedSuffix = std::strtoul(endStr.c_str(), &endptr, 10);
				if (*endptr == '\0' && parsedSuffix > 0)
				{
					isRange = true;
					start = (parsedSuffix >= totalSize) ? 0 : totalSize - parsedSuffix;
					end = (totalSize > 0) ? totalSize - 1 : 0;
				}
			}
		}
	}

	if (isRange && (start > end || start >= totalSize))
	{
		setStatus(416);
		setHeader("Content-Range", "bytes */" + intToStr(totalSize));
		setHeader("Content-Type", "text/html");
		setBody("");
		return;
	}

	if (end >= totalSize)
		end = (totalSize > 0) ? totalSize - 1 : 0;

	size_t length = end - start + 1;

	if (isRange)
	{
		setStatus(206);
		std::ostringstream cr;
		cr << "bytes " << start << "-" << end << "/" << totalSize;
		setHeader("Content-Range", cr.str());
	}
	else
		setStatus(200);

	setHeader("Accept-Ranges", "bytes");
	setHeader("Content-Type", mimeType(filePath));

	// Load the file fully into the response body. Sending is then driven by
	// the poll loop one send() per POLLOUT, so no fd is read/written outside
	// of poll() and there is no sendfile loop.
	std::string content = readFile(filePath);
	if (isRange)
	{
		if (start > content.size())
			start = content.size();
		if (length > content.size() - start)
			length = content.size() - start;
		_body = content.substr(start, length);
	}
	else
		_body = content;
	setHeader("Content-Length", intToStr(_body.size()));
	// _file_path intentionally left empty: no separate file streaming.
}

void Response::serveCgi(const std::string& scriptPath,
						const std::string& requestPath,
						const std::string& method,
						const std::string& queryString,
						const std::vector<char>& requestBody,
						const Request* request,
						const ServerConfig* server)
{
	(void)server;  // Suppress unused parameter warning
	if (!fileExists(scriptPath))
	{
		buildErrorPage(404, server);
		return;
	}

	Request req;
	std::string target = requestPath.empty() ? "/" : requestPath;
	if (!queryString.empty())
		target += "?" + queryString;

	std::string raw = method + " " + target + " HTTP/1.1\r\n";
	raw += "Host: localhost\r\n";
	if (request && !request->getHeader("content-type").empty())
		raw += "Content-Type: " + request->getHeader("content-type") + "\r\n";
	if (method == "POST" || method == "PUT" || !requestBody.empty())
		raw += "Content-Length: " + intToStr(requestBody.size()) + "\r\n";
	if (request && !request->getHeader("cookie").empty())
		raw += "Cookie: " + request->getHeader("cookie") + "\r\n";
	raw += "\r\n";
	if (!requestBody.empty())
		raw.append(requestBody.begin(), requestBody.end());
	req.parse(raw.c_str(), raw.size());
	req.markBodyConfigReady();
	req.parse("", 0);

	CgiHandler* cgi = new CgiHandler(scriptPath);
	cgi->initEnvBasic(req, scriptPath, requestPath.empty() ? "/" : requestPath, queryString);

	short error_code = cgi->start(req);
	if (error_code != 0)
	{
		delete cgi;
		buildErrorPage(error_code, server);
		return;
	}

	_cgi = cgi;
	_cgiPending = true;
	if (!requestBody.empty())
		_cgiInput.assign(requestBody.begin(), requestBody.end());
}

std::string Response::toString() const
{
	std::string out;

	out += "HTTP/1.1 " + intToStr(_statusCode) + " " + _statusMessage + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it)
		out += it->first + ": " + it->second + "\r\n";
	for (std::vector<std::string>::const_iterator it = _setCookies.begin();
		 it != _setCookies.end(); ++it)
		out += "Set-Cookie: " + *it + "\r\n";
	out += "\r\n";
	if (_file_path.empty())
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

void Response::buildErrorPage(int code, const ServerConfig* server)
{
	setStatus(code);

	if (server)
	{
		const std::map<short, std::string>& pages = server->getErrorPages();
		std::map<short, std::string>::const_iterator it = pages.find(code);
		if (it != pages.end() && !it->second.empty())
		{
			std::string path = it->second;
			if (path[0] != '/')
				path = joinPath(server->getRoot(), path);
			if (fileExists(path))
			{
				setHeader("Content-Type", mimeType(path));
				setBody(readFile(path));
				return;
			}
		}
	}

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
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocol";
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        case 300:
            return "Multiple Choice";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Moved Temporarily";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        case 307:
            return "Temporary Redirect";
        case 308:
            return "Permanent Redirect";
        case 400:
            return "Bad Request";
        case 401:
            return "Unaut206horized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Payload Too Large";
        case 414:
            return "URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Requested Range Not Satisfiable";
        case 417:
            return "Expectation Failed";
        case 418:
            return "I'm a teapot";
        case 421:
            return "Misdirected Not FoundRequest";
        case 425:
            return "Too Early";
        case 426:
            return "Upgrade Required";
        case 428:
            return "Precondition Required";
        case 429:
            return "Too Many Requests";
        case 431:
            return "Request Header Fields Too Large";
        case 451:
            return "Unavailable for Legal Reasons";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        case 506:
            return "Variant Also Negotiates";
        case 507:
            return "Insufficient Storage";
        case 510:
            return "Not Extended";
        case 511:
            return "Network Authentication Required";
        default:
            return "Undefined";
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
	if (ext == "mp4")                   return "video/mp4";
	if (ext == "webm")                  return "video/webm";
	if (ext == "ogg")                   return "video/ogg";
	if (ext == "mp3")                   return "audio/mpeg";
	if (ext == "wav")                   return "audio/wav";

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