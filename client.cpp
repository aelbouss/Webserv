#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


int main(void)
{
    int cli_socket;
    sockaddr_in    srv_addr;
    
    const char    *msg = "Hello from the client anass";

   cli_socket = socket(AF_INET, SOCK_STREAM, 0);
   srv_addr.sin_family = AF_INET;
   srv_addr.sin_port = htons(8080);
   srv_addr.sin_addr.s_addr = INADDR_ANY ;

   connect(cli_socket, (struct sockaddr *)&srv_addr , sizeof(srv_addr));
   send(cli_socket, msg, strlen(msg), 0);
   close(cli_socket);
    return (0);
}