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

#include<stdio.h>  
#include <stdlib.h>  
#include<unistd.h>  
#include<string.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  
#include<netdb.h>
#include <string>
using namespace std;

//Maximum the buff size as 512.
#define MAXDATASIZE 512
#define OPT_UDP  1
#define OPT_TCP  2

struct hostent *he;  
struct sockaddr_in server;  
int udpfd;
int tcpfd;
int num;
string host;
int port;
string gossip;
string timestamp;


void udp_client () {

	//Check whether the socket is created successfully.
	if ((udpfd=socket(AF_INET, SOCK_DGRAM,0))==-1) {  
			printf("Fail to create the socket! \n");  
        exit(1);  
        }  
		
	// Initial UDP client.	
		bzero(&server,sizeof(server));  
        server.sin_family = AF_INET;  
        server.sin_port = htons(port);  
        server.sin_addr= *((struct in_addr *)he->h_addr);  
		
	//Create the socket.
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
}

//Use the same sockaddr for tcp as well.
void tcp_client () {
	//Check whether the socket is created successfully.
	if ((tcpfd=socket(AF_INET, SOCK_DGRAM,0))==-1) {  
			printf("Fail to create the socket! \n");  
        exit(1);  
    }  
		
	tcpfd = socket(AF_INET, SOCK_STREAM, 0);
		
	if (connect(tcpfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        printf("TCP connection error!\n");
	}
}

void parallel(int option, int argc, char *argv[]) {
	
	struct hostent *he;
    char *buf = new char[MAXDATASIZE];
    
    if (option == OPT_UDP) {
		//Send data.
        sendto(udpfd, argv[2],strlen(argv[2]),0,(struct sockaddr *)&server,sizeof
            (server));  
        socklen_t  addrlen; 
		
		//Size of the server.
        addrlen=sizeof(server);  
	
		//Print the buff.
		buf[num]='\0';  
		printf("Server Message:%s\n",buf); 
	
        return;
    }

    //write(tcpfd, argc, *argv[2]);
	//Print the buff.
	buf[num-1]='\0';  
	printf("Server Message: %s\n",buf);
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
            case 'p' : port = atoi (optarg); break;
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
    
    getCommandLineArgs (argc, argv);
}

