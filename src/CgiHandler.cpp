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
pid_t CgiHandler::getCgiPid() const {
    return this->_cgi_pid;
}

int CgiHandler::getExitStatus() const {
    return this->_exit_status;
}

std::string CgiHandler::getCgiPath() const {
    return this->_cgi_path;
}

// Setters
void CgiHandler::setCgiPid(pid_t pid) {
    this->_cgi_pid = pid;
}

void CgiHandler::setExitStatus(int status) {
    this->_exit_status = status;
}

void CgiHandler::setCgiPath(std::string path) {
    this->_cgi_path = path;
}