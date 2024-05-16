#include <sys/socket.h>

#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>


#define MAX_WORDSIZE                    256
#define MAX_STATIONS                     100

struct neighbours{
    int port;
    char name[MAX_WORDSIZE];
    bool added;
};

struct client_server{
    int browser_port;
    int query_port;
    int neighbour_count;
    char name[MAX_WORDSIZE];
    char messages[MAX_WORDSIZE][MAX_STATIONS];
    int messages_count;
    char message_out[MAX_WORDSIZE];
    bool message_out_flag;
    struct neighbours *neighbour_list;
    int neighbours_added;
};

void process_message(char* message, struct client_server *my_server){
    if(message[5] == "N"){
        char* delim= ";";
        char* delim2= "\n";
        char* token;
        char* token2;
        char* name;
        token = strtok(message, delim);
        token2 = strtok(NULL, delim);
        strcpy(my_server->neighbour_list[my_server->neighbours_added].port, aoti(token2));
        strtok(token, delim2);
        name = strtok(NULL, delim2);
        strcpy(my_server->neighbour_list[my_server->neighbours_added].name, name);
        my_server->neighbour_list[my_server->neighbours_added].added = true;
        my_server->neighbours_added++;
        return;
    }
    if(!isdigit(message[0])){
        strcpy(my_server->messages[my_server->messages_count], message);
        my_server->messages_count++;
    }
    else{
        char temp[MAX_WORDSIZE];
        sprintf(temp, "%d", my_server->query_port);
        strcat(temp, ",");
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
    int len = strlen(message);
    int result = sendto(udp_sock, message, len, 0, (struct sockaddr*) &udp_addr, sizeof(udp_addr));
    close(udp_sock);
}

int find_destination(char* message){
    char* delim= ",";
    char* token;
    token = strtok(message, delim);
    return atoi(token);
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
        int result = recvfrom(udp_sock, (char*)rec_message, sizeof(rec_message), 0, (struct sockaddr*) &other_serv_addr, &addrlen);
        //do something with recieved mesage
        process_message(rec_message, my_server);
        if(my_server->message_out_flag == true){
            my_server->message_out_flag = false;
            int udp_send = atoi(rec_message);
            char temp[MAX_WORDSIZE];
            strcpy(temp,"x");
            strcat(temp, my_server->message_out);
            send_udp(udp_send, temp);
            memset((*my_server).message_out, '\0', sizeof((*my_server).message_out));
            memset(rec_message, '\0', sizeof((*my_server).message_out));
        }
    }
    return NULL;
}

void send_name_out(struct client_server *my_server){
    char temp[MAX_WORDSIZE];
    char temp2[MAX_WORDSIZE];
    strcpy(temp, "Type_Name/n");
    strcat(temp, my_server->name);
    strcat(temp, ";");
    sprintf(temp2, "%d", my_server->query_port);
    strcat(temp, temp2);
    while(1){
        for(int i = 0; i < my_server->neighbour_count; i++){
            if(my_server->neighbour_list[i].added == false){
                send_udp(my_server->neighbour_list[i].port, temp);
            }
        }
        if(my_server->neighbours_added == my_server->neighbour_count){
            break;
        }
    }
    return;
}
     


void server_listen(struct client_server *my_server){
    /*
    struct sockaddr_in udp_addr, other_serv_addr;
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(my_server->query_port);
    int bind_udpsock = bind(udp_sock,(struct sockaddr*) &udp_addr, sizeof(struct sockaddr_in));
    char rec_message[1000];

    char client_req[1000];
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in _addr;
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = INADDR_ANY;
    _addr.sin_port = htons(my_server->browser_port);
    int bind_sock = bind(sock,(struct sockaddr*) &_addr, sizeof(struct sockaddr_in));
    int listen_sock = listen(sock, 1);
    socklen_t addrlen = sizeof(_addr);

    fd_set readfds;

    

    while(1){
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(udp_sock, &readfds);
        int max_fd = (sock > udp_sock) ? sock : udp_sock;
        int activity = select(max_fd+1, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(sock, &readfds)){
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
            
            char temp[MAX_WORDSIZE];
            strcpy(temp,"hello");
            for(int i = 0; i < my_server->neighbour_count; i++){
                send_udp(my_server->neighbour_ports[i], temp);
            }
            
            while(1){
                if(my_server->messages_count == my_server->neighbour_count){
                    break;
                }
            }

            char respnce[10000] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>HELLO WRLD!";
            
            for(int i = 0; i < my_server->messages_count; i++){
                strcat(respnce, my_server->messages[i]);
            }
            strcat(respnce, "</h1></body></html>");
            
            for(int i = 0; i < MAX_WORDSIZE; i++){
                strcpy(my_server->messages[i], "\0");
            }
            
            my_server->messages_count = 0;
            int client_result = send(accept_sock, respnce, sizeof(respnce), 0);
            close(accept_sock);


        if(FD_ISSET(udp_sock, &readfds)){
            int result = recvfrom(udp_sock, (char*)rec_message, sizeof(rec_message), 0, (struct sockaddr*) &other_serv_addr, &addrlen);
            //do something with recieved mesage
            process_message(rec_message, my_server);
            if(my_server->message_out_flag == true){
                my_server->message_out_flag = false;
                int udp_send = find_destination(my_server->message_out);
                send_udp(udp_send, my_server->message_out);
                memset((*my_server).message_out, '\0', sizeof((*my_server).message_out));
            }
        }



    }
    */
    
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
        char temp[MAX_WORDSIZE];
        sprintf(temp, "%d", my_server->query_port);
        for(int i = 0; i < my_server->neighbour_count; i++){
            send_udp(my_server->neighbour_list[i].port, temp);
        }
        while(1){
            if(my_server->messages_count == my_server->neighbour_count){
                break;
            }
        }

        char respnce[10000] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>HELLO WRLD!";
        
        for(int i = 0; i < my_server->messages_count; i++){
            strcat(respnce, my_server->messages[i]);
        }
        strcat(respnce, "</h1></body></html>");
        send(accept_sock, respnce, sizeof(respnce), 0);


        for(int i = 0; i < my_server->messages_count; i++){
            strcpy(my_server->messages[i], "\0");
        }
        my_server->messages_count = 0;



        
        close(accept_sock);
    }

    
}

int main(int argc, char const *argv[]){
    struct client_server my_server;
    my_server.neighbours_added = 0;
    my_server.messages_count = 0;
    memset(my_server.message_out, '\0', sizeof(my_server.message_out));
    bool message_out_flag = false;
    for (int i = 0; i < MAX_STATIONS; i++){
        strcpy(my_server.messages[i], "\0");
    }
    int num_neighbours = argc-4;
    my_server.neighbour_count = num_neighbours;

    /**/
    my_server.neighbour_list = (struct neighbours*)malloc(num_neighbours*sizeof(struct neighbours));

    char temp[MAX_WORDSIZE];
    for(int i = 0; i < num_neighbours; i++){
        strcpy((char*)temp, (char*)argv[4+i]);
        // split temp with ":" and store in a new array
        char* port_str = strtok(temp, ":");
        char* ip_str = strtok(NULL, ":");
        // Store port_str and ip_str in separate arrays if needed
        my_server.neighbour_list[i].port = atoi(ip_str);
        my_server.neighbour_list[i].added = false;
    }



    my_server.query_port = atoi(argv[3]);
    my_server.browser_port = atoi(argv[2]);
    strcpy(my_server.name, argv[1]);
    server_listen(&my_server);
    return 0;
}