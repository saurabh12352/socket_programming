// BUFSIZ : A common size is 512. It gives us the maximum number of characters that can be transferred through our standard input stream, stdin.
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h> //contains inet_pton - convert IPv4 and IPv6 addresses from text to binary form
#include <unistd.h>
#include <netinet/in.h>

void deprocessing(char peer_server_address[3][BUFSIZ], char peer_port_number[3][BUFSIZ]);

int main(int argc, char **argv)
{
    char buffer[BUFSIZ], peer_server_address[3][BUFSIZ], peer_port_number[3][BUFSIZ], relay_server_address[BUFSIZ];
    struct sockaddr_in relay_serv_addr, serv_addr;
    int client_socket, relay_port_number;
    int file_size, remain_data = 0;
    ssize_t len;

    if (argc < 3)
    {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }

    /* Zeroing relay_serv_addr struct */
    memset(&relay_serv_addr, 0, sizeof(relay_serv_addr));
   
    /* Setting relay server addresses and port numbers */
    relay_port_number = atoi(argv[2]);
    sprintf(relay_server_address, "%s", argv[1]);
    
    /* Construct relay_serv_addr struct */
    relay_serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, relay_server_address, &(relay_serv_addr.sin_addr));
    relay_serv_addr.sin_port = htons(relay_port_number);

    /* Create client socket */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        fprintf(stderr, "Error creating socket --> %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Connect to the server */
    if (connect(client_socket, (struct sockaddr *)&relay_serv_addr, sizeof(relay_serv_addr)) == -1)
    {
        fprintf(stderr, "Error on connect --> %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Receiving peer addresses and port numbers */
    sprintf(buffer, "request:client");
    send(client_socket, buffer, sizeof(buffer), 0);
    for(int i=0;i<3;i++){
        recv(client_socket, peer_port_number[i], BUFSIZ, 0);
        recv(client_socket, peer_server_address[i], BUFSIZ, 0);
    }
    
    for(int i=0;i<3;i++){
        printf("%s\t%s\n", peer_server_address[i], peer_port_number[i]);
    }

    close(client_socket);

    while(1){
        deprocessing(peer_server_address, peer_port_number);
    }

    return 0;
}

void deprocessing(char peer_server_address[3][BUFSIZ], char peer_port_number[3][BUFSIZ]){
    char buffer[BUFSIZ], file_name[BUFSIZ], received_file_name[BUFSIZ];
    struct sockaddr_in serv_addr;
    int client_socket;
    int file_size, remain_data = 0;
    ssize_t len;

    printf("\nEnter the file name:\n");
    scanf(" %[^\n]s", file_name);

    for(int i=0;i<3;i++){
        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        inet_pton(AF_INET, peer_server_address[i], &(serv_addr.sin_addr));
        serv_addr.sin_port = htons(atoi(peer_port_number[i]));

        /* Create client socket */
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1)
        {
            fprintf(stderr, "Error creating socket --> %s\n", strerror(errno));

            exit(EXIT_FAILURE);
        }

        /* Connect to the server */
        if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        {
            fprintf(stderr, "Error on connect --> %s\n", strerror(errno));

            exit(EXIT_FAILURE);
        }

        send(client_socket, file_name, sizeof(file_name), 0);
        if (len < 0)
        {
            fprintf(stderr, "Error on sending file name --> %s", strerror(errno));

            exit(EXIT_FAILURE);
        }

        recv(client_socket, buffer, BUFSIZ, 0);
        if(strcmp(buffer, "Found") == 0)
        {
            fprintf(stdout, "\n===============================================================\n");
            fprintf(stdout, "%s file is received from Peer %d.\n", file_name, i+1);
            fprintf(stdout, "%s file has the following content:\n", file_name);
            recv(client_socket, buffer, BUFSIZ, 0);
            file_size = atoi(buffer);
            remain_data = file_size;

            /* writing received data into new file */
            sprintf(received_file_name, "Received_file_%s", file_name);
            FILE * received_file = fopen(received_file_name, "w");
            
            while(((len = recv(client_socket, buffer, BUFSIZ, 0)) > 0 && remain_data > 0))
            {
                fwrite(buffer, sizeof(char), len, received_file);
                fprintf(stdout, "%s\n", buffer);
                remain_data -= len;
            }
            fclose(received_file);
            
            fprintf(stdout, "===============================================================\n");
            close(client_socket);
            break;
        }
        if(i==2)
        {
            fprintf(stdout, "\n=================== %s file Not Found!!!!===================\n\n", file_name);
        }
        close(client_socket);
    }
}