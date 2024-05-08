//  HEADER FILES

#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>


//  PRE-PROCESSOR MACROS

//  Check memory allocation worked
#define CHECKALLOC(p)	do { if((p) == NULL) { \
    fprintf(stderr, "allocation failed - %s:%s(), line %d\n",__FILE__,__func__,__LINE__); \
    exit(2); } \
} while(false)


//  DATA STRUCTURES

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
    int nroutes; 
};

// Sepeh - could you please provide comments?
struct client_server{
    int browser_port;
    char name[256];
};

// Function to read csv file and load timetable data into structures.
void read_timetable(char filename[]) 
{
        FILE *sys = fopen(filename, "r");                   // attempt to open sysconfig file

        if(sys == NULL){                                    // checks for errors in opening sysconfig file
            printf("could not open sysconfig file '%s'\n", filename);
            exit(EXIT_FAILURE);                             //terminates if file can't be opened
        }
        //reading file
        char line[BUFSIZ];// stores contents of one line at a time as a character array
        while (fgets(line,sizeof line, sys) != NULL){//until a line is empty (end of file reached)

            trim_line(line); // removes the \n or \r at end of line

            if (is_comment_line(line) == 1){ //skips to next line if its a comment line
                continue;
                
            }
            if (device_or_timequantum(line) == 1){

            char bin[BUFSIZ];
            char readspeed[BUFSIZ];
            char writespeed[BUFSIZ];

                //colums are:   'device     devicename      readspeed       writespeed'
            sscanf(line, "%s %s %s %s", bin, devices[n_devices].name, readspeed, writespeed);
            devices[n_devices].readspeed = atoi(readspeed);
            devices[n_devices].writespeed = atoi(writespeed);

            ++n_devices;
            } else if (device_or_timequantum(line) == 2){
                char bin[BUFSIZ];
                char timeqnt[BUFSIZ];

                sscanf(line, "%s %s", bin, timeqnt);
                time_quantum = atoi(timeqnt);
            }

        }
        fclose(sys); //closes system config file when end of file reached

}

// Function to evaluate the optimal route to destination
void find_route(){
}

// Sepeh - could you please provide comments?
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
