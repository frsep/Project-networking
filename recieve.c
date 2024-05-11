#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_WORDSIZE                    256



struct client_server{
    int browser_port;
    int query_port;
    char name[MAX_WORDSIZE];
};

void browser_recievenrespond(int sock, struct client_server *my_server){
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

void server_listen(struct client_server *my_server){

    //  Create a socket and bind it to the browser port for TCP connection
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in _addr;
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = INADDR_ANY;
    _addr.sin_port = htons(my_server->browser_port);
    int bind_sock = bind(sock,(struct sockaddr*) &_addr, sizeof(struct sockaddr_in));
    int listen_sock = listen(sock, 1);
    socklen_t addrlen = sizeof(_addr);

    // creates socket and binds it with the query port for UDP connection
    int sock2 = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in udp_addr;
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(my_server->query_port);
    int bind_udpsock = bind(sock2,(struct sockaddr*) &udp_addr, sizeof(struct sockaddr_in));
    int listen_udpsock = listen(sock2, 1);
    socklen_t addrlen2 = sizeof(udp_addr);





    while(1){
        browser_recievenrespond(accept(sock, (struct sockaddr*) &_addr, &addrlen), my_server);

    }
    
}
int main(int argc, char const *argv[]){
    struct client_server my_server;
    my_server.query_port = atoi(argv[3]);
    my_server.browser_port = atoi(argv[2]);
    strcpy(my_server.name, argv[1]);
    server_listen(&my_server);
    return 0;
}
