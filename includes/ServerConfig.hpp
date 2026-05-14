#pragma once 

#include "WebServer.hpp"
#include <string>
#include <map>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>

class Location;


class ServerConfig
{
	private:
		unsigned short					_port;
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
		// Server root directory for path resolution.
		const std::string	&getRoot() const;
		int   	getFd();
		// Configured error page mapping by status code.
		const std::map<short, std::string> &getErrorPages() const;
		const std::vector<Location> &getLocations();
		const size_t &getClientMaxBodySize() const;
		const unsigned short &getPort();
		const in_addr_t &getHost() const;
		const bool &getAutoindex();
		const std::string &getServerName() const;
		const std::string &getIndex();
		const std::vector<Location>::iterator getLocationKey(std::string key);
		const std::string &getPathErrorPage(short key);
		// Longest-prefix match for a request path.
		const Location *matchLocation(const std::string &path) const;

		
		//void	setupServer(void);
		bool checkLocaitons() const;
		void checkToken(std::string &paramt);



		
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
					_message = "SERVER CONFIG ERROR " + message;
				}
				virtual const char* what() const throw()
				{
					return (_message.c_str());
				}
				virtual ~ErrorException() throw() {}
		};

};
