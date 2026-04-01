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

void CgiHandler::setCgiPath(std::string path) {
    this->_cgi_path = path;
}

void CgiHandler::initEnv(HttpRequest& req, const std::vector<Location>::iterator it_loc) 
{
    std::string extension;
    std::string ext_path;
    int pos;

    extension = this->_cgi_path.substr(this->_cgi_path.find("."));
    std::map<std::string, std::string>::iterator it_path = it_loc->_ext_path().find(extension);
    if(it_path == it_loc->_ext_path.end())
        return ;
    ext_path = it_path->_ext_path[extension];

    this->_env["AUTH_TYPE"] = "Basic";
    this->_env["CONTENT_LENGTH"] = req.getHeader("content-length");
    this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
    this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
    pos = findStart(this->_cgi_path , "cgi-bin/");
    this->_env["SCRIPT_NAME"] = this->_cgi_path;
    this->_env["SCRIPT_FILENAME"] = ((pos < 0 || (size_t)(pos + 8) > this->_cgi_path.size()) ? "" : this->_cgi_path.substr(pos + 8, this->_cgi_path.size())); // check dif cases after put right parametr from the response
    this->_env["PATH_INFO"] = getPathInfo(req.getPath(), it_loc->getCgiExtension());
    this->_env["PATH_TRANSLATED"] = it_loc->getRootLocation() + (this->_env["PATH_INFO"] == "" ? "/" : this->_env["PATH_INFO"]);
    this->_env["QUERY_STRING"] = decode(req.getQuery());
    this->_env["REMOTE_ADDR"] = req.getHeader("host");


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