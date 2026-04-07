#include "../inc/server_infra.hpp"


server_infra::server_infra() {}


server_infra::~server_infra() {}

void	server_infra::set_resources(vector<engine_resource> & vect)
{
	for (size_t i = 0 ; i < vect.size() ; i++)
		resources[i] = vect[i];
}

/*
 * this function will create sockets the endppoints communication of the server
 */

void	server_infra::create_sockets()
{
	for (size_t i = 0 ; i  < resources.size() ; i++)
	{
		sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (sockets[i] == -1)
		{
			std::cout << "socket creation failed "<< std::endl;
			return (-1);
		}
	}
}

/*
 * the bellow function is used to assign sockets with the infos retrived from config file 
 */

void	server_infra::assigns_infos_to_sockets()
{
	for(size_t i = 0 ; i < resources.size() ; i++)
	{
		memset(&sockets_infos[i], 0, sizeof(sockets_infos[i]));
		sockets_infos[i].sin_family = AF_INET;
		sockets_infos[i].sin_port = htons(resources[i].port) ; // need getter 
		sockets_infos[i].sin_addr.s_addr = inet_addr(resources[i].interface) // need getter
	}
}
