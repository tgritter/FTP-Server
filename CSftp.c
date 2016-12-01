
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "dir.h"
#include "usage.h"
#include "Thread.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#define NUM_CLIENT 4
#define BACKLOG 10     // how many pending connections queue will hold

char *socket_IP[52];
int* socketPorts[4];
int* socketLogins[4];
char* socketHelper(char* PORT, int data);
int my_strlen(char *string); //Function to calculate length of given string
void *threading_handler(void *threadid);
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

char* concat(char* str1, char* str2){
char * new_str ;
if((new_str = malloc(strlen(str1)+strlen(str2)+1)) != NULL){
    new_str[0] = '\0';   // ensures the memory is an empty string
    strcat(new_str,str1);
    strcat(new_str,str2);
    return new_str;
} else {
	exit(0);
    // exit?
}
}

int my_strlen(char *string) //Function to calculate length of given string

{
    int i;
    for(i=0;string[i]!='\0';i++);
    return i;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// read a line from the client
void readLine(int new_fd, char* buff){
	recv(new_fd, buff, 2000, 0);
	printf("contents of buff are %s\n", buff);
}

void str_server(int sock) 
{ 
    char buf[1025]; 
    const char* filename = "test.txt"; 
    FILE *file = fopen(filename, "rb"); 
    if (!file)
    {
        printf("Can't open file for reading"); 
        return;
    }
    while (!feof(file)) 
    { 
        int rval = fread(buf, 1, sizeof(buf), file); 
        if (rval < 1)
        {
            printf("Can't read from file");
            fclose(file);
            return;
        }

        int off = 0;
        do
        {
            int sent = send(sock, &buf[off], rval - off, 0);
            if (sent < 1)
            {
                // if the socket is non-blocking, then check
                // the socket error for WSAEWOULDBLOCK/EAGAIN
                // (depending on platform) and if true then
                // use select() to wait for a small period of
                // time to see if the socket becomes writable
                // again before failing the transfer...

                printf("Can't write to socket");
                fclose(file);
                return;
            }
            
            off += sent;
        }
        while (off < rval);
    } 

    fclose(file);
}

// returns 1 if this port is logged in
int IsLoggedIn(char* PORT){
	int i;
	for (i = 0; i < 4; i++){
		if (socketPorts[i] == PORT && socketLogins[i] == 1 ) {
			return 1;
		}
	}
	return 0;
}

// set this port to logged in
void login(char* PORT) {
	int i;
	for (i = 0; i < 4; i++){
		if (socketPorts[i] == PORT ) {
			socketLogins[i] = 1;
		}
	}
}

// return index of the current port in socket table
int PortIndex(char* PORT){
	int i;
	for (i = 0; i < 4; i++){
		if (socketPorts[i] == PORT ) {
			return i;
		}
	}
	return 0;
}


char*  parseInput(char* buff, int new_fd, char* PORT){
	char *returnbuff;
    	char sockIP[INET6_ADDRSTRLEN];
	int inputCase;
	int LoggedIn = IsLoggedIn(PORT);
	
	if (LoggedIn){
	char newline = '\n';
		struct addrinfo hints, *res;
		int sockfd;
	if (strncmp(buff, "USER cs317", 10) == 0 ){
		inputCase = 0;
		}
	if (strncmp(buff, "QUIT", 4) == 0){
		inputCase = 1;
		}
	if (strncmp(buff, "TYPE I", 6) == 0){
		inputCase = 2;
		}
	if (strncmp(buff, "TYPE A", 6) == 0){
		inputCase = 2;
		}
	if (strncmp(buff, "MODE S", 6) == 0){
		inputCase = 3;
		}
	if (strncmp(buff, "STRU F", 6) == 0){
		inputCase = 4;
		}
	if (strncmp(buff, "RETR", 4) == 0){
		inputCase = 5;
		}
	if (strncmp(buff, "PASV", 4) == 0){
		inputCase = 6;
		}
	if (strncmp(buff, "NLST", 4) == 0){
		inputCase = 7;
		}
	switch(inputCase)
	{
		case 0:
			returnbuff = "230 User logged in." ; 
			break;
		case 1:
			returnbuff = "Closing Socket";
			close(new_fd);
			
			break;
		case 2:
			returnbuff = "TYPE: Image";
			send(new_fd, "200 success code", 16, 0);
			send(new_fd, &newline, 1, 0);			
			break;
		case 3:
			returnbuff = "Mode: Stream";
			send(new_fd, "200 success code", 16, 0);
			send(new_fd, &newline, 1, 0);
			break;
		case 4:
			returnbuff = "STRU: File Structure";
			send(new_fd, "200 success code", 16, 0);
			send(new_fd, &newline, 1, 0);
			break;
		case 5:
			returnbuff = "Transfering File";
			str_server(new_fd);
			break;
		case 6:
			returnbuff = "227 Entering Passive Mode (";
			printf("portIndex is %d\n", PortIndex(PORT));
			int port = PortIndex(PORT) + 4005;
			char charport[5];
			sprintf(charport, "%c", port);
			printf("port is %d\n", port);
			socketHelper(charport, 1);
			int p1 = round(port/256);
			int p2 = port % 256;
			int i;
			char* tempIP = socket_IP;
			int ip_len = my_strlen(tempIP);
			for(i = 0; i < ip_len; i++){
				if((char)tempIP[i] == '.'){
					tempIP[i] = ',';
				}
			}
			char p1_char[3];
			sprintf(p1_char, "%d", p1);
			char p2_char[3];
			sprintf(p2_char, "%d", p2);
			char* p1p2 = concat(p1_char, ",");
			p1p2 = concat(p1p2, p2_char);
			char *return_address = strcat(socket_IP, ",");
			return_address = strcat(return_address, p1p2);
			returnbuff = concat(returnbuff, return_address);
			returnbuff = concat(returnbuff, ")");
			break;
		case 7:
			returnbuff = "Returning Name List";
			printf("Printed %d directory entries\n", listFiles(new_fd, "."));
			break;
		default :
			returnbuff = "500 Syntax error, command unrecognized.";
	}
	
}
	else{
		
	if (strncmp(buff, "USER cs317", 10) == 0 ){
		login(PORT);
		returnbuff = "230 User logged in." ; }
	else {
		returnbuff = "430 Invalid login.";
	}
}
	return returnbuff;
}
// if data == 1, is a data port and won't send/recieve text
char *socketHelper(char* PORT, int data)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    void* addr;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char sockIP[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return "err";
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }


        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
	
	void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
	inet_ntop(servinfo->ai_family, servinfo->ai_addr, socket_IP, sizeof socket_IP);	
	printf("Servinfo addr: %s\n", socket_IP);
	
	   break;
    }
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }



    if(!data){
    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
    	
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        printf("New FD: %d\n", new_fd);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
	
	while(1) {
	
	char newline = '\n';
	send(new_fd, "ftp> ", 5, 0);
	char buff[16000];
	readLine(new_fd, buff);
	printf("the buff is: %s\n", buff);
	char* returnVal = parseInput(buff, new_fd, PORT);
	printf("contents of returnVal are: %s\n", returnVal);
	send(new_fd, returnVal,my_strlen(returnVal) , 0);
	send(new_fd, &newline, 1, 0);
	
	if (strncmp(returnVal, "Closing Socket", 7) == 0){
		break;
		}
	
	
	}

    }
}
else {
	printf("Opened secondary port\n");
}

    return sockIP;
}

//Helper function for threading
void *threading_handler(void *threadid)
{
	char* PORT_THREAD;
	int threadnum = (int)threadid;
	printf("ThreadNum: %d\n", threadnum);
	switch(threadnum)
	{
		case 1:
			PORT_THREAD = "3001";
			break;
		case 2:
			PORT_THREAD = "3002";
			break;						
		case 3:
			PORT_THREAD = "3003";
			break;
		case 4:
			PORT_THREAD = "3004";
			break;
		default:
			PORT_THREAD = "3000";
	}
	
	socketPorts[threadnum] = PORT_THREAD;
	socketLogins[threadnum] = 0;
	socketHelper(PORT_THREAD, 0);
	
}
	
	

int main(int argc, char **argv){
	char* PORT;
	int i;
    if (argc != 2) {
   	usage(argv[0]);
	return -1;
	}
	else{
	PORT = strdup(argv[1]);
	}
	
	//Threading loop
	for (i=1; i<=NUM_CLIENT; i++) {
        if(runThread(createThread(&threading_handler, (void*) i), NULL) < 0)
        {
            perror("could not create thread");
            return 1;
        }
        sleep(2);
        
    }
    pthread_exit(NULL);
	
	
	
}
