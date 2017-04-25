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
#include "../include/client-commands.h"
using namespace std;
//#include "../include/client.h"

#define MAXDATASIZE 2048
#define OPT_UDP  1
#define OPT_TCP  2
#define OPT_GOSSIP 3
#define OPT_PEER 4
#define OPT_EXIT 5
  
struct sockaddr_in server;  
int num;
string host;
string port;
//string gossip, timestamp;
char *gossip =    new char[MAXDATASIZE];
char *timestamp = new char[MAXDATASIZE];

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
    int flag = 0;
	    
	do {
	    printf ("Please enter next command: ");
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
	
	printf ("\n");
	return flag; 
}

/*void promptForPeer (char *peerName, char *peerIP, char *peerPort) {

    int length;
    printf ("Enter peer name: ");
    getline (peerName, &length, cin);
    length = 0;
    printf ("\nEnter peer ip: ");
    getline (peerIP, &length, cin);
    length = 0;
    printf ("\nEnter peer port: ");
    getline (peerPort, &length, cin);
    printf ("\n");
}*/

int udp_client () {

    int  udpfd, num, received;
    struct addrinfo hints, *servinfo, *p;
    char *buffer = new char [MAXDATASIZE];
    char *peerName = new char [MAXDATASIZE];
    char *peerIP = new char [MAXDATASIZE];
    char *peerPort = new char [MAXDATASIZE];
	struct sockaddr_in peer;
    
    bzero (&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    if ((received = getaddrinfo (host.c_str(), port.c_str(), &hints, &servinfo)) 
            != 0) {
        fprintf (stderr, "UDP: Address does not exist\n");
        return -1;
    }
    
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((udpfd = socket (p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror ("UDP");
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
    //string data = fullGossipMessage (gossip, timestamp); // data to send

    if (gossip[0] != 0)
       MessageEncode(gossip, timestamp, buffer); // message returns in buffer
	
	do {
		int totalSent = 0; 
	    int bytes;	        
	     
	    printf ("sending...\n");
	    
	    while (totalSent < strlen(buffer)) {
	      
	        if ((bytes = sendto (udpfd, buffer, strlen(buffer), 0, 
	                p->ai_addr, p->ai_addrlen)) == -1) {
	            
	            if (errno == EINTR) continue;
	            perror ("UDP");
	            exit (1);
	        }
      
	        totalSent += bytes;
	    }
	    
	    printf ("finished sending\n\n");
	    command = printUserPrompt();
	    
	    if (command == OPT_GOSSIP) {
	        printf ("Please enter new gossip message: ");
            cin.getline (gossip, MAXDATASIZE);
            //data = fullGossipMessage (gossip, NULL);
            MessageEncode(gossip, "", buffer);
	    
	    } else if (command == OPT_PEER) {
	        //promptForPeer (&peerName, &peerIP, &peerPort);
	        
            printf ("Enter peer name: ");
            cin.getline (peerName, MAXDATASIZE);
           
            printf ("\nEnter peer ip: ");
            cin.getline (peerIP, MAXDATASIZE);
            
            printf ("\nEnter peer port: ");
            cin.getline (peerPort, MAXDATASIZE);
            printf ("\n");
            
	        //data = fullPeerMessage (peerName, peerIP, peerPort);
	        PeerEncode (peerName, peerIP, peerPort, buffer);    
	    }
	    
	    gossip[0] = peerName[0] = peerIP[0] = peerPort[0] = buffer[0] = 0;
	
	} while (command != OPT_EXIT);
	
	
	freeaddrinfo (servinfo);
	close (udpfd);
	return 0;
}

//Use the same sockaddr for tcp as well.
int tcp_client () {

    char *buffer = new char [MAXDATASIZE];
    char *peerName = new char [MAXDATASIZE];
    char *peerIP = new char [MAXDATASIZE];
    char *peerPort = new char [MAXDATASIZE];
    
    struct addrinfo hints, *servinfo, *p;
    int rc, tcpfd, command;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
        perror ("Get address");
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((tcpfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("TCP: socket");
            continue;
        }

        if (connect(tcpfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(tcpfd);
            perror("TCP: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "TCP: failed to connect\n");
        return -1;
    }

    freeaddrinfo(servinfo);

	printWelcomeScreen ();
    printUsage ();
    
    if (gossip[0] != 0)
        MessageEncode(gossip, timestamp, buffer); // message returns in buffer;
    
    //string data = fullGossipMessage (gossip, timestamp); // data to send
	
	do {
		int totalSent = 0; 
	    int bytes;	        
	     
	    printf ("sending...\n");
	    
	    while (totalSent < strlen (buffer)) {
	      
	        if ((bytes = send (tcpfd, buffer, strlen(buffer), 0)) == -1) {
	            
	            if (errno == EINTR) continue;
	            perror ("TCP");
	            exit (1);
	        }

	        totalSent += bytes;
	    }
	    
	    printf ("\nfinished sending\n\n");
	    command = printUserPrompt();
	    memset (buffer, 0, MAXDATASIZE);
	    printf ("%s\n", buffer);
	    
	    if (command == OPT_GOSSIP) {
	        printf ("Please enter new gossip message: ");
            //cin.getline (gossip, MAXDATASIZE);
            cin.getline (gossip, MAXDATASIZE);
            
            //data = fullGossipMessage (gossip, NULL);
            MessageEncode(gossip, "", buffer);
	    
	    } else if (command == OPT_PEER) {
	        //promptForPeer (&peerName, &peerIP, &peerPort);
	        
            printf ("Enter peer name: ");
            cin.getline (peerName, MAXDATASIZE);
           
            printf ("\nEnter peer ip: ");
            cin.getline (peerIP, MAXDATASIZE);
            
            printf ("\nEnter peer port: ");
            cin.getline (peerPort, MAXDATASIZE);
            printf ("\n");
            
	        //data = fullPeerMessage (peerName, peerIP, peerPort);
	        PeerEncode (peerName, peerIP, peerPort, buffer);
	    }
	    
	    memset (gossip, 0, sizeof gossip);
	    memset (peerName, 0, sizeof peerName);
	    memset (peerIP, 0, sizeof peerIP);
	    memset (peerPort, 0, sizeof peerPort);
	    //gossip[0] = peerName[0] = peerIP[0] = peerPort[0] = 0;
	
	} while (command != OPT_EXIT);

    close(tcpfd);

    return 0;
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
    
    if (flags == 0) {
        fprintf (stderr, "Please specify the protocol to use (-T for TCP, -U for UDP)\n");
        exit(1);
    }

    return flags;  
}

/*void MessageDecode (int size, int tag){

	asn1_node definitions = NULL;
	asn1_node node = NULL;
	
	char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

	char time[MAXLINE];
	char name[MAXLINE];
	char message[MAXLINE];
	
	int result = 0;
	
	char sb[MAXLINE];
	char *dataBuff = new char[MAXLINE];
	
	int codeNum=0;

	result = asn1_parser2tree ("ApplicationList_asn1_tab.c", &definitions, errorDescription);

	switch(tag){
	
	case 1:
		result = asn1_create_element(definitions, "ApplicationList.Peer", &node);
		result = asn1_der_decoding (&node, dataBuff, size, errorDescription);

		if(result != ASN1_SUCCESS) {
			asn1_perror (result);
			printf("Decoding error = \"%s\"\n", errorDescription);
			break;
		}

		char code[MAXLINE];
		result = asn1_read_value(node, "code", code, &size);
		codeNum = atoi(code);
		printf("Server response recieved..\n");
		printf("Code = %d [%s]\n", codeNum, (codeNum==100)?"Peer":"error");
		break;
		
	case 2:	
		result = asn1_create_element(definitions, "ApplicationList.PeersAnswer", &node);
		result = asn1_der_decoding (&node, dataBuff, size, errorDescription);

		if(result != ASN1_SUCCESS) {
			asn1_perror (result);
			printf("Decoding error = \"%s\"\n", errorDescription);
			break;
		}

		printf("Server response recieved..\n");
		char recName[MAXLINE];
		
		for(int j=1 ;; j++){

			snprintf(recName, sizeof(recName), "PeersAnswer.time?%d", j);
			result = asn1_read_value(node, recName, time, &size);
			if (result==ASN1_ELEMENT_NOT_FOUND){
				printf(" %d Peers received.\n", j-1);
				break;
			}
			printf("\tTime [%s] =>\"%s\"\n", recName, time);

			snprintf(recName, sizeof(recName), "PeersAnswer.name?%d", j);
			result = asn1_read_value(node, recName, name, &size);
			printf("\tName [%s] =>\"%s\"\n", recName, name);

			snprintf(recName, sizeof(recName), "PeersAnswer.message?%d", j);
			result = asn1_read_value(node, recName, message, &size);
			printf("\tMessage [%s] =>\"%s\"\n", recName, message);

			char wbuf[MAXLINE];
			result = asn1_read_value(node, recName, wbuf, &size);
			if (result==ASN1_ELEMENT_NOT_FOUND){
				printf(" %d Events received.\n", j+1);
				break;
			}

			if (result!=ASN1_SUCCESS) {
				break;
			}
		}
		break;
		
	case 3:
		vector<string> appList = parseCommand(dataBuff);
		int k = (appList.size()-2)/4;
		printf("Server response recieved..\n");
		printf("Decoding %d peers..\n", k-1);

		for(int j=0; j<k; j++){
			strcpy(time, appList.at((j*4)+2+0).c_str());
			strcpy(message, appList.at((j*4)+2+1).c_str());
			strcpy(name, appList.at((j*4)+2+2).c_str());

			printf("Peer #%d ------------------------------\n", j+1);
			printf("\tName=\"%s\"\n", name);
			printf("\tTime=\"%s\"\n", time);
			printf("\tMessage=\"%s\"\n", message);
		}
		break;
	}
	asn1_delete_structure (&node);
	asn1_delete_structure (&definitions);
}
*/

int main (int argc, char *argv[]) {

    int flag = getCommandLineArgs (argc, argv);
    
    if (flag == OPT_UDP)
        udp_client ();
    else
        tcp_client ();
}

