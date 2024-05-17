#include <sys/socket.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

// Constants
#define MAX_WORDSIZE                    256
#define MAX_STATIONS                    100
#define MAX_DEPARTURES                  100
#define MAX_HOPS                        100
#define MAX_LINESIZE                    256 
#define DEBUG                           1
#define CHAR_COMMENT                    '#'
// structs

typedef struct{
    int port;
    char name[MAX_WORDSIZE];
    bool added;
} neighbours;

typedef struct
{   // Route struct to hold data of a single departure route
    int departureTime;
    char routeName[MAX_WORDSIZE];
    char departingFrom[MAX_WORDSIZE];
    int arrivalTime;
    char arrivalStation[MAX_WORDSIZE];
} route;

typedef struct
{// Message struct to hold contents of a message
    char dataType[MAX_WORDSIZE];
    char destination[MAX_WORDSIZE];
    char result[MAX_WORDSIZE];
    route data[MAX_HOPS];
    int currentHop;
    int current_response_count;
    int responses_needed;
} response;

typedef struct
{// Message struct to hold contents of a message
    char dataType[MAX_WORDSIZE];
    char destination[MAX_WORDSIZE];
    char result[MAX_WORDSIZE];
    route data[MAX_HOPS];
    int currentHop;
    int current_response_count;
    int responses_needed;
    response responses[MAX_HOPS];
} messages;

struct timetable
{// Timetable struct to hold station name, lat/lon, and array of all departure routes
    char stationName[MAX_WORDSIZE];
    float longitude;
    float latitude;
    route departures[MAX_DEPARTURES];
    int nroutes;
};

struct client_server
{
    int browser_port;
    int query_port;
    int neighbour_count;
    char name[MAX_WORDSIZE];
    int messages_count;
    neighbours *neighbour_list;
    messages queries[MAX_STATIONS];
    response* responses[MAX_STATIONS];
    int responses_count;
    int neighbours_added;
};

// functions
bool find_route(route *found, int time, char *final_destination, struct timetable *station)
{// Check for possible route to desired destination within current station
    for (int i = 0; i<station->nroutes; i++){
        if (time < station->departures[i].departureTime){
            if (strcmp(station->departures[i].arrivalStation, final_destination)){
                *found = station->departures[i]; 
                return true;
            }
        }
    }
    return false;
}

bool is_comment_line(char line[])
{// checks if a line is a comment line
    int i = 0;
    while(isspace(line[i] != 0)){  //checks if character is a white space character
        ++i;
    }
    return (line[i] == CHAR_COMMENT); //if comment is found, return true
}

void trim_line(char line[])
{// removes trailing 'end-of-line' characters from the line
    int i = 0;
    while(line[i] != '\0') {// iterates through each character of the line
        if( line[i] == '\r' || line[i] == '\n'){ //checks for unwanted end of line characters and replaces with '\0'
            line[i] = '\0';
            break;
        }
        ++i;
    }
}

void read_timetable(char filename[], struct timetable *station)
{// Function to read csv file and load timetable data into structures.
    FILE *tt = fopen(filename, "r");                   // attempt to open file
    if(tt == NULL){                                    // checks for errors in opening file
        printf("could not open timetable file '%s'\n", filename);
        exit(EXIT_FAILURE);                             //terminates if file can't be opened
    }
    //reading file
    char line[BUFSIZ];// stores contents of one line at a time as a character array
    bool stationUnread = true;
    while (fgets(line,sizeof line, tt) != NULL){//until a line is empty (end of file reached)
        trim_line(line); // removes the \n or \r at end of line
        if (is_comment_line(line)){ //skips to next line if its a comment line
            continue;
        }
        if (stationUnread){ // handle station name and location (although lat and longitude not required)
            // station-name,longitude,latitude
            char stationName[MAX_WORDSIZE];
            char longitude[MAX_WORDSIZE];
            char latitude[MAX_WORDSIZE];
            sscanf(line, "%s, %s, %s", stationName, longitude, latitude);
            strcpy( station->stationName, stationName);
            station->longitude = atof(longitude);
            station->latitude = atof(latitude);
            stationUnread = false;
        }
        else{
            // departure-time,route-name,departing-from,arrival-time,arrival-station
            char departTime[MAX_WORDSIZE];
            char routeName[MAX_WORDSIZE];
            char departingFrom[MAX_WORDSIZE];
            char arrivalTime[MAX_WORDSIZE];
            char arrivalStation[MAX_WORDSIZE];
            sscanf( line, "%s, %s, %s, %s, %s", departTime, routeName, departingFrom, arrivalTime, arrivalStation);
                    station->departures[station->nroutes].departureTime = atoi(departTime);
            strcpy(station->departures[station->nroutes].routeName, routeName);
            strcpy( station->departures[station->nroutes].departingFrom, departingFrom);
                    station->departures[station->nroutes].arrivalTime = atoi(arrivalTime);
            strcpy(station->departures[station->nroutes].arrivalStation, arrivalStation);
            ++station->nroutes;
        }
    }
    fclose(tt); //closes timetable file when end of file reached
}

