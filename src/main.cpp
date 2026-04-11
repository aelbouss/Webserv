#include "../inc/ServerConfig.hpp"


int main(int argc , char argv) {

    if(argc == 1 || argc == 2) {
        std::string     config;
        ConfigParser    cluster;

        config = "configs/default.conf";

        cluster.createCluster(config);
    }
}