#include "../inc/ServerConfig.hpp"

ServerConfig::ServerConfig()
{
	this->_port =0;
	this->_host = 0;
	this->_server_name = "";
	this->_root = "";
	this->_client_max_body_size = MAX_CONTENT_LENGTH;
	this->_index = "";
	this->_listen_fd = 0;
	this->_autoindex = false;
	this->initErrorPages();
}



ServerConfig::~ServerConfig() { }

/* copy constructor */
ServerConfig::ServerConfig(const ServerConfig &other)
{
	if (this != &other)
	{
		this->_server_name = other._server_name;
		this->_root = other._root;
		this->_host = other._host;
		this->_port = other._port;
		this->_client_max_body_size = other._client_max_body_size;
		this->_index = other._index;
		this->_error_pages = other._error_pages;
		this->_locations = other._locations;
		this->_listen_fd = other._listen_fd;
		this->_autoindex = other._autoindex;
		this->_server_address = other._server_address;

	}
	return ;
}

/* assinment operator */
ServerConfig &ServerConfig::operator=(const ServerConfig & paramt)
{
	if (this != &paramt)
	{
		this->_server_name = paramt._server_name;
		this->_root = paramt._root;
		this->_port = paramt._port;
		this->_host = paramt._host;
		this->_client_max_body_size = paramt._client_max_body_size;
		this->_index = paramt._index;
		this->_error_pages = paramt._error_pages;
		this->_locations = paramt._locations;
		this->_listen_fd = paramt._listen_fd;
		this->_autoindex = paramt._autoindex;
		this->_server_address = paramt._server_address;
	}
	return (*this);
}

void ServerConfig::initErrorPages()
{
    _error_pages[400] = "";
    _error_pages[401] = "";
    _error_pages[403] = "";
    _error_pages[404] = "";
    _error_pages[405] = "";
    _error_pages[408] = "";
    _error_pages[409] = "";
    _error_pages[410] = "";
    _error_pages[413] = "";
    _error_pages[500] = "";
    _error_pages[501] = "";
    _error_pages[502] = "";
    _error_pages[503] = "";
    _error_pages[504] = "";
    _error_pages[505] = "";
}

void	ServerConfig::setServerName(std::string server_name) {
	checkToken(server_name);
	this->_server_name = server_name;
}

void ServerConfig::setHost(std::string parametr)
{
	checkToken(parametr);
	if (parametr == "localhost")
		parametr = "127.0.0.1";
	if (!isValidHost(parametr))
		throw ErrorException("Wrong syntax: host");
	this->_host = inet_addr(parametr.data());
}

void ServerConfig::setRoot(std::string root)
{
	checkToken(root);
	if (ConfigFile::getTypePath(root) == 2)
	{
		this->_root = root;
		return ;
	}
	char dir[1024];
	getcwd(dir, 1024);
	std::string full_root = dir + root;
	if (ConfigFile::getTypePath(full_root) != 2)
		throw ErrorException("Wrong syntax: root");
	this->_root = full_root;
}

bool ServerConfig::isValidHost(std::string host) const
{
    struct sockaddr_in addr;
    int res = inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    return (res == 1);
}

void ServerConfig::checkToken(std::string& paramt)
{
	size_t first , last;
	last = paramt.rfind(';');
	first = paramt.find(';');

	if(first == std::string::npos || first != last)
		throw ErrorException("Invalid token: multiple or missing ';'");
	if(last != paramt.size() - 1)
		throw ErrorException("Invalid token: ';' must be at the end");
	paramt.erase(pos);
}