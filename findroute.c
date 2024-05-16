//  -----------------------------------------------------------------------------------------
//  PROJECT HEADER
//  CITS3002 Project 1 2024
//  Student1:   23715959    Malachy McGrath
//  Student2:   23616047    Fin     O'Loughlin
//  Student3:   23342221    Sepehr  Amid
//  Student4:   21713972    Josh    Ong

//  -----------------------------------------------------------------------------------------
//  HEADER FILES
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

//  -----------------------------------------------------------------------------------------
//  CONSTANTS
#define CHAR_COMMENT                    '#'
#define MAX_WORDSIZE                    60 
#define MAX_LINESIZE                    256 
#define MAX_THREADS                     100
#define MAX_HOPS                        100
#define MAX_DEPARTURES                  100
#define DEBUG                           true

//  -----------------------------------------------------------------------------------------
//  PRE-PROCESSOR MACROS
#define CHECKALLOC(p)   \
    //  Check memory allocation worked
    do { if((p) == NULL) { \
    fprintf(stderr, "allocation failed - %s:%s(), line %d\n",__FILE__,__func__,__LINE__); \
    exit(2); } \
} while(false)

//  -----------------------------------------------------------------------------------------
//  DATA STRUCTURES
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
    char result[MAX_WORDSIZE];
    char destination[MAX_WORDSIZE];
    route data[MAX_HOPS];
    int nhops;
} message;

struct timetable
{// Timetable struct to hold station name, lat/lon, and array of all departure routes
    char stationName[MAX_WORDSIZE];
    float longitude;
    float latitude;
    route departures[MAX_DEPARTURES];
    int nroutes; 
}

struct client_server
{  // struct stores the inputs given into the program for the server
    int browser_port;
    int query_port;
    char name[MAX_WORDSIZE]; 
}

//  -----------------------------------------------------------------------------------------
//  FUNCTIONS
void parse_message(message* message, char* msg)
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
    parse_data(message, msg_data)
}

void parse_data(message* message, char* msg_data)
{// Parse data into its component hops
    char msg_hop[MAX_LINESIZE];
    char departTime[MAX_WORDSIZE];
    char routeName[MAX_WORDSIZE];
    char departingFrom[MAX_WORDSIZE];
    char arrivalTime[MAX_WORDSIZE];
    char arrivalStation[MAX_WORDSIZE];

    message->nhops = 0;
    while (sscanf(msg_data, "%s;%s", msg_hop, msg_data) > 0){
        sscanf( msg_hop, "%s, %s, %s, %s, %s", departTime, routeName, departingFrom, arrivalTime, arrivalStation);
                message->data[message->nhops].departureTime = atoi(departTime);
        strcpy( message->data[message->nhops].routeName, routeName);
        strcpy( message->data[message->nhops].departingFrom, departingFrom);
                message->data[message->nhops].arrivalTime = atoi(arrivalTime);
        strcpy( message->data[message->nhops].arrivalStation, arrivalStation);      
        ++message->nhops;
    }
}

void handle_response(message){

}

void best_response(response_array){

}

void create_response(message, address, result){

}

void handle_response(message){

}

void handle_query(char *msg)
{
    message *message;
    parse_message(message, msg);
    if (DEBUG && message == NULL){
        printf("Failed to parse message/n");
    }
    



}

void create_query(char *final_destination, int time)
{// Create query to Final Destination
    char *query[MAX_LINESIZE];
    char *hop[MAX_LINESIZE];
    find_route(hop, time, final_destination);
    query = strcpy(query, "Type_Query\n");
    query = strcat(query, strcat(strcat("Destination_", final_destination), "/n"));
    query = strcat(query, strcat(strcat("Data_", hop), "/n"));
}

bool find_route(char *hop, int time, char *destination)
{// Check for possible route to destination within current station
    route *found;
    for (int i = 0; i<station->nroutes; i++){
        if (time < station->departures[i].departTime){
            if (strcmp(station->departures[i].arrivalStation, destination)){
                found = station->departures[i];
                return true;
            } 
        }
    }
    return false;
}

