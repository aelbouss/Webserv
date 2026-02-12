# include  <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <cerrno>
# include <iostream>
# include <arpa/inet.h>

int main(void)
{
    int         sock_fd;
    struct      sockaddr_in addr;
    struct      sockaddr_in addr_cln;
    int         connsock_fd;
    socklen_t   cln_len;
    int         opt;
    ssize_t     nb;
    // domain = which ip_address faily
    // which type  of  socket  either  tcp or  udp
    // protocol just set it  to  zero  it choose based on the  type .

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        std::cerr << "Failed To Create A Socket" << std::endl;
        return (-1);
    }
    std::cout  << "the socket created ... " << std::endl;
    // fill the container  that holds  the  essential  informations  for the  socket .
    addr.sin_family = AF_INET ;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    opt = 1;
    if ( setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        std::cerr << "ERROR " << std::endl;
        return (1);
    }
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "Bind syscall failed" << std::endl;
        if (errno == EADDRINUSE)
            std::cerr << "the port 8080 IN USE " << std::endl;
        close(sock_fd);
        return (1);
    }
    std::cout  << "the bind syscall works correctly ..." << std::endl;
    // setup  the  socket  to  accept  incoming  connections .

    while (true)
    {
    
        if (listen(sock_fd, 5) == -1)
        {
            std::cerr << "listen failed ..." << std::endl;
            close(sock_fd);
            return (1);
        }
        std::cout << "lestening on the port 8080" << std::endl;
        std::cout << "the ready to accept incoming connections" << std::endl;
        std::cout << "lest's gooooooooooo \n"<< std::endl;

        cln_len = sizeof(addr_cln);
        connsock_fd = accept(sock_fd, (struct sockaddr *)&addr_cln, &cln_len);
        if(connsock_fd == -1)
        {
            std::cerr << "failed  to  accept  connections " << std::endl;
            close (sock_fd);
            return (-1);
        }
        // std::cout  << "a coonection catched with the IP address  : " << inet_ntoa(addr_cln.sin_addr) << std::endl;
        // std::cout  << "Port : " << ntohs(addr_cln.sin_port) << std::endl
        char        buffer[1024] = {0};
        while (true)
        {
            nb = recv(connsock_fd, buffer, 1024, 0);
            if (nb <= 0)
            {
                std::cout << "Client Desconnected " << std::endl;
                break ;
            }
            std::cout  << "The msg recieved  : "<< buffer << std::endl;
            send(connsock_fd, buffer, 1024, 0);
        }
        close (connsock_fd);
    }
    return (0);
}