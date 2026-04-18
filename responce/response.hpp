#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include "CgiHandler"

class Response
{
    public:

        Response();
        Response(int statusCode, const std::string& body,
                 const std::string& contentType);

        void build(const std::string& method,
                   const std::string& path,
                   const std::vector<char>& requestBody,
                   const std::string& webRoot = "./www");

       
        void serveFile(const std::string& filePath);
        void serveCgi(const std::string& scriptPath,
                      const std::string& method,
                      const std::string& queryString,
                      const std::vector<char>& requestBody);

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

        void buildErrorPage(int code);

        static std::string statusMessage(int code);
        static std::string mimeType(const std::string& path);
        static bool        fileExists(const std::string& path);
        static std::string readFile(const std::string& path);
        static std::string intToStr(size_t n);
};