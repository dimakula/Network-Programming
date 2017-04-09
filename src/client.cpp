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

string peerName, peerIP, peerPort;

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

void promptForPeer () {

    printf ("Enter peer name: ");
    getline (cin, peerName);
    printf ("\nEnter peer ip: ");
    getline (cin, peerIP);
    printf ("\nEnter peer port: ");
    getline (cin, peerPort);
    printf ("\n");
}

int udp_client () {

    int  udpfd, num, received;
    struct addrinfo hints, *servinfo, *p;
    char  buffer [MAXDATASIZE];  
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
    string data = fullGossipMessage (gossip, timestamp); // data to send
	
	do {
		int totalSent = 0; 
	    int bytes;	        
	     
	    printf ("sending...\n");
	    
	    while (totalSent < data.length()) {
	      
	        if ((bytes = sendto (udpfd, data.c_str(), data.length(), 0, 
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
            getline (cin, gossip);
            data = fullGossipMessage (gossip, timestamp);
	    
	    } else if (command == OPT_PEER) {
	        promptForPeer ();
	        data = fullPeerMessage (peerName, peerIP, peerPort);    
	    }
	
	} while (command != OPT_EXIT);
	
	
	freeaddrinfo (servinfo);
	close (udpfd);
	return 0;
}

//Use the same sockaddr for tcp as well.
int tcp_client () {

   char *buffer = new char [MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rc;
    int  tcpfd;

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
    int command;
    string data = fullGossipMessage (gossip, timestamp); // data to send
	
	do {
		int totalSent = 0; 
	    int bytes;	        
	     
	    printf ("sending...\n");
	    
	    while (totalSent < data.length()) {
	      
	        if ((bytes = send (tcpfd, data.c_str(), data.length(), 0)) == -1) {
	            
	            if (errno == EINTR) continue;
	            perror ("TCP");
	            exit (1);
	        }
      
	        totalSent += bytes;
	    }
	    
	    printf ("finished sending\n\n");
	    command = printUserPrompt();
	    
	    if (command == OPT_GOSSIP) {
	        printf ("Please enter new gossip message: ");
            getline (cin, gossip);
            data = fullGossipMessage (gossip, timestamp);
	    
	    } else if (command == OPT_PEER) {
	        promptForPeer ();
	        data = fullPeerMessage (peerName, peerIP, peerPort);    
	    }
	
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

/*int MessageEncode(char *message, char *thisString) {
	
	vector<string> appList;

	thisString = strtok ((char *)cmd.c_str(),":");
	while (thisString != NULL){
		appList.push_back(thisString);
		thisString = strtok (NULL, ":");
	}
	return appList;	
	
	asn1_node definitions = NULL;
	node = NULL;
	
	char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];
	char time[MAXDATASIZE];
	char message[MAXDATASIZE];
	char name[MAXDATASIZE];
	char ip[MAXDATASIZE];
	//What else we need to show?
	
	int result = 0;
	int size;
	
	//Three Applications required, which means 3 cases.
	
	int tag = -1;
	vector<string> appList = parseCommand(message);
	if (appList.at(0).compare("APPLICATION_DEFINITION") == 0) {
		tag = 1;
	}
	if (appList.at(0).compare("SECOND_APPLICATION") == 0) {
		tag = 2;
	}
	else if (appList.at(0).compare("LAST_APPLICATION") == 0) {
		tag = 3;
	}
	
	result = asn1_parser2tree ("ApplicationList.asn", &definitions, errorDescription);
	
	switch(tag) {
		case 1:
		//Gossip ::= [APPLICATION 1] EXPLICIT SEQUENCE {
		//sha256hash OCTET STRING,
		//timestamp GeneralizedTime,
		//message UTF8String
		//}
		result = asn1_create_element(definitions, "ApplicationList.Request1", &node);
		
		strcpy(name, appList.at(1).c_str() );
		strcpy(time, appList.at(2).c_str() );
		strcpy(message, appList.at(3).c_str() );
		

		result = asn1_write_value(node, "name", name, 1);
		result = asn1_write_value(node, "time", time, strlen());
		result = asn1_write_value(node, "message", message, strlen(message));
		
		size = MAXDATASIZE;
		
		result = asn1_der_coding (node, "", dataBuff, &size, errorDescription);
		if(result != ASN1_SUCCESS) {
			asn1_perror (result);
			printf("Encoding error = \"%s\"\n", errorDescription);
		}
		break;
		
		case 2:
		//Peer ::= [APPLICATION 2] IMPLICIT SEQUENCE {
		//name UTF8String, 
		//port INTEGER, 
		//ip PrintableString
		//}
		result = asn1_create_element(definitions, "ApplicationList.Request2", &node);
        strcpy(name, appList.at(1).c_str());
		strcpy(port, appList.at(2).c_str());
		strcpy(ip, appList.at(3).c_str())

		result = asn1_write_value(node, "name", name, 1);
		result = asn1_write_value(node, "port", port, strlen());
		result = asn1_write_value(node, "ip", ip, strlen(ip));
		
		size = MAXDATASIZE;
		
		result = asn1_der_coding (node, "", dataBuff, &size, errorDescription);
		if(result != ASN1_SUCCESS) {
			asn1_perror (result);
			printf("Encoding error = \"%s\"\n", errorDescription);
			return -1;
		}
		break;
		
		case 3:
		//PeersQuery ::= [APPLICATION 3] IMPLICIT NULL
		result = asn1_create_element(definitions, "ApplicationList.Request3", &node);
		strcpy(NULL, appList.at(1).c_str());
		
		result = asn1_write_value(node, "NULL", NULL, 1);
		
		size = MAXDATASIZE;
		
		result = asn1_der_coding (node, "", dataBuff, &size, errorDescription);
		if(result != ASN1_SUCCESS) {
			asn1_perror (result);
			printf("Encoding error = \"%s\"\n", errorDescription);
			return -1;
		}break;
	}
	
	asn1_delete_structure (&node);
	asn1_delete_structure (&definitions);
	
	return size;
}*/

/*
string MessageDecode (int size){

	asn1_node definitions = NULL;
	node = NULL;
	
	char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

	char time[MAXDATASIZE];
	char name[MAXDATASIZE];
	char message[MAXDATASIZE];
	int result = 0;
	char sb[MAXDATASIZE];

	result = asn1_array2tree (EventProtocol_asn1_tab, &definitions, errorDescription);

	unsigned char userNum[100];
	int x;
	unsigned long tag;
	result = asn1_get_tag_der((const unsigned char *)dataBuff, size, userNum, &x, &tag);
	if(result != ASN1_SUCCESS) {
		asn1_perror (result);
		printf("TAG error = \"%s\"\n", errorDescription);
	}

	switch(tag){
	case 1: 
		result = asn1_create_element(definitions, "ApplicationList.Request1", &node );
		result = asn1_der_decoding (&node, dataBuff, size, errorDescription);

		if(result != ASN1_SUCCESS) {
			asn1_perror (result);
			printf("Decoding error = \"%s\"\n", errorDescription);
			break;
		}

		printf("{APPLICATION} recieved..\n");
		size = MAXDATASIZE;
		result = asn1_read_value(node, "name", name, &size);
		printf("\tName=\"%s\"\n", name);

		size = MAXDATASIZE;
		result = asn1_read_value(node, "time", time, &size);
		printf("\tTime=\"%s\"\n", time);

		size = MAXDATASIZE;
		result = asn1_read_value(node, "description", message, &size);
		printf("\tMessage=\"%s\"\n", message);

		sprintf(sb, "APPLICATION;%s;%s;%s", time, message, name);

		break;
	case 2:	
		result = asn1_create_element(definitions, "ApplicationList.Request2", &node );
		result = asn1_der_decoding (&node, dataBuff, size, errorDescription);

		if(result != ASN1_SUCCESS)  {
			asn1_perror (result);
			printf("Decoding error = \"%s\"\n", errorDescription);
			break;
		}

		printf("{APPLICATION} recieved..\n");
		size = MAXDATASIZE;
		result = asn1_read_value(node, "afterTime", time, &size);
		printf("\tTime=\"%s\"\n", time);

		size = MAXDATASIZE;
		result = asn1_read_value(node, "name", name, &size);
		printf("\tName=\"%s\"\n", name);

		sprintf(sb, "APPLICATION 2;%s;%s", name, time);

		break;
	case 3: 
			result = asn1_create_element(definitions, "ApplicationList.Request3", &node );
		result = asn1_der_decoding (&node, dataBuff, size, errorDescription);

		if(result != ASN1_SUCCESS)  {
			asn1_perror (result);
			printf("Decoding error = \"%s\"\n", errorDescription);
			break;
		}

		printf("{APPLICATION} recieved..\n");

		size = MAXDATASIZE;
		result = asn1_read_value(node, "name", name, &size);
		printf("\tName=\"%s\"\n", name);
		sprintf(sb, "APPLICATION;%s", name);

		break;

	}
	asn1_delete_structure (&node);
	asn1_delete_structure (&definitions);

	string stb = sb;
	return stb;
}
*/

int main (int argc, char *argv[]) {
    
    init_parseTree(); // initialise parse tree for asn1 encoding
    int flag = getCommandLineArgs (argc, argv);
    
    if (flag == OPT_UDP)
        udp_client ();
    else
        tcp_client ();
}

