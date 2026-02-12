#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


int main(void)
{
    int srv_socket;
    int cli_socket;
    char buffer[1024] = {0};
    sockaddr_in srv_addr;



    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(8080);
    srv_addr.sin_addr.s_addr = INADDR_ANY ;

    bind(srv_socket, (struct sockaddr *)& srv_addr, sizeof(srv_addr));
    listen(srv_socket, 5);
    cli_socket = accept(srv_socket, nullptr, nullptr);
    recv(cli_socket, buffer, sizeof(buffer), 0);
    std::cout  << "the msg received by the client is : " << buffer << std::endl;
    close(srv_socket);

    return (0);
}