#pragma once


//#include "Webserv.hpp"
// #include "CgiHandler.hpp"
#include "ServerConfig.hpp"
#include "ConfigFile.hpp"
class Location
{
	private:
		std::string					_path;
		std::string					_root;
		bool						_autoindex;
		std::string					_index;
		std::vector<short>			_methods; // Method flags ordered as GET, POST, DELETE, PUT, HEAD.
		std::string					_return;
		std::string					_alias;
		std::vector<std::string>	_cgi_path;
		std::vector<std::string>	_cgi_ext;
		unsigned long				_client_max_body_size;

	public:
		std::map<std::string, std::string> _ext_path;

		Location();
		Location(const Location &other);
		Location &operator=(const Location &rhs);
		~Location();

		void setPath(std::string paramet);
		void setRootLocation(std::string paramet);
		void setMethods(std::vector<std::string> methods);
		void setAutoindex(std::string paramet);
		void setIndexLocation(std::string paramet);
		void setReturn(std::string paramet);
		void setAlias(std::string paramet);
		void setCgiPath(std::vector<std::string> path);
		void setCgiExtension(std::vector<std::string> extension);
		void setMaxBodySize(std::string paramet);
		void setMaxBodySize(unsigned long paramet);

		const std::string &getPath() const;
		const std::string &getRootLocation() const;
		const std::vector<short> &getMethods() const;
		const bool &getAutoindex() const;
		const std::string &getIndexLocation() const;
		const std::string &getReturn() const;
		const std::string &getAlias() const;
		const std::vector<std::string> &getCgiPath() const;
		const std::vector<std::string> &getCgiExtension() const;
		const std::map<std::string, std::string> &getExtensionPath() const;
		const unsigned long &getMaxBodySize() const;

		std::string getPrintMethods() const; // for checking only

}; 