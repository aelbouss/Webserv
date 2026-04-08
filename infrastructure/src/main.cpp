# include "../inc/server_infra.hpp"

int	main(void)
{
	server_infra	webserv;

	engine_resource	server1("127.0.0.1", 8080, 100);
	engine_resource	server2("127.0.0.1", 8081, 101);
	engine_resource	server3("127.0.0.1", 8082, 102);
	std::vector <engine_resource> resources;

	resources.push_back(server1);
	resources.push_back(server2);
	resources.push_back(server3);


	webserv.set_resources(resources); // extract resources from config file
	//webserv.show_resources();
	webserv.create_sockets(); // sockest created successfully .
	webserv.bind_sockets(); // bind sockets with infos
	webserv.activate_sockets();
	
	// current  step : apply listen to  all the open sockets 

	// the  steps :
	// listen in ports , accept , handle non blocking fcntl , accept  the  clients .
	
	return (0);
}