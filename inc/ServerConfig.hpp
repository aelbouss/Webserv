/*
** Server-level configuration model and validation helpers.
**
** This class owns normalized server directives parsed from config files,
** validates tokens and paths, and builds per-location runtime settings.
*/

#pragma once 

#include <arpa/inet.h>
#include <string>
#include <vector>
#include <map>
#include "ConfigFile.hpp"
#include "ConfigParser.hpp"
//#include "LocationConfig.hpp"
//#include "Location.hpp"

#define MAX_CONTENT_LENGTH (1024 * 1024)  // 1MB default
class Location;


class ServerConfig
{
	private:
		uint16_t						_port;
		in_addr_t						_host;
		std::string						_server_name;
		std::string						_root;
		unsigned long					_client_max_body_size;
		std::string						_index;
		bool							_autoindex;
		std::map<short, std::string>	_error_pages;
		std::vector<Location> 			_locations;
        struct sockaddr_in 				_server_address;
        int     						_listen_fd;

	public:
		ServerConfig();
		~ServerConfig();
		ServerConfig(const ServerConfig &other);
		ServerConfig &operator=(const ServerConfig & rhs);

		void initErrorPages(void);
        void checkToken(std::string& paramt);



        //Setter
        void    setServerName(std::string server_name);
		void	setHost(std::string host);
		void 	setRoot(std::string root);
		void 	setPort(std::string parametr);
		void	setClientMaxBodysize(std::string paramt);
		void 	setIndex(std::string index);
		void	setAutoindex(std::string autoindex);
		void	setFd(int fd);



		//Getter
		const std::string	&getRoot();
		int   	getFd();
		const std::map<short, std::string> &getErrorPages();
		const std::vector<Location> &getLocations();
		const size_t &getClientMaxBodySize();
		const uint16_t &getPort();
		const in_addr_t &getHost();
		const bool &getAutoindex();
		const std::string &getServerName();
		const std::string &getIndex();
		const std::vector<Location>::iterator getLocationKey(std::string key);
		const std::string &getPathErrorPage(short key);

		
		void	setupServer(void);
		bool checkLocaitons() const;
		void checkToken(std::string &parametr);



		
		bool 	isValidHost(std::string host) const;
		bool	isValidErrorPages();
		void	setErrorPages(std::vector<std::string> &paramet);
		void	setLocation(std::string path, std::vector<std::string> paramet);
		int 	isValidLocation(Location &location) const;
		public:
		class ErrorException : public std::exception
		{
			private:
				std::string _message;
			public:
				ErrorException(std::string message) throw()
				{
					_message = "SERVER CONFIG ERROR" + message;
				}
				virtual const char* what() const throw()
				{
					return (_message.c_str());
				}
				virtual ~ErrorException() throw() {}
		};

};

		//Utils
		std::string statusCodeString(short statusCode);