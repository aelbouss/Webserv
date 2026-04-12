#include "../inc/CgiHandler.hpp"


CgiHandler::CgiHandler() {
    this->_cgi_path = "";
    this->_cgi_pid = -1;
    this->_exit_status = 0;
    this->_ch_env = NULL;
    this->_argv = NULL;
}

CgiHandler::CgiHandler(std::string path) {
    this->_cgi_path = path;
    this->_cgi_pid = -1;
    this->_exit_status = 0;
    this->_ch_env = NULL;
    this->_argv = NULL;
}

CgiHandler::~CgiHandler() {
    if (this->_ch_env != NULL) {
        for (int i = 0; this->_ch_env[i] != NULL; i++) {
            delete[] this->_ch_env[i];
        }
        delete[] this->_ch_env;
    }
    if (this->_argv != NULL) {
        for (int i = 0; this->_argv[i] != NULL; i++) {
            delete[] this->_argv[i];
        }
        delete[] this->_argv;
    }
    this->_env.clear();
}


// Getters
// pid_t CgiHandler::getCgiPid() const {
//     return this->_cgi_pid;
// }

// int CgiHandler::getExitStatus() const {
//     return this->_exit_status;
// }

// std::string CgiHandler::getCgiPath() const {
//     return this->_cgi_path;
// }

// // Setters
// void CgiHandler::setCgiPid(pid_t pid) {
//     this->_cgi_pid = pid;
// }

// void CgiHandler::setExitStatus(int status) {
//     this->_exit_status = status;
// }

// void CgiHandler::setCgiPath(std::string path) {
//     this->_cgi_path = path;
// }

void CgiHandler::initEnv(HttpRequest& req, const std::vector<Location>::iterator it_loc)
{
	int			poz;
	std::string extension;
	std::string ext_path;

	extension = this->_cgi_path.substr(this->_cgi_path.find("."));
	std::map<std::string, std::string>::iterator it_path = it_loc->_ext_path.find(extension);
    if (it_path == it_loc->_ext_path.end())
        return ;
    ext_path = it_loc->_ext_path[extension];

	this->_env["AUTH_TYPE"] = "Basic";
	this->_env["CONTENT_LENGTH"] = req.getHeader("content-length");
	this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
    this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	poz = findStart(this->_cgi_path, "cgi-bin/");
	this->_env["SCRIPT_NAME"] = this->_cgi_path;
    this->_env["SCRIPT_FILENAME"] = ((poz < 0 || (size_t)(poz + 8) > this->_cgi_path.size()) ? "" : this->_cgi_path.substr(poz + 8, this->_cgi_path.size())); // check dif cases after put right parametr from the response
    this->_env["PATH_INFO"] = getPathInfo(req.getPath(), it_loc->getCgiExtension());
    this->_env["PATH_TRANSLATED"] = it_loc->getRootLocation() + (this->_env["PATH_INFO"] == "" ? "/" : this->_env["PATH_INFO"]);
    this->_env["QUERY_STRING"] = decode(req.getQuery());
    this->_env["REMOTE_ADDR"] = req.getHeader("host");
	poz = findStart(req.getHeader("host"), ":");
    this->_env["SERVER_NAME"] = (poz > 0 ? req.getHeader("host").substr(0, poz) : "");
    this->_env["SERVER_PORT"] = (poz > 0 ? req.getHeader("host").substr(poz + 1, req.getHeader("host").size()) : "");
    this->_env["REQUEST_METHOD"] = req.getMethodStr();
    this->_env["HTTP_COOKIE"] = req.getHeader("cookie");
    this->_env["DOCUMENT_ROOT"] = it_loc->getRootLocation();
	this->_env["REQUEST_URI"] = req.getPath() + req.getQuery();
    this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
    this->_env["REDIRECT_STATUS"] = "200";
	this->_env["SERVER_SOFTWARE"] = "AMANIX";

	this->_ch_env = (char **)calloc(sizeof(char *), this->_env.size() + 1);
	std::map<std::string, std::string>::const_iterator it = this->_env.begin();
	for (int i = 0; it != this->_env.end(); it++, i++)
	{
		std::string tmp = it->first + "=" + it->second;
		this->_ch_env[i] = strdup(tmp.c_str());
	}
	this->_argv = (char **)malloc(sizeof(char *) * 3);
	this->_argv[0] = strdup(ext_path.c_str());
	this->_argv[1] = strdup(this->_cgi_path.c_str());
	this->_argv[2] = NULL;
}

int CgiHandler::findStart(const std::string path , const std::string delimiter)
{
    if(path.empty())
        return (-1);
    size_t pos = path.find(delimiter);
    if( pos != std::string::npos)
        return (pos);
    else 
        return (-1);   
}

std::string getPathInfo(std::string& path , std::vector<std::string> ext)
{
    std::string tmp;
    size_t start;
    size_t end;


    for(std::vector<std::string>::iterator it_ext = ext.begin() ; it_ext != ext.end() ; it_ext++)
    {
        start = path.find(*it_ext);
        if(start != std::string::npos)
            break;
    }
    if(start == std::string::npos)
        return "";
    if(start + 3 >= path.size())
        return "";
    tmp = path.substr(start + 3 , path.size());
    if(!tmp[0] || tmp[0] != '/')
        return "";
    end = tmp.find('?');
    return(end == std::string::npos ? tmp : tmp.substr(0 , end));
}

std::string CgiHandler::decode(std::string &path)
{
	size_t token = path.find("%");
	while (token != std::string::npos)
	{
		if (path.length() < token + 2)
			break ;
		char decimal = fromHexToDec(path.substr(token + 1, 2));
		path.replace(token, 3, toString(decimal));
		token = path.find("%");
	}
	return (path);
}