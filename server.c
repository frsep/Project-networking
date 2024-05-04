#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>

struct client_server{
    char *browser_port;
    char name[256];
};

void server_listen(struct client_server *my_server){
    char client_req[1000];
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int bind_sock = bind(sock, my_server->browser_port, sizeof(my_server->browser_port));
    int listen_sock = listen(sock, 1);
    while(1){
        int accept_sock = accept(sock, my_server->browser_port, sizeof(my_server->browser_port));
        read(accept_sock, client_req, sizeof(client_req));
        printf("%s", client_req);
        char respnce[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>Hello, World!</h1></body></html>";
        send(accept_sock, respnce, sizeof(respnce), 0);
    }
    
}

int main(int argc, char const *argv[]){
    struct client_server my_server;

    my_server.browser_port = argv[2];
    strcpy(my_server.name, argv[1]);
    server_listen(&my_server);
    return 0;
}
