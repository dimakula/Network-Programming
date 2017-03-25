/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2017 
                Authors: Dmitry Kulakov, Xiaohan Yang, Nicholas Zynko
                Florida Tech, Software Engineering
   
       This program is free software; you can redistribute it and/or modify
       it under the terms of the GNU Affero General Public License as published by
       the Free Software Foundation; either the current version of the License, or
       (at your option) any later version.
   
      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.
  
      You should have received a copy of the GNU Affero General Public License
      along with this program; if not, write to the Free Software
      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              */
/* ------------------------------------------------------------------------- */

#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h>
#include <string>
#include <iostream>
#include <errno.h>

using namespace std;

//Maximum the buff size as 512.
#define MAXDATASIZE 512
#define OPT_UDP  1
#define OPT_TCP  2
#define OPT_GOSSIP 3
#define OPT_PEER 4
#define OPT_EXIT 5

struct hostent *he;  
struct sockaddr_in server;  
int num;
string host;
string port;
string gossip;
string timestamp;

void printWelcomeScreen () {
    printf ("welcome! You have connected to %s through port %s\n", 
            host.c_str(), port.c_str());
}

void printUsage () {
    printf ("Usage: Type \"Gossip\" if you wish to send a new message\n");
    printf ("Type \"Peer\" if you wish to add another peer into the database\n");
    printf ("Leave the prompt blank if you wish to exit the client program\n");
}

int printUserPrompt () {

    string command;
	printf ("Gossip message sent\n");
    printf ("Please enter next command: ");
    int flag = 0;
	    
	do {
        getline (cin, command);
	       
	    if (command.empty()) {
	        flag = OPT_EXIT;
	    
	    } else if (command == "Gossip") {
	        flag = OPT_GOSSIP;
	    
	    } else if (command == "Peer") {
	        flag = OPT_PEER;
	    
	    } else {
	        printUsage ();
	    }
	
	} while (flag == 0);
	
	return flag; 
}

int udp_client () {

    int  udpfd, num, recieved;
    struct addrinfo hints, *servinfo, *p;
    char  buffer [MAXDATASIZE];  
	struct sockaddr_in peer;
    
    bzero (&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    if ((recieved = getaddrinfo (host.c_str(), port.c_str(), &hints, &servinfo)) 
            != 0) {
        fprintf (stderr, "Broadcast: Address does not exist\n");
        return -1;
    }
    
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((udpfd = socket (p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror ("Broadcast");
            continue;
        }
        break;
    }

	if (p == NULL) {
	    fprintf (stderr, "UDP: Failed to create socket\n");
	    return -1;
	}
	
	printWelcomeScreen ();
    printUsage ();
    int command;
	
	do {
		int totalSent = 0; 
	    int bytes;
	     
	    printf ("sending %s\n", gossip.c_str());
	    
	    while (totalSent < gossip.length()) {
	      
	        if ((bytes = sendto (udpfd, gossip.c_str(), gossip.length(), 0, 
	                p->ai_addr, p->ai_addrlen)) == -1) {
	            
	            if (errno == EINTR) continue;
	            perror ("UDP");
	            exit (1);
	        }
      
	        totalSent += bytes;
	    }
	    
	    printf ("finished sending\n\n");
	    command = printUserPrompt();
	
	} while (command != OPT_EXIT);
	
	
	freeaddrinfo (servinfo);
	close (udpfd);
	return 0;
}

//Use the same sockaddr for tcp as well.
int tcp_client () {

    int tcpfd;
	//Check whether the socket is created successfully.
	if ((tcpfd = socket(AF_INET, SOCK_DGRAM,0))==-1) {  
			printf("Fail to create the socket! \n");  
        exit(1);  
    }  
		
	tcpfd = socket(AF_INET, SOCK_STREAM, 0);
		
	if (connect(tcpfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        printf("TCP connection error!\n");
	}
}

int getCommandLineArgs (int argc, char *argv[]) {

    int flags = 0;
    char c;

    while ((c=getopt(argc, argv, "m:t::TUs:p:"))!=-1) {
    
        switch (c) {
            case 'm' : gossip = optarg; break;
            case 't' : timestamp = optarg; break;
            case 'T' : flags = flags | OPT_TCP; break;
            case 'U' : flags = flags | OPT_UDP; break;
            case 's' : host = optarg;
            case 'p' : port = optarg; break;
            default: 
                printf ("Usage: %s [-m <gossip>] [-t <timestamp>] [-T][-U]\n[-s <host>] [-p <port]\n",
                        argv[0]);
                exit(1);
        }
    }
    
    if (flags == OPT_UDP | OPT_TCP) {
        fprintf (stderr, "You can't do both TCP and UDP!\n");
        exit(1);
    
    } else if (flags == 0) {
        fprintf (stderr, "Please specify the protocol to use (-T for TCP, -U for UDP)\n");
        exit(1);
    }

    return flags;  
}

int main (int argc, char *argv[]) {
    
    int flag = getCommandLineArgs (argc, argv);
    
    if (flag == OPT_UDP)
        udp_client ();
    else
        tcp_client ();
}

