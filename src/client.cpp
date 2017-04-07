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

int MessageEncode(char *message, char *thisString) {
	
	vector<string> appList;

	thisString = strtok ((char *)cmd.c_str(),";");
	while (thisString != NULL){
		appList.push_back(thisString);
		thisString = strtok (NULL, ";");
	}
	return appList;	
	
	an1_node definitions = NULL;
	node = NULL;
	
	char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];
	char time[MAXDATASIZE];
	char description[MAXDATASIZE];
	char name[MAXDATASIZE];
	char ip[MAXDATASIZE];
	//What else we need to show?
	
	int result = 0;
	
	//Three Applications required, which means 3 cases.
	
	int tag = -1;
	vector<string> appList = parseCommand(message);
	if (appList.at(0).compare("APPLICATION_DEFINITION") == 0) {
		tag = 1;
	}
	if (applist.at(0).compare("SECOND_APPLICATION") == 0) {
		tag = 2;
	}
	else if (applist.at(0).compare("LAST_APPLICATION") == 0) {
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
		case 2:
		//Peer ::= [APPLICATION 2] IMPLICIT SEQUENCE {
		//name UTF8String, 
		//port INTEGER, 
		//ip PrintableString
		//}
		case 3:
		//PeersQuery ::= [APPLICATION 3] IMPLICIT NULL
	}
	
	asn1_delete_structure (&node);
	asn1_delete_structure (&definitions);
}

/*


	switch(tag){
	case 1:
		//	Event ::= [1] SEQUENCE {
		//		time GeneralizedTime,
		//		group UTF8String,
		//		description UTF8String
		//	}
		// "EVENT_DEFINITION;2015-03-12:18h30m00s001Z;Meeting with the user.;CSE5232"
		result = asn1_create_element(definitions, "EventProtocol.Event", &node );

		strcpy( tmp_time ,  list.at( 1 ).c_str() );
		strcpy( tmp_description ,  list.at( 2 ).c_str() );
		strcpy( tmp_group ,  list.at( 3 ).c_str() );

		result = asn1_write_value(node, "time", tmp_time, 1);
		result = asn1_write_value(node, "group", tmp_group, strlen(tmp_group));
		result = asn1_write_value(node, "description", tmp_description, strlen(tmp_description));

		len = DATA_SIZE;
		result = asn1_der_coding (node, "", dataBuff, &len, errorDescription);
		if(result != ASN1_SUCCESS) {
			asn1_perror (result);
			printf("Encoding error = \"%s\"\n", errorDescription);
			//return -1;
		}
		break;
	case 2:
		//	Request ::= [2] SEQUENCE {
		//		group UTF8String,
		//		afterTime GeneralizedTime
		//	}

		// "GET_NEXT_EVENTS;CSE5232;2015-03-12:18h30m00s000Z"
		result = asn1_create_element(definitions, "EventProtocol.Request", &node );

		strcpy( tmp_group ,  list.at( 1 ).c_str() );
		strcpy( tmp_time ,  list.at( 2 ).c_str() );

		result = asn1_write_value(node, "group", tmp_group, strlen(tmp_group));
		result = asn1_write_value(node, "afterTime", tmp_time, 1);

		len = DATA_SIZE;
		result = asn1_der_coding (node, "", dataBuff, &len, errorDescription);
		if(result != ASN1_SUCCESS) {
			asn1_perror (result);
			printf("Encoding error = \"%s\"\n", errorDescription);
			return -1;
		}
		break;
	}

	asn1_delete_structure (&node);
	asn1_delete_structure (&definitions);

	return len;
}
*/

int main (int argc, char *argv[]) {
    
    int flag = getCommandLineArgs (argc, argv);
    
    if (flag == OPT_UDP)
        udp_client ();
    else
        tcp_client ();
}

