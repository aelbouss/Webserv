#pragma once 

#include <string>

class CgiHandler{

    private:
        std::string _cgi_path;
        pid_t       _cgi_pid;
        int         _exit_status;
        char**     _ch_env;
        char**     _argv;
        std::map<std::string, std::string> _env;
    public:
        CgiHandler();
        CgiHandler(std::string path);
        ~CgiHandler();
};