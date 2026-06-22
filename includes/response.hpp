#pragma once

#include <string>
#include <map>
#include <vector>
#include "Location.hpp"
#include "request.hpp"
#include "CgiHandler.hpp"

class Location;
class Request;
class ServerConfig;

class Response
{
	public:
		Response();
		Response(int statusCode, const std::string& body,
				 const std::string& contentType);
		~Response();

	private:
		// Response owns a raw CgiHandler* during async CGI setup; forbid copying
		// to avoid double-free. It is only ever used as a single stack local.
		Response(const Response&);
		Response& operator=(const Response&);
	public:

		// Unified build method with optional location and server.
		void build(const std::string& method,
				   const std::string& path,
				   const std::vector<char>& requestBody,
				   const Location* location,
				   const ServerConfig* server ,
				   const std::string& defaultRoot,
				   const Request* request);

		// // Convenience overload for simple cases (no location/server rules).
		// void build(const std::string& method,
		// 		   const std::string& path,
		// 		   const std::vector<char>& requestBody,
		// 		   const std::string& webRoot);

		void serveFile(const std::string& filePath , const ServerConfig* server,
               const std::string& rangeHeader = "");
		void serveCgi(const std::string& scriptPath,
					  const std::string& requestPath,
					  const std::string& method,
					  const std::string& queryString,
					  const std::vector<char>& requestBody,
					  const Request* request = NULL,
				 	const ServerConfig* server = NULL);

		void setStatus(int code);
		void setHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);
		void setBody(const std::vector<char>& body);

		int                getStatusCode()    const;
		std::string        getStatusMessage() const;
		const std::string& getBody()          const;

		std::string toString() const;

		// ── Async (non-blocking) CGI support ──
		// When build() routes to a CGI script it forks the child and sets the
		// pipe ends non-blocking, then records the pending state here instead
		// of blocking. The multiplexer registers the pipes in the poll loop.
		bool               isCgiPending() const;
		CgiHandler*        detachCgi();              // transfer ownership out
		bool               cgiExpectsInput() const;  // POST body to feed?
		const std::string& getCgiInput() const;
		void               applyCgiOutput(const std::string& output);
		void               cgiErrorPage(int code, const ServerConfig* server);
		// Multiple Set-Cookie headers can coexist (server session + CGI).
		void               addSetCookie(const std::string& value);

	private:
		int                                 _statusCode;
		std::string                         _statusMessage;
		std::map<std::string, std::string>  _headers;
		std::string                         _body;
		std::vector<std::string>            _setCookies;
		// Async CGI state (owned until detachCgi()).
		bool                                _cgiPending;
		CgiHandler*                         _cgi;
		std::string                         _cgiInput;
		// If serving a file, store path and size; body will be empty.
		std::string                         _file_path;
		size_t                              _file_size;
		off_t                                _file_range_start;

		// Helper methods (no duplication)
		void setDefaultHeaders();
		void error(int code, const ServerConfig* server);
		void buildErrorPage(int code, const ServerConfig* server);
		bool executeCgiHandler(const std::string& filePath,
							   const std::string& requestPath,
							   const std::string& method,
							   const std::string& queryString,
							   const std::vector<char>& requestBody,
							   const Location* location,
							   const ServerConfig* server,
							   const Request* request);
		static std::string statusMessage(int code);
		static std::string mimeType(const std::string& path);
		static bool        fileExists(const std::string& path);
		static std::string readFile(const std::string& path);
		static std::string intToStr(size_t n);
		static std::string sanitizeFilename(const std::string& name);
		static bool        writeFile(const std::string& path, const std::vector<char>& data);
		static bool        extractMultipartFile(const Request& request, std::string& filename, std::vector<char>& data);
		int                handleUploadPost(const std::string& requestPath,
								   const std::vector<char>& requestBody,
								   const Location& location,
								   const Request* request);

	public:
		const std::string &getFilePath() const { return _file_path; }
		size_t getFileSize() const { return _file_size; }
		off_t getFileOffset() const { return _file_range_start; }
};