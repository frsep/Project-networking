//  -----------------------------------------------------------------------------------------
//  PROJECT HEADER

//  CITS3002 Project 1 2024
//  Student1:   23715959    Malachy McGrath
//  Student2:   23616047    Fin     O'Loughlin
//  Student3:   23342221    Sepehr  Amid
//  Student4:   21713972    Josh    Ong


//  myscheduler (v1.0)
//  Compile with:  cc -std=c11 -Wall -Werror -o serverc server.c -lm


//  -----------------------------------------------------------------------------------------
//  HEADER FILES

#include <sys/socket.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>


//  -----------------------------------------------------------------------------------------
//  CONSTANTS

#define CHAR_COMMENT                    '#'
#define MAX_WORDSIZE                    256


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
{   // Departure struct to hold data of a single departure route in a Timetable
    int departureHour;
    int departureMinute;
    char *routeName;
    char *departingFrom;
    int arrivalHour;
    int arrivalMinute;
    char *arrivalStation;
} departure;


struct timetable
{// Timetable struct to hold station name, lat/lon, and array of all departure routes
    //  station-name,longitude,latitude,routes
    char *stationName;
    float longitude;
    float latitude;
    departure *route;
    int nroutes; 
};


struct client_server
{// SEPEHR - could you please provide comments for what this struct is/does
    int browser_port;
    char name[MAX_WORDSIZE]; //SEPEHR- I've replaced with a pre-defined CONSTANT as best practise - delete this comment once read
};


//  -----------------------------------------------------------------------------------------
//  FUNCTIONS

int is_comment_line(char line[])
{// checks if a line is a comment line
    int i = 0;
    while(isspace(line[i] != 0)){  //checks if character is a white space character
        ++i;
    }
    if (line[i] == '#'){
        return true; //indicates the line is a comment line
    }else{
        return false; //indicates the line is not a comment line
    }
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


void read_timetable(char filename[])
{// Function to read csv file and load timetable data into structures.
    FILE *tt = fopen(filename, "r");                   // attempt to open file

    if(tt == NULL){                                    // checks for errors in opening file
        printf("could not open sysconfig file '%s'\n", filename);
        exit(EXIT_FAILURE);                             //terminates if file can't be opened
    }
    //reading file
    char line[BUFSIZ];// stores contents of one line at a time as a character array
    while (fgets(line,sizeof line, tt) != NULL){//until a line is empty (end of file reached)

        trim_line(line); // removes the \n or \r at end of line

        if (is_comment_line(line) == 1){ //skips to next line if its a comment line
            continue;
        }
        
        // departure-time,route-name,departing-from,arrival-time,arrival-station
        
        if (device_or_timequantum(line) == 1){

            char bin[BUFSIZ];
            char readspeed[BUFSIZ];
            char writespeed[BUFSIZ];

                //columns are:   'device     devicename      readspeed       writespeed'
            sscanf(line, "%s %s %s %s", bin, devices[n_devices].name, readspeed, writespeed);
            devices[n_devices].readspeed = atoi(readspeed);
            devices[n_devices].writespeed = atoi(writespeed);

            ++n_devices;
        } 
        else if (device_or_timequantum(line) == 2){
            char bin[BUFSIZ];
            char timeqnt[BUFSIZ];

            sscanf(line, "%s %s", bin, timeqnt);
            time_quantum = atoi(timeqnt);
        }

    }
    fclose(tt); //closes timetable file when end of file reached
}

void find_route()
{   // Function to evaluate the optimal route to destination (within file)

}

// SEPEHR - could you please provide comments?
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
    struct client_server my_server;
    my_server.browser_port = atoi(argv[2]);
    strcpy(my_server.name, argv[1]);
    server_listen(&my_server);
    return 0;
}
