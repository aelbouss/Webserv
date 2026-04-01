#pragma once 

#include <arpa/inet.h>
#include <string>
#include <vector>
#include <map>
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







		
		bool ServerConfig::isValidHost(std::string host) const

};