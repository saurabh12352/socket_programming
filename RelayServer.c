#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define SERVER_ADDRESS  "127.0.0.1"

int create_socket(struct sockaddr_in serv_addr);

int main(int argc, char **argv)
{
    char peer_server_address[3][BUFSIZ], peer_port_number[3][BUFSIZ], buffer[BUFSIZ];
    int server_socket, port_number, peer_socket, no_of_peer;
    struct sockaddr_in serv_addr, peer_addr;
    socklen_t sock_len;
    ssize_t len;
    
    if (argc < 2)
    {
        fprintf(stderr,"usage %s port\n", argv[0]);
        exit(0);
    }

    /* Zeroing serv_addr struct */
    memset(&serv_addr, 0, sizeof(serv_addr));

    /* Resolving Port number from terminal */
    port_number = atoi(argv[1]);

    /* Construct serv_addr struct */
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_ADDRESS, &(serv_addr.sin_addr));
    serv_addr.sin_port = htons(port_number);

    server_socket = create_socket(serv_addr);
    fprintf(stdout, "Relay Server is Listening!!!!!\n");
    sock_len = sizeof(struct sockaddr_in);

    while(1)
    {
        peer_socket = accept(server_socket, (struct sockaddr *)&peer_addr, &sock_len);
        if (peer_socket == -1)
        {
            fprintf(stderr, "Error on accept --> %s", strerror(errno));

            exit(EXIT_FAILURE);
        }

        recv(peer_socket, buffer, BUFSIZ, 0);   
        if(strcmp("request:peer", buffer)==0)
        {
            /* Receiving peer addresses and port numbers */
            for(int i=0;i<3;i++)
            {
                recv(peer_socket, peer_port_number[i], BUFSIZ, 0);
                recv(peer_socket, peer_server_address[i], BUFSIZ, 0);    
                printf("%s\t%s\n", peer_server_address[i], peer_port_number[i]);
            }
            
            close(peer_socket);    
        }else if(strcmp("request:client", buffer)==0)
        {
            /* Sending peer addresses and port numbers */
            for(int i=0;i<3;i++)
            {
                send(peer_socket, peer_port_number[i], sizeof(peer_port_number[i]), 0);
                send(peer_socket, peer_server_address[i], sizeof(peer_server_address[i]), 0);
            }
            
            close(peer_socket);
        }else
        {
            fprintf(stdout, "Unknown Request.\n");
        }
    }

    return 0;
}

int create_socket(struct sockaddr_in serv_addr){
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        fprintf(stderr, "Error creating socket --> %s", strerror(errno));
        
        exit(EXIT_FAILURE);
    }

    /* Bind */
    if ((bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr))) == -1)
    {
        fprintf(stderr, "Error on bind --> %s", strerror(errno));
        
        exit(EXIT_FAILURE);
    }

    /* Listening to incoming connections */
    if ((listen(server_socket, 5)) == -1)
    {
        fprintf(stderr, "Error on listen --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    return server_socket;
}