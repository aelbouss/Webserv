#pragma once

#include "WebServer.hpp"
#include "server_infra.hpp"
#include "multiplexing.hpp"
#include <vector>

class ServerManager
{
public:
    ServerManager();
    ~ServerManager();

    void setupServers(std::vector<ServerConfig> &servers);
    void runServers();

private:
    server_infra _webserv;
    multiplexing _cluster;
};