void create_name_message(struct client_server *my_server, char *name_message){
    char temp[MAX_WORDSIZE];
    char temp2[MAX_WORDSIZE];
    strcpy(temp, "Type_Name/n");
    strcat(temp, my_server->name);
    strcat(temp, ";");
    sprintf(temp2, "%d", my_server->query_port);
    strcat(temp, temp2);
    strcpy(name_message, temp);
}

void parse_msg_data(messages* message, char* msg_data)
{// Parse data into its component hops
    int hop = 0;
    char msg_hop[MAX_LINESIZE];
    char departTime[MAX_WORDSIZE];
    char routeName[MAX_WORDSIZE];
    char departingFrom[MAX_WORDSIZE];
    char arrivalTime[MAX_WORDSIZE];
    char arrivalStation[MAX_WORDSIZE];

    while (sscanf(msg_data, "%s;%s", msg_hop, msg_data) > 0){
        sscanf( msg_hop, "%s, %s, %s, %s, %s", departTime, routeName, departingFrom, arrivalTime, arrivalStation);
                message->data[hop].departureTime = atoi(departTime);
        strcpy( message->data[hop].routeName, routeName);
        strcpy( message->data[hop].departingFrom, departingFrom);
                message->data[hop].arrivalTime = atoi(arrivalTime);
        strcpy( message->data[hop].arrivalStation, arrivalStation);      
        ++hop; 
    }
    message->currentHop = hop;
}

void parse_response_data(response* message, char* msg_data)
{// Parse data into its component hops
    int hop = 0;        
    char msg_hop[MAX_LINESIZE];
    char departTime[MAX_WORDSIZE];
    char routeName[MAX_WORDSIZE];
    char departingFrom[MAX_WORDSIZE];
    char arrivalTime[MAX_WORDSIZE];
    char arrivalStation[MAX_WORDSIZE];

    while (sscanf(msg_data, "%s;%s", msg_hop, msg_data) > 0){
        sscanf( msg_hop, "%s, %s, %s, %s, %s", departTime, routeName, departingFrom, arrivalTime, arrivalStation);
                message->data[hop].departureTime = atoi(departTime);
        strcpy( message->data[hop].routeName, routeName);
        strcpy( message->data[hop].departingFrom, departingFrom);
                message->data[hop].arrivalTime = atoi(arrivalTime);
        strcpy( message->data[hop].arrivalStation, arrivalStation);      
        ++hop; 
    }
    message->currentHop = hop;
}

void parse_message(messages* message, char* msg)
{// Parse message into its component parts and store in message struct
    char *line;
    char key[MAX_WORDSIZE];
    char msg_data[MAX_LINESIZE];
    line = strtok(msg, "/n");
    sscanf(line, "%s_%s", key, message->dataType);
    line = strtok(NULL, "/n");
    sscanf(line, "%s_%s", key, message->result);
    line = strtok(NULL, "/n");
    sscanf(line, "%s_%s", key, message->destination);
    line = strtok(NULL, "/n");
    sscanf(line, "%s_%s", key, msg_data);
    parse_msg_data(message, msg_data);
}

