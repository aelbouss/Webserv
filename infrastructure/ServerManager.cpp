#include "../includes/ServerManager.hpp"
#include <arpa/inet.h>
#include <cstring>

ServerManager::ServerManager() {}

ServerManager::~ServerManager() {}

void ServerManager::setupServers(std::vector<ServerConfig> &servers)
{
    std::vector<engine_resource> resources;
    for (size_t i = 0; i < servers.size(); ++i)
    {
        ServerConfig &s = servers[i];
        struct in_addr in;
        in.s_addr = s.getHost();
        char ipbuf[INET_ADDRSTRLEN];
        const char *ipstr = inet_ntop(AF_INET, &in, ipbuf, sizeof(ipbuf));
        std::string iface = ipstr ? std::string(ipstr) : std::string("127.0.0.1");
        resources.push_back(engine_resource(iface, static_cast<int>(s.getPort()), s.getClientMaxBodySize()));
    }
    _webserv.set_resources(resources);
    _cluster.set_server_configs(servers);
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
