#include "../inc/Location.hpp"
#include <cstdlib>

Location::Location()
{
	this->_path = "";
	this->_root = "";
	this->_autoindex = false;
	this->_index = "";
	this->_return = "";
	this->_alias = "";
	this->_client_max_body_size = MAX_CONTENT_LENGTH; // you need to back to this after
	this->_methods.reserve(5);
	this->_methods.push_back(1);
	this->_methods.push_back(0);
	this->_methods.push_back(0);
	this->_methods.push_back(0);
	this->_methods.push_back(0);
}

Location::Location(const Location &other)
{
	this->_path = other._path;
	this->_root = other._root;
	this->_autoindex = other._autoindex;
	this->_index = other._index;
	this->_cgi_path = other._cgi_path;
	this->_cgi_ext = other._cgi_ext;
	this->_return = other._return;
	this->_alias = other._alias;
    this->_methods = other._methods;
	this->_ext_path = other._ext_path;
	this->_client_max_body_size = other._client_max_body_size;
}

Location &Location::operator=(const Location &rhs)
{
	if (this != &rhs)
	{
		this->_path = rhs._path;
		this->_root = rhs._root;
		this->_autoindex = rhs._autoindex;
		this->_index = rhs._index;
		this->_cgi_path = rhs._cgi_path;
		this->_cgi_ext = rhs._cgi_ext;
		this->_return = rhs._return;
		this->_alias = rhs._alias;
		this->_methods = rhs._methods;
		this->_ext_path = rhs._ext_path;
		this->_client_max_body_size = rhs._client_max_body_size;
    }
	return (*this);
}

Location::~Location() { }

/* set functions */
void Location::setPath(std::string paramet)
{
	this->_path = paramet;
}

void Location::setRootLocation(std::string paramet)
{
	if (ConfigFile::getTypePath(paramet) != 2)
		throw ServerConfig::ErrorException("root of location");
	this->_root = paramet;
}

void Location::setMethods(std::vector<std::string> methods)
{
	this->_methods[0] = 0;
	this->_methods[1] = 0;
	this->_methods[2] = 0;
	this->_methods[3] = 0;
	this->_methods[4] = 0;

	for (size_t i = 0; i < methods.size(); i++)
	{
		if (methods[i] == "GET")
			this->_methods[0] = 1;
		else if (methods[i] == "POST")
			this->_methods[1] = 1;
		else if (methods[i] == "DELETE")
			this->_methods[2] = 1;
		else if (methods[i] == "PUT")
			this->_methods[3] = 1;
		else if (methods[i] == "HEAD")
			this->_methods[4] = 1;
		else
			throw ServerConfig::ErrorException("Allow method not supported " + methods[i]);
	}
}

void Location::setAutoindex(std::string paramet)
{
	if (paramet == "on" || paramet == "off")
		this->_autoindex = (paramet == "on");
	else
		throw ServerConfig::ErrorException("Wrong autoindex");
}

void Location::setIndexLocation(std::string paramet)
{
	this->_index = paramet;
}

void Location::setReturn(std::string paramet)
{
	this->_return = paramet;
}

void Location::setAlias(std::string paramet)
{
	this->_alias = paramet;
}

void Location::setCgiPath(std::vector<std::string> path)
{
	this->_cgi_path = path;
}

void Location::setCgiExtension(std::vector<std::string> extension)
{
	this->_cgi_ext = extension;
}

void Location::setMaxBodySize(std::string paramet)
{
	unsigned long body_size = 0;
	char *end_ptr;

	for (size_t i = 0; i < paramet.length(); i++)
	{
		if (paramet[i] < '0' || paramet[i] > '9')
			throw ServerConfig::ErrorException("Wrong syntax: client_max_body_size");
	}
	end_ptr = NULL;
	body_size = std::strtoul(paramet.c_str(), &end_ptr, 10);
	if (*end_ptr != '\0' || body_size == 0)
		throw ServerConfig::ErrorException("Wrong syntax: client_max_body_size");
	this->_client_max_body_size = body_size;
}

void Location::setMaxBodySize(unsigned long paramet)
{
	this->_client_max_body_size = paramet;
}

/* get functions */
const std::string &Location::getPath() const
{
	return (this->_path);
}

const std::string &Location::getRootLocation() const
{
	return (this->_root);
}

const std::string &Location::getIndexLocation() const
{
	return (this->_index);
}

const std::vector<short> &Location::getMethods() const
{
	return (this->_methods);
}

const std::vector<std::string> &Location::getCgiPath() const
{
	return (this->_cgi_path);
}

const std::vector<std::string> &Location::getCgiExtension() const
{
	return (this->_cgi_ext);
}

const bool &Location::getAutoindex() const
{
	return (this->_autoindex);
}

const std::string &Location::getReturn() const
{
	return (this->_return);
}

const std::string &Location::getAlias() const
{
	return (this->_alias);
}

const std::map<std::string, std::string> &Location::getExtensionPath() const
{
	return (this->_ext_path);
}

const unsigned long &Location::getMaxBodySize() const
{
	return (this->_client_max_body_size);
}

/* Build a comma-separated method list for debug and config diagnostics. */
std::string Location::getPrintMethods() const
{
    std::string res;
    const char* names[5] = {"GET", "POST", "DELETE", "PUT", "HEAD"};

    for (size_t i = 0; i < 5; i++)
    {
        if (_methods[i])
        {
            if (!res.empty())
                res += ", ";
            res += names[i];
        }
    }
    return res;
}