void parse_response(response* message, char* msg){
    // Parse message into its component parts and store in message struct
    char *line;
    char key[MAX_WORDSIZE];
    char msg_data[MAX_LINESIZE];
    line = strtok(msg, "/n");
    sscanf(line, "%s_%s", key, message->dataType);
    line = strtok(NULL, "/n");
    sscanf(line, "%s_%s", key, message->result);
    line = strtok(NULL, "/n");
    sscanf(line, "%s_%s", key, message->destination);
    line = strtok(NULL, "/n");
    sscanf(line, "%s_%s", key, msg_data);
    parse_response_data(message, msg_data);
}

void parse_query(messages* message, char* msg){
    // Parse message into its component parts and store in message struct
    char *line;
    char key[MAX_WORDSIZE];
    char msg_data[MAX_LINESIZE];
    line = strtok(msg, "/n");
    sscanf(line, "%s_%s", key, message->dataType);
    line = strtok(NULL, "/n");
    sscanf(line, "%s_%s", key, message->destination);
    line = strtok(NULL, "/n");
    sscanf(line, "%s_%s", key, msg_data);
    parse_msg_data(message, msg_data);
    message->responses_needed = 0;
    message->current_response_count = 0;
}

void create_query(char* query, route* route)
{// Create a query message to send to other servers
    char temp[MAX_LINESIZE];
    sprintf(temp, "%s;%d,%s,%s,%d,%s", query, route->departureTime, route->routeName, route->departingFrom, route->arrivalTime, route->arrivalStation);
    strcpy (query, temp);
}

void create_response(char* reply, char* neg_or_pos, response* message){
    char response[MAX_WORDSIZE];
    char temp[MAX_WORDSIZE];
    strcpy(response, "Type_Response\n");
    strcat(response, "\n");
    strcat(response, neg_or_pos);
    strcat(response, "\n");
    for(int i = 0; i < message->currentHop - 1; i++){
        sprintf(temp, "%d:%d", message->data[i].departureTime/100, message->data[i].departureTime%100);
        strcat(response, temp);
        strcat(response, ",");
        strcat(response, message->data[i].routeName);
        strcat(response, ",");
        strcat(response, message->data[i].departingFrom);
        strcat(response, ",");
        strcat(response, message->data[i].arrivalStation);
        strcat(response, ",");
        sprintf(temp, "%d:%d", message->data[i].arrivalTime/100, message->data[i].arrivalTime%100);
        strcat(response, temp);
        strcat(response, ";");
    }
    sprintf(temp, "%d:%d", message->data[message->currentHop - 1].departureTime/100, message->data[message->currentHop - 1].departureTime%100);
    strcat(response, temp);
    strcat(response, ",");
    strcat(response, message->data[message->currentHop - 1].routeName);
    strcat(response, ",");
    strcat(response, message->data[message->currentHop - 1].departingFrom);
    strcat(response, ",");
    strcat(response, message->data[message->currentHop - 1].arrivalStation);
    strcat(response, ",");
    sprintf(temp, "%d:%d", message->data[message->currentHop - 1].arrivalTime/100,  message->data[message->currentHop - 1].arrivalTime%100);
    strcat(response, temp);
    strcpy(reply, response);
}

void send_udp(int port_number, char* message){
    struct sockaddr_in udp_addr;
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(port_number);
    int len = strlen(message);
    sendto(udp_sock, message, len, 0, (struct sockaddr*) &udp_addr, sizeof(udp_addr));
    close(udp_sock);
}

