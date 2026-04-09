# ifndef SERVER_INFRA_HPP
# define SERVER_INFRA_HPP

# include <iostream>
# include <unistd.h>
# include <vector>
# include <sys/socket.h>
# include <netinet/in.h>
# include <vector>
# include <cstring>
# include <arpa/inet.h>
# include <cstdlib>
# include <fcntl.h>
# include <exception>
# include "errno.h"

# include "multi_listener_setup.hpp"

// the class of the infrastructure for the server


class	SocketExceptions :public std::exception
{
	private:
		std::string	msg;

	public:

		SocketExceptions(const std::string& Msg);
		virtual  ~SocketExceptions() throw() ;
		virtual	const char * what() const throw() ;
};

class server_infra
{
	private:
			
			std::vector <engine_resource> resources; // the pair of interface and port for & max body size every server block 
			std::vector <int> sockets ;  // hold all the lestensing master sockets .
			std::vector <sockaddr_in> sockets_infos; // a vector of  the informations must be bind to each socket
	public:
		server_infra(/* args */);
		~server_infra();
		server_infra(const server_infra& src);
		server_infra&	operator = (server_infra& src);
		void	set_resources(std::vector<engine_resource> & vect);
		void	create_sockets();
		void	set_non_blocking(int fd);
		void	bind_sockets();
		void	show_resources();
		void	activate_sockets();
};
# endif