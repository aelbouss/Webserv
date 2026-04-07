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



	 // 1 extract resources 
	 // 2 create sockets 
	 // 3 bind infos to sockets 

	webserv.set_resources(resources);
	webserv.create_sockets();
	webserv.assigns_infos_to_sockets();




	return (0);
}