void delete_message(int index, struct client_server *my_server){
    for(int i = index; i < my_server->messages_count; i++){
        my_server->queries[i] = my_server->queries[i+1];
    }
    my_server->messages_count--;
}
void handle_message(messages *message, char *msg, struct timetable *station, struct client_server *my_server){
    parse_query(message, msg);
        if (DEBUG && message == NULL){
            printf("Failed to parse message/n");
        }
         // send positive response back to source if found in timetable
        route found;
        if (find_route(&found, message->data[message->currentHop].arrivalTime, message->destination, station)){
            message->currentHop++;
            message->data[message->currentHop] = found;
            char pos_response [MAX_LINESIZE];
            create_response(pos_response, "Result_Success", message);
        }
        // send response to all neighbours that havnt been visited
        else{
            for(int i = 0; i < my_server->neighbour_count; i++){
                route neighbour_route;
                for(int j = 0; j < message->currentHop; j++){
                    if(strcmp(my_server->neighbour_list[i].name, message->data[j].arrivalStation)){
                        break;
                    }
                }
                // send query to this neighbour
                if (find_route(&neighbour_route, message->data[message->currentHop].arrivalTime, my_server->neighbour_list[i].name, station)){
                    create_query(msg, &neighbour_route);
                    send_udp(my_server->neighbour_list[i].port, msg);
                    message->currentHop++;
                    message->responses_needed++;
                }
                else{
                    printf("neighbour not found/n");
                }
                my_server->queries[my_server->messages_count] = *message;
                my_server->messages_count++;
            }
            // send neg response back to source
            if (message->responses_needed == 0){
                char* neg_response = create_response("Result_Fail", message);
                for(int i = 0; i < my_server->neighbour_count; i++){
                    if (strcmp(my_server->neighbour_list[i].name, message->data[message->currentHop].departingFrom)){
                        send_udp(my_server->neighbour_list[i].port, neg_response);
                    }
                }
            }
        }

}
void handle_response(response *message, char *msg, struct timetable *station, struct client_server *my_server){
    parse_response(message, msg);
        if (DEBUG && message == NULL){
            printf("Failed to parse message/n");
        }
        ///add response to list of responses
        if(strcmp(message->data[0].departingFrom, my_server->name) == 0){
            my_server->responses[my_server->messages_count] = message;
            my_server->responses_count++;
            return;
        }
        for (int i = 0; i < my_server->messages_count; i++){
            bool keepgoing = false;
            int j = 0;
            while(!strcmp(my_server->name, message->data[message->currentHop].arrivalStation)){
                if (!strcmp(my_server->queries[i]->data[j]->arrivalStation, message->data[j].arrivalStation)) {
                    keepgoing = true;
                    break;
                }
                j++;
            }
            if(keepgoing){
                continue;
            }
            my_server->queries[my_server->messages_count]->responses[my_server->queries[my_server->messages_count]->current_response_count] = message;
            my_server->queries[my_server->messages_count]->current_response_count++;

        }
        /// if all respnces have been received then send best one back to source
        if (message->current_response_count == message->responses_needed){
            response best_response;
            int lowest_time = 1000000;
            int i;
            for(i = 0; i < message->current_response_count; i++){
                if(message->responses[i].data[message->currentHop].arrivalTime < lowest_time){
                    if(strcmp(message->responses[i].result,"Result_Success")){
                        lowest_time = message->responses[i].data[message->currentHop].arrivalTime;
                        best_response = message->responses[i];
                    }
                }
            }
            if (lowest_time == 1000000){
                int j = message->currentHop;
                while(!strcmp(message->data[j].arrivalStation, my_server->name)){
                    j--;
                }
                j--;
                char* neg_response;
                create_response(neg_response, "Result_Fail", message);
                for(int x = 0; x < my_server->neighbour_count; x++){
                    if (strcmp(my_server->neighbour_list[x].name, message->data[j].departingFrom)){
                        send_udp(my_server->neighbour_list[x].port, neg_response);
                    }
                }
                delete_message(i, my_server);
                return;
            }
            ///send it to next position if successfull repsonce was found
            int j = best_response.currentHop;
            while(!strcmp(best_response.data[j].arrivalStation, my_server->name)){
                j--;
            }
            j--;
            for(int x = 0; x < my_server->neighbour_count; x++){
                if(strcmp(my_server->neighbour_list[x].name, best_response.data[j].arrivalStation)){
                    char* res;
                    create_response(res,"Result_Success",&best_response);
                    send_udp(my_server->neighbour_list[x].port, res);
                    break;
                }
            }
            delete_message(i, my_server);
        }
}

