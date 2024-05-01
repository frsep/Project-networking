#include <sys/socket.h>

struct server{
    char *browser_port;
    char *name;
};
void server_listen(struct server *server){
    char client_req[1000];
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int bind_sock = bind(sock, server->browser_port, sizeof(server->browser_port));
    int listen_sock = listen(sock, 1);
    while(1){
        int accept_sock = accept(sock, server->browser_port, sizeof(server->browser_port));
        read(accept_sock, client_req, sizeof(client_req));
        printf("%s", client_req);
    }
    
}

int main(int argc, char const *argv[]){
    struct server server;
    server.browser_port = argv[2];
    server.name = argv[1];
    server_listen(&server);
    return 0;
}
