#include "includes/client.hpp"
#include "includes/multiplexing.hpp"
#include "includes/request.hpp"
#include "includes/response.hpp"
#include "includes/multi_listener_setup.hpp"
#include "includes/server_infra.hpp"
#include "includes/ConfigParser.hpp"
#include "includes/ServerManager.hpp"
#include <signal.h>

extern volatile sig_atomic_t g_stop;

void sigpipeHandle(int sig) { (void)sig; }
void stopHandle(int sig) { (void)sig; g_stop = 1; }

int	main(int argc, char **argv)
{
	if (argc == 2)
	{
		try
		{
			std::string config;
			ConfigParser cluster;
			ServerManager master;
			signal(SIGPIPE, sigpipeHandle);
			signal(SIGINT, stopHandle);
			signal(SIGTERM, stopHandle);

			config = argv[1];
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
