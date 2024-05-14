#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_WORDSIZE                    256
#define MAX_STATIONS                     100




struct client_server{
    int browser_port;
    int query_port;
    int* neighbour_ports;
    char name[MAX_WORDSIZE];
    char messages[MAX_WORDSIZE][MAX_STATIONS];
    int messages_count;
    char message_out[MAX_WORDSIZE];
    bool message_out_flag;
};

void process_message(char* message, struct client_server *my_server){
    if(message[0] == 'I' && strcmp(&message[1], " ") == 0){
        strcpy(my_server->messages[my_server->messages_count], message);
        my_server->messages_count++;
    }
    else{
        char temp[MAX_WORDSIZE] = "I ";
        strcat(temp, my_server->name);
        strcpy(my_server->message_out, temp);
        my_server->message_out_flag = true;
    }

}

void send_udp(int port_number, char* message){
    struct sockaddr_in udp_addr;
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(port_number);
    sendto(udp_sock, message, sizeof(message), 0, (struct sockaddr*) &udp_addr, sizeof(udp_addr));
    close(udp_sock);
}

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

        for(int i = 0; i < sizeof(my_server->neighbour_ports); i++){
            send_udp(my_server->neighbour_ports[i], destination);
        }
        while(1){
            if(my_server->messages_count == sizeof(my_server->neighbour_ports)){
                break;
            }
        

        char respnce[10000] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>HELLO WRLD!\n";
        
        for(int i = 0; i < my_server->messages_count; i++){
            strcat(respnce, my_server->messages[i]);
            strcat(respnce, "\n");

        }
        strcat(respnce, "</h1></body></html>");
        for(int i = 0; i < sizeof(my_server->messages); i++){
            strcpy(my_server->messages[i], "\0");
        }
        my_server->messages_count = 0;


        send(sock, respnce, sizeof(respnce), 0);
        close(sock);
        }
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
        process_message(rec_message, my_server);
        if(my_server->message_out_flag == true){
            my_server->message_out_flag = false;
            sendto(udp_sock, my_server->message_out, sizeof(my_server->message_out), 0, (struct sockaddr*) &other_serv_addr, sizeof(other_serv_addr));
            my_server->message_out[0] = '\0';
        }
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

        for(int i = 0; i < sizeof(my_server->neighbour_ports); i++){
            send_udp(my_server->neighbour_ports[i], destination);
        }
        while(1){
            if(my_server->messages_count == sizeof(my_server->neighbour_ports)){
                break;
            }
        }

        char respnce[10000] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>HELLO WRLD!";
        
        for(int i = 0; i < my_server->messages_count; i++){
            strcat(respnce, my_server->messages[i]);
        }
        strcat(respnce, "</h1></body></html>");
        for(int i = 0; i < sizeof(my_server->messages); i++){
            strcpy(my_server->messages[i], "\0");
        }
        my_server->messages_count = 0;


        send(sock, respnce, sizeof(respnce), 0);

        
        close(accept_sock);
    }


}
int main(int argc, char const *argv[]){
    struct client_server my_server;
    my_server.messages_count = 0;
    my_server.message_out[0] = '\0';
    bool message_out_flag = false;
    for (int i = 0; i < MAX_STATIONS; i++){
        strcpy(my_server.messages[i], "\0");
    }
    my_server.neighbour_ports = malloc(sizeof(int)*(argc-4));
    for(int i = 0; i < (argc-4); i++){
        char temp[MAX_WORDSIZE];

        for(int j = 0; j < MAX_WORDSIZE; j++){
            if(argv[4+i][j] == ':'){
                temp[j] = '\0';
                break;
            }
            temp[j] = argv[4+i][j];
        }
        
        my_server.neighbour_ports[i] = atoi(argv[4+i]);
    }
    my_server.query_port = atoi(argv[3]);
    my_server.browser_port = atoi(argv[2]);
    strcpy(my_server.name, argv[1]);
    server_listen(&my_server);
    return 0;
}
