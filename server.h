#include <stdlib.h>
#include <string.h> // for strlen
#include <strings.h> // for bzero
#include <unistd.h>
#include <algorithm>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include<netdb.h>
#include <fcntl.h>

///////////////////////////
// Forwarded Subroutines //
///////////////////////////

void gossipBroadcast();

void gossipCallback(void* , int, char, char);

void sendUDP( char *, char *, int );

void sendTCP( std::string, std::string, int );

void signal_stop (int);void sendUDP(std::string, std::string, int);

void sendTCP(std::string, std::string, int);

void broadcastGossip(char *);

// Parses the user commands and stores them in an sqlite3
// database
char* reader (std::string);

//handler for the tcp socket
void tcp_handler (int);

// handler for the udp socket
void udp_handler (int , sockaddr*, socklen_t );

// run when child is created
void sig_child (int signo);

// sets up sqlite database for storing gossip
void setup_database (char *filePath);
