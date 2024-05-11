#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_WORDSIZE                    256
#define MAX_THREADS                     100




struct client_server{
    int browser_port;
    int query_port;
    int* neighbour_ports;
    char name[MAX_WORDSIZE];
};

void browser_recievenrespond(int sock, struct client_server *my_server){
    while(1){
        char client_req[1000];
        read(sock, client_req, sizeof(client_req));
        char* delim= "=";
        char* token;
        token = strtok(client_req, delim);
        token = strtok(NULL, delim);
        char destination[MAX_WORDSIZE];
        for (int i = 0; i < MAX_WORDSIZE; i++){
            if (token[i] == ' ') {
                destination[i] = '\0';
                break;
            }
            destination[i] = token[i];
        }
        char respnce[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>HELLO WRLD!</h1></body></html>";
        send(sock, respnce, sizeof(respnce), 0);
        close(sock);
        }
    }

void* udp_port(struct client_server *my_server){
    struct sockaddr_in udp_addr, other_serv_addr;
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(my_server->query_port);
    int bind_udpsock = bind(udp_sock,(struct sockaddr*) &udp_addr, sizeof(struct sockaddr_in));
    socklen_t addrlen = sizeof(udp_addr);
    char rec_message[1000];
    while(1){
        recvfrom(udp_sock, rec_message, sizeof(rec_message), 0, (struct sockaddr*) &other_serv_addr, &addrlen);
        //do something with recieved mesage
        char responce[] = "hellow";
        sendto(udp_sock, responce, sizeof(responce), 0, (struct sockaddr*) &other_serv_addr, sizeof(other_serv_addr));
    }
    return NULL;
}

void server_listen(struct client_server *my_server){

    pthread_t udp_listen;
    pthread_create(&udp_listen, NULL, (void*)udp_port, my_server);

    char client_req[1000];
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in _addr;
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = INADDR_ANY;
    _addr.sin_port = htons(my_server->browser_port);
    int bind_sock = bind(sock,(struct sockaddr*) &_addr, sizeof(struct sockaddr_in));
    int listen_sock = listen(sock, 1);
    socklen_t addrlen = sizeof(_addr);
    while(1){
        int accept_sock = accept(sock, (struct sockaddr*) &_addr, &addrlen);
        read(accept_sock, client_req, sizeof(client_req));
        char* delim= "=";
        char* token;
        token = strtok(client_req, delim);
        token = strtok(NULL, delim);
        char destination[MAX_WORDSIZE];
        for (int i = 0; i < MAX_WORDSIZE; i++){
            if (token[i] == ' ') {
                destination[i] = '\0';
                break;
            }
            destination[i] = token[i];
        }
        char respnce[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>HELLO WRLD!</h1></body></html>";
        send(accept_sock, respnce, sizeof(respnce), 0);
        close(accept_sock);
        }


}
int main(int argc, char const *argv[]){
    struct client_server my_server;
    my_server.neighbour_ports = malloc(sizeof(int)*(argc-4));
    for(int i = 0; i < (argc-4); i++){
        my_server.neighbour_ports[i] = atoi(argv[4+i]);
    }
    my_server.query_port = atoi(argv[3]);
    my_server.browser_port = atoi(argv[2]);
    strcpy(my_server.name, argv[1]);
    server_listen(&my_server);
    return 0;
}
