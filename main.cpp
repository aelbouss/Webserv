#include "includes/client.hpp"
#include "includes/multiplexing.hpp"
#include "includes/request.hpp"
#include "includes/response.hpp"
#include "includes/multi_listener_setup.hpp"
#include "includes/server_infra.hpp"
#include "includes/ConfigParser.hpp"
#include "includes/ServerManager.hpp"
#include <signal.h>

void sigpipeHandle(int sig) { (void)sig; }

int	main(int argc, char **argv)
{
	if (argc == 1 || argc == 2)
	{
		try
		{
			std::string config;
			ConfigParser cluster;
			ServerManager master;
			signal(SIGPIPE, sigpipeHandle);

			config = (argc == 1 ? "configs/default.conf" : argv[1]);
			cluster.createCluster(config);
			std::vector<ServerConfig> servers = cluster.getServers();
			master.setupServers(servers);
			master.runServers();
		}
		catch (std::exception &e)
		{
			std::cerr << e.what() << std::endl;
			return (1);
		}
	}
	else
	{
		std::cerr << "Error: wrong arguments" << std::endl;
		return (1);
	}
	return (0);
}
