#include "../includes/ServerManager.hpp"
#include <arpa/inet.h>
#include <cstring>

ServerManager::ServerManager() {}

ServerManager::~ServerManager() {}

void ServerManager::setupServers(std::vector<ServerConfig> &servers)
{
    std::vector<engine_resource> resources;
    std::vector< std::vector<size_t> > groups;

    for (size_t i = 0; i < servers.size(); ++i)
    {
        ServerConfig &s = servers[i];
        struct in_addr in;
        in.s_addr = s.getHost();
        char ipbuf[INET_ADDRSTRLEN];
        const char *ipstr = inet_ntop(AF_INET, &in, ipbuf, sizeof(ipbuf));
        std::string iface = ipstr ? std::string(ipstr) : std::string("127.0.0.1");
        int port = static_cast<int>(s.getPort());

        size_t idx = 0;
        bool found = false;
        for (; idx < resources.size(); ++idx)
        {
            if (resources[idx].get_interface() == iface && resources[idx].get_port() == port)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            resources.push_back(engine_resource(iface, port, s.getClientMaxBodySize()));
            groups.push_back(std::vector<size_t>());
            idx = resources.size() - 1;
        }
        groups[idx].push_back(i);
    }

    _webserv.set_resources(resources);
    _cluster.set_server_configs(servers);
    _cluster.set_server_groups(groups);
}

void ServerManager::runServers()
{
    _webserv.create_sockets();
    _webserv.bind_sockets();
    _webserv.activate_sockets();

    _cluster.set_master_sockets(_webserv);
    _cluster.prepare_master_sockets();
    _cluster.cluster_controlling();
}
