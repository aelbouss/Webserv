#pragma once 

#include <string>
#include <iostream>
#include <map>


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

        // Getters
        pid_t getCgiPid() const;
        int getExitStatus() const;
        std::string getCgiPath() const;



        // Setters
        void setCgiPid(pid_t pid);
        void setExitStatus(int status);
        void setCgiPath(std::string path);
        void setEnv(std::map<std::string, std::string> env);
};