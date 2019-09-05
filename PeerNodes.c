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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <time.h>

void deprocessing(int peer_socket, int k);
int create_socket(struct sockaddr_in serv_addr);
void construct_serv_addr(struct sockaddr_in serv_addr[], char peer_server_address[3][BUFSIZ], char peer_port_number[3][BUFSIZ]);

int main(int argc, char **argv)
{
    char buffer[BUFSIZ], peer_port_number[3][BUFSIZ], relay_port_number[BUFSIZ], peer_server_address[3][BUFSIZ], relay_server_address[BUFSIZ];
    struct sockaddr_in relay_serv_addr, serv_addr[3], peer_addr;
    int client_socket, peer_socket, rand_port;
    socklen_t sock_len;  

    if (argc < 3)
    {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }     
    
    /* Zeroing serv_addr struct */
    memset(&relay_serv_addr, 0, sizeof(relay_serv_addr));
    memset(serv_addr, 0, sizeof(serv_addr));
 
    /* Setting server addresses and port numbers */
    srand(time(0));
    for(int i=0;i<3;i++){
        sprintf(peer_port_number[i], "%d", ((rand_port=rand()%65535)>1023?rand_port:rand_port+1024));
        sprintf(peer_server_address[i], "%s", "127.0.0.1");
    }
    
    sprintf(relay_port_number, "%s", argv[2]);
    sprintf(relay_server_address, "%s", argv[1]);
    
    /* Construct serv_addr struct */
    relay_serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, relay_server_address, &(relay_serv_addr.sin_addr));
    relay_serv_addr.sin_port = htons(atoi(relay_port_number));
    construct_serv_addr(serv_addr, peer_server_address, peer_port_number);

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

    /* Sending peer addresses and port numbers */
    sprintf(buffer, "request:peer");
    send(client_socket, buffer, sizeof(buffer), 0);
    for(int i=0;i<3;i++){
        send(client_socket, peer_port_number[i], sizeof(peer_port_number[i]), 0);
        send(client_socket, peer_server_address[i], sizeof(peer_server_address[i]), 0);
    }
    
    close(client_socket);

    if(fork()==0)
    {
        int socket = create_socket(serv_addr[0]);
        fprintf(stdout, "Peer 1 is Listening!!!!\n");
        sock_len = sizeof(struct sockaddr_in);
        while(1)
        {
            peer_socket = accept(socket, (struct sockaddr *)&peer_addr, &sock_len);
            if (peer_socket == -1)
            {
                fprintf(stderr, "Error on accept --> %s", strerror(errno));

                exit(EXIT_FAILURE);
            }
            // deprocessing(peer_socket);
            deprocessing(peer_socket, 1);
            close(peer_socket);
            // close(socket);
        }
    }else
    {
        if(fork()==0){
            int socket = create_socket(serv_addr[1]);
            fprintf(stdout, "Peer 2 is Listening!!!!\n");
            sock_len = sizeof(struct sockaddr_in);
            while(1)
            {
                peer_socket = accept(socket, (struct sockaddr *)&peer_addr, &sock_len);
                if (peer_socket == -1)
                {
                    fprintf(stderr, "Error on accept --> %s", strerror(errno));

                    exit(EXIT_FAILURE);
                }
                // deprocessing(peer_socket);
                deprocessing(peer_socket, 2);
                close(peer_socket);
                // close(socket);
            }
        }else
        {
            int socket = create_socket(serv_addr[2]);
            fprintf(stdout, "Peer 3 is Listening!!!!\n");
            sock_len = sizeof(struct sockaddr_in);
            while(1)
            {
                peer_socket = accept(socket, (struct sockaddr *)&peer_addr, &sock_len);
                if (peer_socket == -1)
                {
                    fprintf(stderr, "Error on accept --> %s", strerror(errno));

                    exit(EXIT_FAILURE);
                }
                // deprocessing(peer_socket);
                deprocessing(peer_socket, 3);
                close(peer_socket);
                // close(socket);
            }
        }
    }

    return 0;
}

void construct_serv_addr(struct sockaddr_in serv_addr[], char peer_server_address[3][BUFSIZ], char peer_port_number[3][BUFSIZ]){
    for(int i=0;i<3;i++){
        serv_addr[i].sin_family = AF_INET;
        inet_pton(AF_INET, peer_server_address[i], &(serv_addr[i].sin_addr));
        serv_addr[i].sin_port = htons(atoi(peer_port_number[i]));
    }
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

void deprocessing(int peer_socket, int k){
    char buffer[BUFSIZ], file_name[BUFSIZ], file_name1[BUFSIZ], file_size[BUFSIZ];
    int offset, fd, remain_data = 0, sent_bytes = 0;
    struct stat file_stat;
    ssize_t len;    

    /* Receiving file name */
    recv(peer_socket, file_name, BUFSIZ, 0);
    if(k==1){
        sprintf(file_name1, "/home/saurabh/%s", file_name);
    }else if(k==2){
        sprintf(file_name1, "/home/saurabh/Desktop/%s", file_name);
    }else{
        sprintf(file_name1, "/home/saurabh/Desktop/Assignment03/%s", file_name);
    }
    
    fd = open(file_name1, O_RDONLY);
    if (fd == -1)
    {
        // fprintf(stderr, "Error opening file --> %s", strerror(errno));
        sprintf(buffer, "NOT Found");
        send(peer_socket, buffer, sizeof(buffer), 0);
        // close(peer_socket);
        // exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(stdout, "\nPeer %d is sending %s file.\n", k, file_name);
        sprintf(buffer, "Found");
        send(peer_socket, buffer, sizeof(buffer), 0);

        /* Get file stats */
        if (fstat(fd, &file_stat) < 0)
        {
            fprintf(stderr, "Error fstat --> %s", strerror(errno));

            exit(EXIT_FAILURE);
        }

        // fprintf(stdout, "File Size: \n%ld bytes\n", file_stat.st_size);

        sprintf(file_size, "%ld", file_stat.st_size);

        /* Sending file size */
        len = send(peer_socket, file_size, sizeof(file_size), 0);
        if (len < 0)
        {
            fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

            exit(EXIT_FAILURE);
        }

        // fprintf(stdout, "Server sent %ld bytes for the size\n", len);

        offset = 0;
        remain_data = file_stat.st_size;

        /* Sending file data */
        while(((sent_bytes = sendfile(peer_socket, fd, NULL, BUFSIZ)) > 0) && (remain_data > 0)){
            remain_data -= sent_bytes;
        }
    }
}