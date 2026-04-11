#include "../inc/ServerConfig.hpp"

int main(int argc, char **argv)
{
    try
    {
        std::string config;
        ConfigParser cluster;

        if (argc == 2)
            config = argv[1];
        else if (argc == 1)
        {
            config = "configs/default.conf";
            if (ConfigFile::getTypePath(config) != 1)
                config = "../configs/default.conf";
        }
        else
            return (1);

        cluster.createCluster(config);
        cluster.print();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}