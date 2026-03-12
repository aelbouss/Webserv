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