void process_name_message(char* message, struct client_server *my_server, struct timetable *station){
    if(strcmp(message[5],"N")){ // Type is name
        char* delim= ";";
        char* delim2= "\n";
        char* token;
        char* token2;
        char* name;
        token = strtok(message, delim);
        token2 = strtok(NULL, delim);
        strcpy(my_server->neighbour_list[my_server->neighbours_added].port, atoi(token2));
        strtok(token, delim2);
        name = strtok(NULL, delim2);
        strcpy(my_server->neighbour_list[my_server->neighbours_added].name, name);
        my_server->neighbour_list[my_server->neighbours_added].added = true;
        my_server->neighbours_added++;
        char* own_name;
        create_name_message(my_server, own_name);
        send_udp(atoi(token2), own_name);
        return;
    }
    if (strcmp(&message[5], "Q") == 0){
        handle_message(message, station, my_server);
    }
    else if (strcmp(&message[5], "R") == 0){
        handle_response(message, station, my_server);
    }
}

int find_destination(char* message){
    char* delim= ",";
    char* token;
    token = strtok(message, delim);
    return atoi(token);
}

void* udp_port(struct client_server *my_server, struct timetable *station){
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
        //do something with received mesage
        process_name_message(rec_message, my_server, station);
    }
    return NULL;
}

void send_name_out(struct client_server *my_server){

    char* temp;
    create_name_message(my_server, temp);
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

void server_listen(struct client_server *my_server, struct timetable *station){
    send_name_out(my_server);
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
        // search for destination in timetable
        route neighbour_route;
        if (find_route(&neighbour_route, 0, destination, station)){
            char* response = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>";
            strcat(response, neighbour_route.departureTime);
            strcat(response, neighbour_route.routeName);
            strcat(response, neighbour_route.departingFrom);
            strcat(response, neighbour_route.arrivalTime);
            strcat(response, neighbour_route.arrivalStation);
            strcat(response, "</h1></body></html>");
            send(accept_sock, response, sizeof(response), 0);
            close(accept_sock);
            continue;
        }
        // send query to all neighbours
        for (int i = 0; i < my_server->neighbour_count; i++){
            if (find_route(&neighbour_route, 0, my_server->neighbour_list[i].name, station)){
                char* query = "Type_Query\n";
                strcat(query, destination);
                strcat(query, "\n");
                create_query(query, &neighbour_route);
                send_udp(my_server->neighbour_list[i].port, query);
                my_server->messages_count++;
            }
        }
        
        while(1){
            if(my_server->responses_count == my_server->neighbour_count){
                break;
            }
        }
        // send best response back to client
        response* best_response;
        int lowest_time = 1000000;
        for(int i = 0; i < my_server->responses_count; i++){
            if(my_server->responses[i]->data[my_server->responses[i]->currentHop].arrivalTime < lowest_time){
                if(strcmp(my_server->responses[i]->result,"Result_Success")){
                        lowest_time = my_server->responses[i]->data[my_server->responses[i]->currentHop].arrivalTime;
                        best_response = &my_server->responses[i];
                    }
            }
        }
        if (lowest_time == 1000000){
            send(accept_sock, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>NO ROUTE FOUND</h1></body></html>", sizeof("HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>NO ROUTE FOUND</h1></body></html>"), 0);
            close(accept_sock);
            continue;
        }
        char* response = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>";
        for(int i = 0; i < best_response->currentHop; i++){
            strcat(response, best_response->data[i].departureTime);
            strcat(response, best_response->data[i].routeName);
            strcat(response, best_response->data[i].departingFrom);
            strcat(response, best_response->data[i].arrivalTime);
            strcat(response, best_response->data[i].arrivalStation);
        }
        strcat(response, "</h1></body></html>");
        send(accept_sock, response, sizeof(response), 0);
        close(accept_sock);
    }
}

int main(int argc, char const *argv[]){
    struct timetable station;
    char* filename = "tt-";
    strcat(filename, argv[1]);
    read_timetable(filename, &station);
    struct client_server my_server;
    my_server.responses_count = 0;
    my_server.messages_count = 0;
    my_server.neighbours_added = 0;
    bool message_out_flag = false;
    int num_neighbours = argc-4;
    my_server.neighbour_count = num_neighbours;

    /**/
    my_server.neighbour_list = (struct neighbours*)malloc(num_neighbours*sizeof(neighbours));

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
    server_listen(&my_server, &station);
    return 0;
}