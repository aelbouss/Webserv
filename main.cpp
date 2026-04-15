# include "includes/client.hpp"
# include "includes/multiplexing.hpp"
# include "includes/request.hpp"
# include "includes/response.hpp"
# include "includes/multi_listener_setup.hpp"
# include "includes/server_infra.hpp"


int	main(void)
{
	server_infra	webserv;
	multiplexing	cluster;
	

	engine_resource	server1("127.0.0.1", 8080, 100);
	engine_resource	server2("127.0.0.1", 8081, 101);
	engine_resource	server3("127.0.0.1", 8082, 102);
	std::vector <engine_resource> resources;

	resources.push_back(server1);
	resources.push_back(server2);
	resources.push_back(server3);

	try
	{
		// infra part
		webserv.set_resources(resources); // extract resources from config file
		//webserv.show_resources();
		webserv.create_sockets(); // sockest created successfully .
		webserv.bind_sockets(); // bind sockets with infos
		webserv.activate_sockets();
		//webserv.close_sockets(); // close  sockets functions 

		
		// multiplexing part 
		cluster.set_master_sockets(webserv);
		cluster.prepare_master_sockets();
		cluster.cluster_controlling();
	}

	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl ;
	}

	return (0);
}

// must set also set the clients  sockets with the non blocking