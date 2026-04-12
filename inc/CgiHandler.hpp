#pragma once 


#include "WebServer.hpp"

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
        // pid_t getCgiPid() const;
        // int getExitStatus() const;
        // std::string getCgiPath() const;

        void            initEnv(HttpRequest& req , const std::vector<Location>::iterator it_loc);
        std::string     decode(std::string &path);

        // // Setters
        // void setCgiPid(pid_t pid);
        // void setExitStatus(int status);
        // void setCgiPath(std::string path);















        void setEnv(std::map<std::string, std::string> env);
        int findStart(const std::string path, const std::string delimiter); 
        std::string getPathInfo(std::string& path , std::vector<std::string> ext);

};