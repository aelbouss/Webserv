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

void ServerConfig::setHost(std::string paramet)
{
	checkToken(paramet);
	if (paramet == "localhost")
		paramet = "127.0.0.1";
	if (!isValidHost(paramet))
		throw ErrorException("Wrong syntax: host");
	this->_host = inet_addr(paramet.data());
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

void ServerConfig::setPort(std::string paramet)
{
	unsigned int port;
	char *end_ptr;
	unsigned long parsed;
	
	port = 0;
	checkToken(paramet);
	for (size_t i = 0; i < paramet.length(); i++)
	{
		if (!std::isdigit(paramet[i]))
			throw ErrorException("Wrong syntax: port");
	}
	end_ptr = NULL;
	parsed = std::strtoul(paramet.c_str(), &end_ptr, 10);
	if (*end_ptr != '\0' || parsed < 1 || parsed > 65535)
		throw ErrorException("Wrong syntax: port");
	port = static_cast<unsigned int>(parsed);
	this->_port = (uint16_t) port;
}

bool	 ServerConfig::isValidHost(std::string host) const
{
    struct sockaddr_in addr;
    int res = inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    return (res == 1);
}

void	ServerConfig::setClientMaxBodysize(std::string paramt)
{
	unsigned long body_size = 0;

	checkToken(paramt);
	if (paramt.empty())
		throw ErrorException("Wrong syntax: client_max_body_size");
	for (size_t i = 0; i < paramt.length(); i++)
	{
		if (paramt[i] < '0' || paramt[i] > '9')
			throw ErrorException("Wrong syntax: client_max_body_size");
	}
	try
	{
		body_size = std::stoul(paramt);
	}
	catch (const std::exception&)
	{
		throw ErrorException("Wrong syntax: client_max_body_size");
	}
	if (!body_size)
		throw ErrorException("Wrong syntax: client_max_body_size");
	this->_client_max_body_size = body_size;
}

void ServerConfig::setIndex(std::string index)
{
	checkToken(index);
	this->_index = index;
}

void ServerConfig::setAutoindex(std::string autoindex)
{
	checkToken(autoindex);
	if (autoindex != "on" && autoindex != "off")
		throw ErrorException("Wrong syntax: autoindex");
	if (autoindex == "on")
		this->_autoindex = true;
}

void ServerConfig::setErrorPages(std::vector<std::string> &paramet)
{
	if (paramet.empty())
		return;
	if (paramet.size() % 2 != 0)
		throw ErrorException ("Error page initialization faled");
	for (size_t i = 0; i < paramet.size() - 1; i++)
	{
		for (size_t j = 0; j < paramet[i].size(); j++) {
			if (!std::isdigit(paramet[i][j]))
				throw ErrorException("Error code is invalid");
		}
		if (paramet[i].size() != 3)
			throw ErrorException("Error code is invalid");
		short code_error = std::stoi(paramet[i]);
		if (statusCodeString(code_error)  == "Undefined" || code_error < 400)
			throw ErrorException ("Incorrect error code: " + paramet[i]);
		i++;
		std::string path = paramet[i];
		checkToken(path);
		if (ConfigFile::getTypePath(path) != 2)
		{
			if (ConfigFile::getTypePath(this->_root + path) != 1)
				throw ErrorException ("Incorrect path for error page file: " + this->_root + path);
			if (ConfigFile::checkFile(this->_root + path, 0) == -1 || ConfigFile::checkFile(this->_root + path, 4) == -1)
				throw ErrorException ("Error page file :" + this->_root + path + " is not accessible");
		}
		std::map<short, std::string>::iterator it = this->_error_pages.find(code_error);
		if (it != _error_pages.end())
			this->_error_pages[code_error] = path;
		else
			this->_error_pages.insert(std::make_pair(code_error, path));
	}
}

void ServerConfig::setLocation(std::string path, std::vector<std::string> paramet)
{
	Location new_location;
	std::vector<std::string> methods;
	bool flag_methods = false;
	bool flag_autoindex = false;
	bool flag_max_size = false;
	int valid;

	new_location.setPath(path);
	for (size_t i = 0; i < paramet.size(); i++)
	{
		if (paramet[i] == "root" && (i + 1) < paramet.size())
		{
			if (!new_location.getRootLocation().empty())
				throw ErrorException("Root of location is duplicated");
			checkToken(paramet[++i]);
			if (ConfigFile::getTypePath(paramet[i]) == 2)
				new_location.setRootLocation(paramet[i]);
			else
				new_location.setRootLocation(this->_root + paramet[i]);
		}
		else if ((paramet[i] == "allow_methods" || paramet[i] == "methods") && (i + 1) < paramet.size())
		{
			if (flag_methods)
				throw ErrorException("Allow_methods of location is duplicated");
			// Method tokens are collected until ';' and validated below.
			while (++i < paramet.size())
			{
				if (paramet[i].find(";") != std::string::npos)
				{
					checkToken(paramet[i]);
					methods.push_back(paramet[i]);
					break ;
				}
				else
				{
					methods.push_back(paramet[i]);
					if (i + 1 >= paramet.size())
						throw ErrorException("Token is invalid");
				}
			}
			new_location.setMethods(methods);
			flag_methods = true;
		}
		else if (paramet[i] == "autoindex" && (i + 1) < paramet.size())
		{
			if (path == "/cgi-bin")
				throw ErrorException("paramet autoindex not allow for CGI");
			if (flag_autoindex)
				throw ErrorException("Autoindex of location is duplicated");
			checkToken(paramet[++i]);
			new_location.setAutoindex(paramet[i]);
			flag_autoindex = true;
		}
		else if (paramet[i] == "index" && (i + 1) < paramet.size())
		{
			if (!new_location.getIndexLocation().empty())
				throw ErrorException("Index of location is duplicated");
			checkToken(paramet[++i]);
			new_location.setIndexLocation(paramet[i]);
		}
		else if (paramet[i] == "return" && (i + 1) < paramet.size())
		{
			if (path == "/cgi-bin")
				throw ErrorException("paramet return not allow for CGI");
			if (!new_location.getReturn().empty())
				throw ErrorException("Return of location is duplicated");
			checkToken(paramet[++i]);
			new_location.setReturn(paramet[i]);
		}
		else if (paramet[i] == "alias" && (i + 1) < paramet.size())
		{
			if (path == "/cgi-bin")
				throw ErrorException("paramet alias not allow for CGI");
			if (!new_location.getAlias().empty())
				throw ErrorException("Alias of location is duplicated");
			checkToken(paramet[++i]);
			new_location.setAlias(paramet[i]);
		}
		else if (paramet[i] == "cgi_ext" && (i + 1) < paramet.size())
		{
			std::vector<std::string> extension;
			while (++i < paramet.size())
			{
				if (paramet[i].find(";") != std::string::npos)
				{
					checkToken(paramet[i]);
					extension.push_back(paramet[i]);
					break ;
				}
				else
				{
					extension.push_back(paramet[i]);
					if (i + 1 >= paramet.size())
						throw ErrorException("Token is invalid");
				}
			}
			new_location.setCgiExtension(extension);
		}
		else if (paramet[i] == "cgi_path" && (i + 1) < paramet.size())
		{
			std::vector<std::string> path;
			while (++i < paramet.size())
			{
				if (paramet[i].find(";") != std::string::npos)
				{
					checkToken(paramet[i]);
					path.push_back(paramet[i]);
					break ;
				}
				else
				{
					path.push_back(paramet[i]);
					if (i + 1 >= paramet.size())
						throw ErrorException("Token is invalid");
				}
				if (paramet[i].find("/python") == std::string::npos && paramet[i].find("/bash") == std::string::npos)
					throw ErrorException("cgi_path is invalid");
			}
			new_location.setCgiPath(path);
		}
		else if (paramet[i] == "client_max_body_size" && (i + 1) < paramet.size())
		{
			if (flag_max_size)
				throw ErrorException("Maxbody_size of location is duplicated");
			checkToken(paramet[++i]);
			new_location.setMaxBodySize(paramet[i]);
			flag_max_size = true;
		}
		else if (i < paramet.size())
			throw ErrorException("paramet in a location is invalid");
	}
	if (new_location.getPath() != "/cgi-bin" && new_location.getIndexLocation().empty())
		new_location.setIndexLocation(this->_index);
	if (!flag_max_size)
		new_location.setMaxBodySize(this->_client_max_body_size);
	valid = isValidLocation(new_location);
	if (valid == 1)
		throw ErrorException("Failed CGI validation");
	else if (valid == 2)
		throw ErrorException("Failed path in locaition validation");
	else if (valid == 3)
		throw ErrorException("Failed redirection file in locaition validation");
	else if (valid == 4)
		throw ErrorException("Failed alias file in locaition validation");
	this->_locations.push_back(new_location);
}

int ServerConfig::isValidLocation(Location &location) const
{
	if (location.getPath() == "/cgi-bin")
	{
		if (location.getCgiPath().empty() || location.getCgiExtension().empty() || location.getIndexLocation().empty())
			return (1);


		if (ConfigFile::checkFile(location.getIndexLocation(), 4) < 0)
		{
			std::string path = location.getRootLocation() + location.getPath() + "/" + location.getIndexLocation();
			if (ConfigFile::getTypePath(path) != 1)
			{				
				std::string root = getcwd(NULL, 0);
				location.setRootLocation(root);
				path = root + location.getPath() + "/" + location.getIndexLocation();
			}
			if (path.empty() || ConfigFile::getTypePath(path) != 1 || ConfigFile::checkFile(path, 4) < 0)
				return (1);
		}
		if (location.getCgiPath().size() != location.getCgiExtension().size())
			return (1);
		std::vector<std::string>::const_iterator it;
		for (it = location.getCgiPath().begin(); it != location.getCgiPath().end(); ++it)
		{
			if (ConfigFile::getTypePath(*it) < 0)
				return (1);
		}
		std::vector<std::string>::const_iterator it_path;
		for (it = location.getCgiExtension().begin(); it != location.getCgiExtension().end(); ++it)
		{
			std::string tmp = *it;
			if (tmp != ".py" && tmp != ".sh" && tmp != "*.py" && tmp != "*.sh")
				return (1);
			for (it_path = location.getCgiPath().begin(); it_path != location.getCgiPath().end(); ++it_path)
			{
				std::string tmp_path = *it_path;
				if (tmp == ".py" || tmp == "*.py")
				{
					if (tmp_path.find("python") != std::string::npos)
						location._ext_path.insert(std::make_pair(".py", tmp_path));
				}
				else if (tmp == ".sh" || tmp == "*.sh")
				{
					if (tmp_path.find("bash") != std::string::npos)
						location._ext_path[".sh"] = tmp_path;
				}
			}
		}
		if (location.getCgiPath().size() != location.getExtensionPath().size())
			return (1);
	}
	else
	{
		if (location.getPath()[0] != '/')
			return (2);
		if (location.getRootLocation().empty()) {
			location.setRootLocation(this->_root);
		}
		if (ConfigFile::isFileExistAndReadable(location.getRootLocation() + location.getPath() + "/", location.getIndexLocation()))
			return (5);
		if (!location.getReturn().empty())
		{
			if (ConfigFile::isFileExistAndReadable(location.getRootLocation(), location.getReturn()))
				return (3);
		}
		if (!location.getAlias().empty())
		{
			if (ConfigFile::isFileExistAndReadable(location.getRootLocation(), location.getAlias()))
			 	return (4);
		}
	}
	return (0);
}

void	ServerConfig::setFd(int fd) {
	this->_listen_fd = fd;
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
	paramt.erase(last);
}

bool ServerConfig::isValidErrorPages()
{
	std::map<short, std::string>::const_iterator it;
	for (it = this->_error_pages.begin(); it != this->_error_pages.end(); it++)
	{
		if (it->first < 100 || it->first > 599)
			return (false);
		if (ConfigFile::checkFile(getRoot() + it->second, 0) < 0 || ConfigFile::checkFile(getRoot() + it->second, 4) < 0)
			return (false);
	}
	return (true);
}

// Accessors
const std::string &ServerConfig::getServerName()
{
	return (this->_server_name);
}

const std::string &ServerConfig::getRoot()
{
	return (this->_root);
}

const bool &ServerConfig::getAutoindex()
{
	return (this->_autoindex);
}

const in_addr_t &ServerConfig::getHost()
{
	return (this->_host);
}

const uint16_t &ServerConfig::getPort()
{
	return (this->_port);
}

const size_t &ServerConfig::getClientMaxBodySize()
{
	return (this->_client_max_body_size);
}

const std::vector<Location> &ServerConfig::getLocations()
{
	return (this->_locations);
}

const std::map<short, std::string> &ServerConfig::getErrorPages()
{
	return (this->_error_pages);
}

const std::string &ServerConfig::getIndex()
{
	return (this->_index);
}

int   	ServerConfig::getFd() 
{ 
	return (this->_listen_fd); 
}

const std::string &ServerConfig::getPathErrorPage(short key)
{
	std::map<short, std::string>::iterator it = this->_error_pages.find(key);
	if (it == this->_error_pages.end())
		throw ErrorException("Error_page does not exist");
	return (it->second);
}

// Detect duplicate location paths within the same server block.
bool ServerConfig::checkLocaitons() const
{
	if (this->_locations.size() < 2)
		return (false);
	std::vector<Location>::const_iterator it1;
	std::vector<Location>::const_iterator it2;
	for (it1 = this->_locations.begin(); it1 != this->_locations.end() - 1; it1++) {
		for (it2 = it1 + 1; it2 != this->_locations.end(); it2++) {
			if (it1->getPath() == it2->getPath())
				return (true);
		}
	}
	return (false);
}