void route_string(route *found, char *hop)
{// converts route to string representing that hop
    char departHour[MAX_WORDSIZE];
    char departMinute[MAX_WORDSIZE];
    char arrivalHour[MAX_WORDSIZE];
    char arrivalMinute[MAX_WORDSIZE];
    sprintf(departHour, "%d", found->departureTime / 100);
    sprintf(departMinute, "%d", found->departureTime % 100);
    strcat(departHour, ":");
    strcat(departHour, departMinute);
    strcpy(hop, departHour);
    strcat(hop, ",");
    strcat(hop, found->routeName);
    strcat(hop, ",");
    strcat(hop, found->departingFrom);
    strcat(hop, ",");
    sprintf(arrivalHour, "%d", found->arrivalTime / 100);
    sprintf(arrivalMinute, "%d", found->arrivalTime % 100);
    strcat(arrivalHour, ":");
    strcat(arrivalHour, arrivalMinute);
    strcat(hop, arrivalHour);
    strcat(hop, ",");
    strcat(hop, found->arrivalStation);
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

time_t get_time()
{
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  if (DEBUG){
    printf ( "Current local time and date: %s", asctime (timeinfo) );
  }
  return rawtime;
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
                    station->departures[station->nroutes]->departureTime = atoi(departTime);
            strcpy( station->departures[station->nroutes]->routeName, routeName);
            strcpy( station->departures[station->nroutes]->departingFrom, departingFrom);
                    station->departures[station->nroutes]->arrivalTime = atoi(arrivalTime);
            strcpy( station->departures[station->nroutes]->arrivalStation, arrivalStation);
            ++station->nroutes;
        }
    }
    fclose(tt); //closes timetable file when end of file reached
}


void server_listen(struct client_server *my_server)
{   // creates socket and binds it to the browser port for TCP connection
    char client_req[1000];
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tcp_addr;
    //sets it to ipv4
    tcp_addr.sin_family = AF_INET;
    //allows it to connect to a nonspecific ip
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    //adds port int to the sockadd_in using hton to convert to correct format
    tcp_addr.sin_port = htons(my_server->browser_port);
    int bind_sock = bind(sock,(struct sockaddr*) &tcp_addr, sizeof(struct sockaddr_in));
    int listen_sock = listen(sock, 1);
    socklen_t addrlen = sizeof(tcp_addr);

    // creates socket and binds it with the query port for UDP connection
    int sock2 = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in udp_addr;
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(my_server->query_port);
    int bind_udpsock = bind(sock2,(struct sockaddr*) &udp_addr, sizeof(struct sockaddr_in));
    int listen_udpsock = listen(sock2, 1);
    socklen_t addrlen2 = sizeof(udp_addr);

    while(true){
        int accept_sock = accept(sock, (struct sockaddr*) &tcp_addr, &addrlen);
        read(accept_sock, client_req, sizeof(client_req));
        // takes out the destination name out of the client_req string
        char* delim= "=";
        char* token;
        token = strtok(client_req, delim);
        token = strtok(NULL, delim);
        // destination is the name of the destination station
        char destination[MAX_WORDSIZE];
        for (int i = 0; i < MAX_WORDSIZE; i++){
            if (token[i] == ' ') {
                destination[i] = '\0';
                break;
            }
            destination[i] = token[i];
        }
        // response is the response that the server will send to the client
        char respnce[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>Hello, World!</h1></body></html>";
        send(accept_sock, respnce, sizeof(respnce), 0);
        close(accept_sock);
    }
}

void communicate_with_other_server(){
    return;
}

int main(int argc, char const *argv[]){
    // struct client_server my_server;
    // my_server.query_port = atoi(argv[3]);
    // my_server.browser_port = atoi(argv[2]);
    // strcpy(my_server.name, argv[1]);
    // server_listen(&my_server);
    return 0;
}
