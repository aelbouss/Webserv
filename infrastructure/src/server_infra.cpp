#include "../inc/server_infra.hpp"


server_infra::server_infra() {}

server_infra::~server_infra() {}

void	server_infra::set_resources(std::vector<engine_resource> & vect){ resources = vect; }

/*
 * this function will create sockets the endppoints communication of the server
 */

void	server_infra::create_sockets()
{
	int	opt;
	for (size_t i = 0 ; i  < resources.size() ; i++)
	{
		sockets.push_back(socket(AF_INET, SOCK_STREAM, 0));
		if (sockets[i] == -1)
			throw SocketExceptions("failed to create socket");
	}
	// prevent the port hanging in the sockets .  
	opt = 1;
	for (size_t i = 0 ; i <  sockets.size() ; i++)
	{
		if ( setsockopt(sockets[i], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
			throw SocketExceptions("setsockopt failed");
		// set sockets  with non blocking 
		set_non_blocking(sockets[i]);
	}
	std::cout << "the sockets are created " << std::endl;
}


/*
 * the routine below will turn on the non blocking state of sockets  
 */

void	server_infra::set_non_blocking(int fd)
{
	int	flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		throw SocketExceptions("failed to get socket flags");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw SocketExceptions("failed to set O_NONBLOCK");
}

/*
 * the bellow function is used to assign sockets with the infos retrived from config file 
 */

void	server_infra::bind_sockets()
{
	// the vector of infos of sockets is  not created yet
	sockaddr_in	infos;

	for(size_t i = 0 ; i < resources.size() ; i++)
	{
		memset(&infos, 0, sizeof(infos));
		infos.sin_family = AF_INET;
		infos.sin_port = htons(resources[i].get_port()) ; 
		const char	*str  = resources[i].get_interface().c_str();
		infos.sin_addr.s_addr = inet_addr(str); // check with the config file to handle (* or 0.0.0.0)
		sockets_infos.push_back(infos);
	}

	for (size_t i = 0 ; i < sockets.size() ; i++)
	{
		if (bind(sockets[i], (struct sockaddr *)&sockets_infos[i], sizeof(sockets_infos[i])) < 0)
			throw SocketExceptions("failed to bind sockets");
	}
	std::cout << "the sockets are binded" << std::endl;
}



void	server_infra::show_resources()
{
	for (size_t i = 0 ; i < resources.size() ; i++)
	{
		std::cout << resources[i].get_interface()  << std::endl;
		std::cout << resources[i].get_port() << std::endl;
		std::cout << resources[i].get_size_body()  << std::endl;
		std::cout << " ========================= " << std::endl;
	}
}

/*
 * the routine bellow puts the socket in the passive mode
 */

void	server_infra::activate_sockets()
{
	for (size_t i = 0 ; i < sockets.size(); i++)
	{
		if (listen(sockets[i], SOMAXCONN) < 0)
			throw SocketExceptions("failed to listen sockets");
	}
	std::cout << "the sockets are ready to accept connections" << std::endl;
}

// the socket exceptions utilities :

SocketExceptions::SocketExceptions(const std::string& Msg)
{
	msg = Msg + " : "  + std::string(strerror(errno));
}

const char * SocketExceptions::what() const throw()
{
	return (msg.c_str()) ;
}

SocketExceptions::~SocketExceptions()  throw() {}