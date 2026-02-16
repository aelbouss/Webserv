# include  <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <cerrno>
# include <iostream>
# include <arpa/inet.h>
# include <fcntl.h>
# include <vector>
# include <poll.h>
# include <bits/stdc++.h>

int main(void)
{
    int         sock_fd;
    struct      sockaddr_in addr;
    struct      sockaddr_in addr_cln;
    int         cln_fd;
    socklen_t   cln_len;
    int         opt;
    ssize_t     nb;
    int ret_p;
    char   buffer[1024];

    /*
     * create  the  socket  which is  the connection end 
     */

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        std::cerr << "Failed To Create A Socket" << std::endl;
        return (-1);
    }
     fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    std::cout  << "the socket created ... " << std::endl;
    // fill the container  that holds  the  essential  informations  for the  socket .
    addr.sin_family = AF_INET ;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    opt = 1;

    /*
     * allow  the  socket to  be reused
     */
    if ( setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        std::cerr << "ERROR " << std::endl;
        return (1);
    }

    /*
     *  bind an ip address and a port to the socket to be visible in the network 
     */

    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "Bind syscall failed" << std::endl;
        if (errno == EADDRINUSE)
            std::cerr << "the port 8080 IN USE " << std::endl;
        close(sock_fd);
        return (1);
    }

    /*
     *  turn  the  flag of  the  socket  of pasive make  it accept connections
     *  create a queue that  will holds  the  pending  connections . 
     */

    if (listen(sock_fd, 5) == -1)
    {
        std::cerr << "listen failed ..." << std::endl;
        close(sock_fd);
        return (1);
    }
    cln_len = sizeof(addr_cln);


    // ================  one  client  per  time ==================================

    // while (true)
    // {

    //     /*
    //      * the os  take the  connection  handle  their connection put them in the queue created by listen  
    //      * the accept syscall pick up the clients in the queue and conect with them
    //      * give each new client a badge (new socket) to use it for the connection
    //      */

    //     cln_fd = accept(sock_fd, (struct sockaddr *)&addr_cln, &cln_len) ;
    //     if (cln_fd == -1)
    //     {
    //         std::cerr << "Bad socket something went wrong" << std::endl;
    //         close(sock_fd);
    //         return (1);
    //     }
    //     memset(buffer, '\0', sizeof(buffer));
    //     while (true)
    //     {
    //         nb = recv(cln_fd, buffer, sizeof(buffer), 0);
    //         if (nb <= 0)
    //         {
    //             std::cout << "The connection finished [* _ *]" << std::endl;
    //             close(cln_fd);
    //             break;
    //         }
    //         std::cout << "The reieved msg is : "<< buffer<< std::endl;
    //          memset(buffer, '\0', sizeof(buffer));
    //     }
    //     std::cout  << "returned here" << std::endl;
    // }


    // ============================ multi client per time =========================================

    /*
     *  handle multiple  clients simultaneously
     *  multiplexing
     */

    std::vector <struct pollfd > fds;
    struct  pollfd lsn;
    struct  pollfd new_cln;
    int     new_badge;
    int     pr;


    lsn.fd = sock_fd;
    lsn.events = POLLIN;

    fds.push_back(lsn);



    /*
     * a server  that handles  many  clients  simultaneously .
     */

    while (true)
    {
        pr = poll(&fds[0], fds.size(), -1);

        for (size_t i = 0 ; i < fds.size() ; i++)
        {
            if (fds[i].revents == 0)
                continue;

            /* add a new client to the watched  list */

            /* actually there is a  dta  to read */
            if (fds[i].revents & POLLIN)
            {
                /* if  it's a  new client */
                if (fds[i].fd == sock_fd)
                {
                    new_badge = accept(fds[i].fd , NULL, NULL);
                    if(new_badge > 0)
                     fcntl(new_badge, F_SETFL, O_NONBLOCK);
                    new_cln.fd = new_badge;
                    new_cln.events = POLLIN;
                    new_cln.revents = 0;
                    fds.push_back(new_cln);
                    std::cout << "a new client connected its id is : " << new_cln.fd << std::endl;
                }
                else
                {
                    /* if it's already an existing client */

                    while (true)
                    {
                        memset(buffer, '\0', sizeof(buffer));
                        nb = recv(fds[i].fd, buffer, sizeof(buffer), 0);
                        if (nb == 0)
                        {
                            std::cout << "the nb : "<< nb<< std::endl;
                            close (fds[i].fd);
                            fds.erase(fds.begin() + i);
                            i--;
                            std::cout << "The client : " << fds[i].fd << " disconnected"<< std::endl;
                            break;
                        }
                        else
                        {
                            std::cout << "the client : " << fds[i].fd << " sent : " << buffer << std::endl;
                            break ;
                        }
                    }
                }
            }
        }
    }

    return (0);
}