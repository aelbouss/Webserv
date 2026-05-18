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

		// Unified build method with optional location and server.
		void build(const std::string& method,
				   const std::string& path,
				   const std::vector<char>& requestBody,
				   const Location* location = NULL,
				   const ServerConfig* server = NULL,
				   const std::string& defaultRoot = "./www",
<<<<<<< HEAD
				   Request* request = NULL);
=======
				   const Request* request = NULL);
>>>>>>> 168ddf900f738217e8c632eab7666959723d69bc

		// Convenience overload for simple cases (no location/server rules).
		void build(const std::string& method,
				   const std::string& path,
				   const std::vector<char>& requestBody,
				   const std::string& webRoot);

		void serveFile(const std::string& filePath);
		void serveCgi(const std::string& scriptPath,
					  const std::string& requestPath,
					  const std::string& method,
					  const std::string& queryString,
				  const std::vector<char>& requestBody,
				  Request* request = NULL);

		void setStatus(int code);
		void setHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);
		void setBody(const std::vector<char>& body);

		int                getStatusCode()    const;
		std::string        getStatusMessage() const;
		const std::string& getBody()          const;

		std::string toString() const;

	private:
		int                                 _statusCode;
		std::string                         _statusMessage;
		std::map<std::string, std::string>  _headers;
		std::string                         _body;

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
<<<<<<< HEAD
						   const ServerConfig* server,
						   Request* request);

=======
							   const ServerConfig* server);
>>>>>>> 168ddf900f738217e8c632eab7666959723d69bc
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
};
