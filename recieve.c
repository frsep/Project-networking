#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_WORDSIZE                    256



struct client_server
{// SEPEHR - could you please provide comments for what this struct is/does
    int browser_port;
    char name[MAX_WORDSIZE]; //SEPEHR- I've replaced with a pre-defined CONSTANT as best practise - delete this comment once read
};


void server_listen(struct client_server *my_server){
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
        char respnce[10000] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>HELLO WRLD!</h1></body></html>";
        strcat(respnce, destination);
        send(accept_sock, respnce, sizeof(respnce), 0);
        close(accept_sock);
    }
    
}
int main(int argc, char const *argv[]){
    struct client_server my_server;
    my_server.browser_port = atoi(argv[2]);
    strcpy(my_server.name, argv[1]);
    server_listen(&my_server);
    return 0;
}
