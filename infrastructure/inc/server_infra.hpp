# ifndef SERVER_INFRA_HPP
# define SERVER_INFRA_HPP

# include <iostream>
# include <unistd.h>
# include <vector>
# include <sys/socket.h>
# include <netinet/in.h>

# include "multi_listener_configuration.hpp"


// the class of the infrastructure for the server

class server_infra
{
	private:
			
			std::vector <engine_resource> resources; // the pair of interface and port for every server block 
			std::vector <int> sockets ;  // hold all the lestensing master sockets .
			std::vector <sockaddr_in> sockets_infos; // a vector of  the informations must be bind to each socket
	public:
		server_infra(/* args */);
		~server_infra();
		server_infra(const server_infra& src);
		server_infra&	operator = (server_infra& src);
		void	set_resources(vector<engine_resource> & vect);
		void	create_sockets();
		void	assigns_infos_to_sockets();
};
# endif