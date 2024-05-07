#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>

//  JUST A VERY HELPFUL MACRO
#define CHECKALLOC(p)	do { if((p) == NULL) { \
    fprintf(stderr, "allocation failed - %s:%s(), line %d\n",__FILE__,__func__,__LINE__); \
    exit(2); } \
} while(false)


// Departure struct to hold data of a single departure route in a Timetable
typedef struct{
    // departure-time,route-name,departing-from,arrival-time,arrival-station
    int departureHour;
    int departureMinute;
    char *routeName;
    char *departingFrom;
    int arrivalHour;
    int arrivalMinute;
    char *arrivalStation;
} departure;

// Timetable struct to hold station name, lat/lon, and array of all departure routes
struct timetable{
    //  station-name,longitude,latitude,routes
    char *stationName;
    float longitude;
    float latitude;
    departure *route; 
};

struct client_server{
    int browser_port;
    char name[256];
};

// Function to read csv file and load timetable data into structures.
void read_timetable(){
}

// Function to evaluate the optimal route to destination
void find_route(){
}

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
        printf("%s", client_req);
        char respnce[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>Hello, World!</h1></body></html>";
        send(accept_sock, respnce, sizeof(respnce), 0);
        close(accept_sock);
    }
    
}
void communicate_with_other_server(){
    return;
}

int main(int argc, char const *argv[]){
    struct client_server my_server;
    my_server.browser_port = atoi(argv[2]);
    strcpy(my_server.name, argv[1]);
    server_listen(&my_server);
    return 0;
}
