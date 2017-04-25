/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2017 
                Authors:  Dmitry Kulakov, Xiaohan Yang, Nicholas Zynko
                Florida Tech, Computer Science
   
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

#include "../include/server.h"
using namespace std;

#define MAX_CALLS 5 // the maximum number of queued connections allowed
#define MAXLINE 2048 // the maximum size of the input buffer
#define MAX_THREADS 5 // Maximum number of threads that can be operating

// get path separator for the specific os
const char PathSeparator =
    #ifdef _WIN32
        '\\';
    #else
        '/';
    #endif

// tcp, udp and connection file descriptors
int tcpfd, udpfd, confd;

// sqlite database
sqlite3 *db;

// Holds the latest gossip message.
string latestGossip;

// Prevents threads from writing to the database at the same time
mutex database_mutex;

asn1_node definitions = NULL;
asn1_node structure = NULL;
char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

const asn1_static_node ApplicationList_asn1_tab[] = {
  { "ApplicationList", 536875024, NULL },
  { NULL, 1073741836, NULL },
  { "Gossip", 1610620933, NULL },
  { NULL, 1073744904, "1"},
  { "sha256hash", 1073741831, NULL },
  { "timestamp", 1073741861, NULL },
  { "message", 34, NULL },
  { "Peer", 1610620933, NULL },
  { NULL, 1073746952, "2"},
  { "name", 1073741858, NULL },
  { "port", 1073741827, NULL },
  { "ip", 31, NULL },
  { "PeersQuery", 1610620948, NULL },
  { NULL, 5128, "3"},
  { "PeersAnswer", 1610620939, NULL },
  { NULL, 1073743880, "1"},
  { NULL, 2, "Peer"},
  { "UTF8String", 1610620935, NULL },
  { NULL, 4360, "12"},
  { "PrintableString", 536879111, NULL },
  { NULL, 4360, "19"},
  { NULL, 0, NULL }
};


// called upon hearing the control-C or control-Z signal
void signal_stop (int a) {
    fprintf (stderr, "Cancelling server, closing all file handlers\n");
    close (tcpfd);
    close (udpfd);
    close (confd);
    sqlite3_close (db);
    fflush (stdout);
    exit (1);
}


// counts the number of peers known
static int peers;

// Called for each row of the select statement, which appends the peers
// in the format specified
static int
callback (void *result, int argc, char **argv, char **azColName) {
  
    peers++; // increment the number of peers by one
    int i = 0;
    strncat ((char *)result, argv[i], strlen (argv[i]));
    strncat ((char *)result, ":", 1);
    strncat ((char *)result, "PORT=", 5);
    strncat ((char *)result, argv[i+1], strlen (argv[i+1]));
    strncat ((char *)result, ":", 1);
    strncat ((char *)result, "IP=", 3);
    strncat ((char *)result, argv[i+2], strlen (argv[i+2]));
    strncat ((char *)result, "|", 1);   
    return 0;
}


static int 
peersCallback (void *result, int argc, char **argv, char **azColName) {

    asn1_node node = NULL;
    int rc = asn1_create_element(definitions, "ApplicationList.Peer", &node);

    if ((rc = asn1_write_value(node, "name", argv[0], 0))
	        != ASN1_SUCCESS) {
	    asn1_perror (rc);
	}
	
	if ((rc = asn1_write_value(node, "port", argv[1], 0))
	        != ASN1_SUCCESS) {
	    asn1_perror (rc);
	}
	
    if ((rc = asn1_write_value(node, "ip", argv[2], 0))
	        != ASN1_SUCCESS) {
	    asn1_perror (rc);
	}
	
	char *buffer = new char[MAXLINE];
	int size = MAXLINE;
		
	rc = asn1_der_coding (node, "", buffer, &size, errorDescription);
    asn1_delete_structure (&node);
	
    if ((rc = asn1_write_value((asn1_node)result, "Peer.?LAST", "NEW", 1))
	        != ASN1_SUCCESS) {
	    asn1_perror (rc);
	}
	
	if ((rc = asn1_write_value((asn1_node)result, "Peer.?LAST", buffer, 1))
	        != ASN1_SUCCESS) {
	    asn1_perror (rc);
	}
	
	delete [] buffer;
}

static int gossipCallback(void *message, int argc, char **argv, 
    char **azColName)
{
	char *tempPort;
	char *tempIP;
	int i = 0;
    tempIP = argv[i];
	tempPort = argv[i+1];

	broadcast ((char *) message, tempIP, tempPort);
	return 0;
}

int broadcast (char *message, char *address, char *port)
{
    int  sockfd, num, recieved;
    struct addrinfo hints, *servinfo, *p;
    char  buf[MAXLINE];  
	struct sockaddr_in peer;
	int totalSent = 0; 
	int bytes; 
    
    bzero (&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    printf ("Sending message to %s on port %s\n", address, port);
    
    if ((recieved = getaddrinfo (address, port, &hints, &servinfo)) != 0) {
        fprintf (stderr, "Broadcast: Address does not exist\n");
        return -1;
    }
    
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket (p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror ("Broadcast");
            continue;
        }
        
        break;
    }

	if (p == NULL) {
	    fprintf (stderr, "Broadcast: Failed to create socket\n");
	    return -1;
	}
	
	while (totalSent < strlen (message)) {
	  
	    if ((bytes = sendto (sockfd, message, strlen(message), 0, p->ai_addr,
	        p->ai_addrlen)) == -1) {
	        
	        if (errno == EINTR) continue;
	        return (totalSent == 0) ? -1 : totalSent;
	    }
  
	    totalSent += bytes;
	}
	
	printf ("Sent\n");
	freeaddrinfo (servinfo);
	close (sockfd);
	return 0;
}


void broadcastGossip(char *message) {

	string sql;
	int rc;
	sql = "SELECT IP, PORT FROM PEERS";

	// Storage for error code.
	char* zErrMsg = new char();
	(*zErrMsg) = 0;

	rc = sqlite3_exec(db, sql.c_str(), gossipCallback, message, &zErrMsg);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free (zErrMsg);
	}

	return;
}


int MessageDecode (char *buffer, int &offset) {

	char *sha256, *time, *message;
	char *name, *port, *ip;
	string sql, values;
	
	char* zErrMsg = 0; // Buffer for error handling.
	bool broadcast, sendPeers;
	broadcast = sendPeers = false;

	int result = 0;
	int length = offset;

    asn1_node node = NULL;

    // generate asn1 definitions
    char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];
    if ((result = asn1_array2tree (ApplicationList_asn1_tab, &definitions, errorDescription))
            != ASN1_SUCCESS) {
            asn1_perror(result);
    }
	
	unsigned char userNum[100];
	int x;
	unsigned long tag;
	result = asn1_get_tag_der ((const unsigned char *)buffer, length, userNum, &x, &tag);
	
	if(result != ASN1_SUCCESS) {
		asn1_perror (result);
		printf("TAG error = \"%s\"\n", errorDescription);
		return -2;
	}
    
	switch(tag) {
	    case 1:
		    result = asn1_create_element(definitions, "ApplicationList.Gossip", &node );
		    result = asn1_der_decoding (&node, buffer, length, errorDescription);

		    if(result != ASN1_SUCCESS) {
			    asn1_perror (result);
			    printf("Decoding error = \"%s\"\n", errorDescription);
			    break;
		    }

            sha256 = new char[MAXLINE];
            time = new char[MAXLINE];
            message = new char[MAXLINE];

		    printf("{APPLICATION} recieved..\n");
		    result = asn1_read_value(node, "sha256hash", sha256, &offset);
		    printf("\t\Hash=\"%s\"\n", sha256);
		    
		    
		    result = asn1_read_value(node, "timestamp", time, &offset);
		    printf("\tTime=\"%s\"\n", time);

		    result = asn1_read_value(node, "message", message, &offset);
		    printf("\tMessage=\"%s\"\n", message);

            values = "\'" + string(sha256) + "\', \'" + string(message) +
                        "\', \'" + string(time) + "\'";
                      
            sql = "INSERT INTO GOSSIP (DIGEST, MESSAGE, GENERATED)" \
                "VALUES (" + values + ");";
           
            database_mutex.lock();
            result = sqlite3_exec(db, sql.c_str(), peersCallback, 0, &zErrMsg);
            database_mutex.unlock();
            
            broadcast = true;
            delete [] sha256, time, message;
		    break;
		    
	    case 2:	
		    result = asn1_create_element(definitions, "ApplicationList.Peer", &node );
		    result = asn1_der_decoding (&node, buffer, length, errorDescription);

		    if(result != ASN1_SUCCESS)  {
			    asn1_perror (result);
			    printf("Decoding error = \"%s\"\n", errorDescription);
			    break;
		    }

            name = new char[MAXLINE];
            port = new char[MAXLINE];
            ip   = new char[MAXLINE];
            
		    printf("{APPLICATION} recieved..\n");
		    result = asn1_read_value(node, "name", name, &offset);
		    printf("\tName =\"%s\"\n", name);

		    result = asn1_read_value(node, "port", port, &offset);
		    printf("\tPort =\"%s\"\n", port);
		    
		    result = asn1_read_value(node, "ip", ip, &offset);
		    printf("\tIP =\"%s\"\n", ip);
		    		            
            values = "\'" + string(name) + "\', " + string(port) + ", \'" + string(ip) + "\'";
        
            // inserts or replaces peer with new values
            sql = "INSERT OR REPLACE INTO PEERS (PEER, PORT, IP)" \
              "VALUES (" + values + ");";

            database_mutex.lock();
            result = sqlite3_exec(db, sql.c_str(), peersCallback, 0, &zErrMsg);
            database_mutex.unlock();
            
            delete [] name, port, ip;
		    break;
		    
	    case 3: 
	        int size = MAXLINE;
			result = asn1_create_element(definitions, "ApplicationList.PeersAnswer", &node);
		    result = asn1_der_coding (node, "", buffer, &size, errorDescription); 

		    if(result != ASN1_SUCCESS)  {
			    asn1_perror (result);
			    printf("Decoding error = \"%s\"\n", errorDescription);
			    break;
		    }

            // pass in the node to the callback function to build the PeersAnswer
            result = sqlite3_exec(db, sql.c_str(), peersCallback, &node, &zErrMsg);
            sendPeers = true;
		    printf("{APPLICATION} recieved..\n");

		    result = asn1_read_value(node, "name", name, &offset);
		    printf("\tName=\"%s\"\n", name);

		    break;

	}
	
    if (result == SQLITE_CONSTRAINT) {
        fprintf(stderr, "DISCARDED\n");
     
    } else if (result != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);      
    
    } else if (broadcast) {
    	
    	broadcastGossip (buffer);
    }
    
    else if (sendPeers) {

    }

    //memset (buffer, 0, MAXLINE); //FIX THIS EVENTUALLY OTHERWISE YOU'LL ALWAYS BE CLEARING YOUR BUFFER
	//buffer[0] = '/0';
	offset = 0;
	asn1_delete_structure (&node);
	asn1_delete_structure (&definitions);
    
	return 0;
}


// Parses the user commands and stores them in an sqlite3
// database, returns an output string to send to the client
char *reader (char *buffer, int &offset) {

    int rc; // return character
    string name, port, ip; // Values for Peer message
    string digest, message, date; // Values for gossip message
    string sql; // Sql statement
    string values; // Values for sql query
    string split; // contains text delimited by either ':'
    bool broadcast = false; // check if message needs to be transmitted
    bool sendPeers = false; // check if the client asked for PEERS?
    peers = 0; // number of peers currently known
    char* zErrMsg = 0; // Buffer for error handling.
    bool ok = false;
    size_t endOfMessage; // position in buffer for the end of valid input
    string fulltext (buffer); // Used to find '%' or '\n' in the buffer
    
    // Check if a newline or % character is present, if not return null to get
    // more input
    endOfMessage = fulltext.find ("%");
    if (endOfMessage > offset) endOfMessage = fulltext.find("\n");
    if (endOfMessage > offset) return NULL;
    
    // Copy over valid message to parse using istringstream
    char temp [endOfMessage+1];
    memcpy (temp, &buffer[0], endOfMessage);
    string shortenedText (temp);
    istringstream stream (shortenedText);

    char* result = new char[MAXLINE]; // Result of any select statements
    char* output = new char[MAXLINE]; // output returned to respective handler
    
    // who knows what kind of junk is in the heap
    result[0] = '\0';
    output[0] = '\0';
   
   
    // Parsing message
    getline (stream, split, ':');  
    
    if (split == "GOSSIP") {
        getline (stream, digest, ':');
        getline (stream, date, ':');
        getline (stream, message, ':');
        
        values = "\'" + digest + "\', \'" + message +
            "\', \'" + date + "\'";
 
        sql = "INSERT INTO GOSSIP (DIGEST, MESSAGE, GENERATED)" \
              "VALUES (" + values + ");";
        
        // need to broadcast message at the end once sql is executed
        broadcast = true;
        ok = true;
    
    } else if (split == "PEER") {
        getline (stream, name, ':');
        
        while (getline (stream, split, '=') ) {
   
            if (split == "PORT") {
                getline (stream, port, ':');
            
            } else if ( split == "IP") {
                getline (stream, ip, ':');
            }
        }
        
        values = "\'" + name + "\', " + port + ", \'" + ip + "\'";
        
        // inserts or replaces peer with new values
        sql = "INSERT OR REPLACE INTO PEERS (PEER, PORT, IP)" \
              "VALUES (" + values + ");";
              
        ok = true;
    
    } else if (split == "PEERS?") {
        sql = "SELECT * from PEERS;";
        sendPeers = true;
    
    // No Valid message has been found, presumably fragmented
    } else {
        return NULL;
    }
    
    // locks sql query in case thread is trying to insert or update data
    database_mutex.lock();
    rc = sqlite3_exec(db, sql.c_str(), callback, result, &zErrMsg);
    database_mutex.unlock();
    
    // Constraint due to primary key or not null
    if (rc == SQLITE_CONSTRAINT) {
        fprintf(stderr, "DISCARDED");
        strncat (output, "DISCARDED", 9);
     
    } else if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        strncat (output, "ERROR", 5);
        sqlite3_free(zErrMsg);
       
    } else if (broadcast) {
    	broadcastGossip((char *)message.c_str());
    }
    
    else if (sendPeers) {
        // Form output string to send to client
        string peerNum = to_string (peers);
        strncat ((char *)output, "|PEERS:", 7);

        strncat (output, peerNum.c_str(), strlen (peerNum.c_str()));
        strncat (output, "|", 1);
    }

    if (strlen (result) != 0) {
        strncat (output, result, strlen (result));
    
    // send ok 
    } else if (ok) {
        strncat (output, "OK", 2);
    }
    
    strncat (output, "\n", 1);
    delete[] result; //free up heap
    
        
    // Take out the message from the buffer to free up space
    offset = offset - (endOfMessage + 1);
    printf ("offset: %d\n", offset);
    printf ("endOfMessage: %d\n", endOfMessage + 1);
    memcpy (buffer, &buffer[endOfMessage+1], (size_t) offset);
    buffer[offset] = '\0';  
    
    // return output for the handler to send
    return (char *)output;
}


//handler for the tcp socket
void* tcp_handler (void *threadArgs) {

    int offset = 0; // number of bytes read from the client
    int totalWrite = 0; // total bytes written to client
    char *result;
    char *input;
    int bytes, rc;

    
    // Guarantees that thread resources are deallocated upon return, no need to
    // rejoin the main thread
    pthread_detach(pthread_self()); 
    
    // get all the thread argument
    int confd =     ((struct tcp_args *) threadArgs) -> confd;
    int flags =     ((struct tcp_args *) threadArgs) -> flags;
    char *buffer =  ((struct tcp_args *) threadArgs) -> buffer;
    //free (threadArgs);
    
    struct timeval timeout;
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;
    
    if (setsockopt (confd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
            perror("setsockopt failed\n");
    
    // keep receiving until client closes connection
    while (1) {
    
        if ((bytes = recv (confd, buffer + offset, MAXLINE - offset, 
                flags)) != -1)
            offset += bytes;
        
        if (bytes == -1) {
            printf ("Timeout occurred\n");
            break;
        }
        
        // if zero is returned, the client closed connection
        if (bytes == 0) {
            printf ("Client has closed the connection\n");
            break;
        }

        printf ("Bytes: %d\n", bytes);
        rc = MessageDecode (buffer, offset);
        memset (buffer, 0, MAXLINE);
        
        
        /*   
        // While reader parses valid commands and returns non-null results
        while ((result = reader(buffer, offset)) != NULL) {
             printf ("TCP: %s\n", result);
             
             while (totalWrite < strlen (result)) {
                if ((bytes = send (confd, result, strlen (result), 0)) == -1) {
                    if (errno == EINTR) continue;
                    perror ("send:");
                }
                    
                totalWrite += bytes;
            }
            
            // Reset the total bytes sent to the client
            totalWrite = 0;
        } 
        */
    }
    
    close (confd);
    //delete [] result;
}


// handler for the udp socket
void* udp_handler (void *threadArgs) {
	int rc;
    int bytes;
	int offset = 0; // number of bytes read from the client
	char *input;
	char *result;
	int totalSent;
	
	// Guarantees that thread resources are deallocated upon return, no need to
    // rejoin the main thread
    pthread_detach(pthread_self()); 
    
    // get all the thread argument
    int sockfd =            ((struct udp_args *) threadArgs) -> udpfd;
    sockaddr* client =      ((struct udp_args *) threadArgs) -> client; 
    socklen_t client_len =  ((struct udp_args *) threadArgs) -> client_length;
    int flags =             ((struct udp_args *) threadArgs) -> flags;
    char *buffer =          ((struct udp_args *) threadArgs) -> buffer;
    //free (threadArgs);
	
	// keep receiving until client closes connection
	while (1) {

	    // Receive until read all the message
	    if ((bytes = recvfrom(sockfd, buffer + offset, MAXLINE - offset,
	            flags, client, &client_len)) != -1)
	        offset += bytes;
	        
	    else perror ("Recieve");

        if (bytes == -1) {
            printf ("Timeout occurred\n");
            break;
        }
        
        // if zero is returned, the client closed connection
        if (bytes == 0) {
            printf ("Client has closed the connection\n");
            break;
        }
		    
		rc = MessageDecode (buffer, offset);
		memset (buffer, 0, MAXLINE);
		    
		/*while ((result = reader(buffer, offset)) != NULL) {
		    printf ("UDP: %s\n", result);
		    
		    while (totalSent < strlen (result)) {
                
                if((bytes = sendto(sockfd, result, strlen(result), 0, client,       
                        client_len)) == -1) {
                    if (errno == EINTR) continue;
                    perror ("sendto:");
                }
			    
			    totalSent += bytes;
	        }
	        
	        // Reset the total bytes send to the client
	        totalSent = 0;
		}*/
	}
	
	close (sockfd);
	printf ("Client has closed the connection\n");
	//delete []result;
}

// run when child is created
void sig_child (int signo) {
    pid_t pid;
    int stat;
    
    // waitpid () might overwrite errno, so save it
    int saved_errno = errno;

    while ((pid = waitpid (-1, &stat, WNOHANG)) > 0)
        printf ("child %d terminated]n", pid);
    
    errno = saved_errno;
}

// sets up sqlite database for storing gossip
void setup_database (char *filePath) {

    int rc = sqlite3_open(filePath, &db);
    
    if (rc) {
        fprintf (stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    
    char *sql = "CREATE TABLE IF NOT EXISTS PEERS(" \
                "PEER TEXT PRIMARY  KEY   NOT NULL,"  \
                "PORT               INT   NOT NULL,"  \
                "IP                 TEXT  NOT NULL);" \
                
                "CREATE TABLE IF NOT EXISTS GOSSIP(" \
                "DIGEST TEXT PRIMARY KEY NOT NULL," \
                "MESSAGE             TEXT," \
                "GENERATED           TEXT NOT NULL);";


    char *zErrMsg = 0;

    rc = sqlite3_exec (db, sql, callback, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf (stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free (zErrMsg);
    }  
}

int main (int argc, char *argv[]) {
    // execute signal_stop method upon receiving control-c or control-z commands
    signal (SIGINT, signal_stop);
    signal (SIGTSTP, signal_stop);
    
    char c;
    char* filePath;
    unsigned short int port;

    while ((c=getopt(argc, argv, "d:p:"))!=-1) {
        switch (c) {
            case 'd' : filePath = optarg; break;
            case 'p' : port = atoi (optarg); break;
            default: 
                printf ("Usage: %s [-d <file path>] [-p <port>]\n",
                    argv[0]);
                exit(1);
        }
    }
    
    printf ("\nfilepath: %s port: %d\n", filePath, port);
    
    // database for storing peers and messages 
    setup_database (filePath);
      
    int nready; // return result for poll
    int rc; // Return code
    const int nfds = 2; // number of file descriptors
    const int on = 1; // optional value for Setsockopt and ioctl
    pid_t child;
    struct sockaddr_in client, server;
    struct pollfd ufds[nfds]; // used to select between the udp and tcp

    socklen_t addr_len = sizeof (server);
    socklen_t client_len = sizeof (client);

    int tcpPort, udpPort;
    int childpid;
    void sig_child (int);

    int numThreads = 0; // current number of threads active
    pthread_t threadID; // current threadID, not used for anything
 
    // get tcp socket descriptor
    if ((tcpfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("TCP socket");
        exit (-1);
    }
    
    // SO_REUSEADDR allows multiple sockets to listen in on the port
    // also gets rid of "address in use" error
    if ((rc = setsockopt (tcpfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on))) < 0) {
        perror ("setsockopt");
        close (tcpfd);
        exit (-1);
    }

    // set socket to be nonblocking
    if ((rc = ioctl(tcpfd, FIONBIO, (char *)&on)) < 0) {
        perror ("ioctl");
        close (tcpfd);
        exit (-1);
    }
        
    // zeroes out the sin_zero field
    bzero(&server, addr_len);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons (port);
    
    // checking everything converted properly
    tcpPort = ntohs(server.sin_port);
    printf("Binding tcp connection to port %d\n", tcpPort);

    // bind server to specified port
    if ((rc = bind (tcpfd, (struct sockaddr*) &server, addr_len)) < 0) {
        perror ("TCP binding");
        close (tcpfd);
        exit(-1);  
    }

    // listen for incoming connections
    if ((rc = listen(tcpfd, MAX_CALLS)) < 0) {
        perror ("listen");
        close (tcpfd);
        exit (-1);
    }

    //Build UDP socket 
    if((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  
    {  
        perror("UDP socket");
        close (udpfd); 
        exit(-1);  
    }  
          
    //Config network
    bzero(&server, sizeof(server));    // Clear.
    server.sin_family = AF_INET;       // IPv4.  
    server.sin_port   = htons(port);   // port.  
    server.sin_addr.s_addr = htonl(INADDR_ANY); // ip.  
    
    // checking that the port converted properly
    udpPort = ntohs(server.sin_port);
    printf("Binding udp connection to port %d\n", udpPort);
          
    //Bind socket.         
    if((rc = bind(udpfd, (struct sockaddr*) &server, sizeof(server))) < 0) {  
        perror("bind");  
        close(udpfd);        
        exit(-1);  
    } 
    
    // catches signal when child is created
    signal (SIGCHLD, sig_child); 
    
    ufds[0].fd = tcpfd;
    ufds[0].events = POLLIN;
    
    ufds[1].fd = udpfd;
    ufds[1].events = POLLIN;
    int flags;
    int timeout = 20000; // 20 second timeout for poll, -1 for no timeout

    // start persistent server
    for ( ; ; ) {
        char *buffer = new char[MAXLINE];
        if ((nready = poll(ufds, 2, -1)) < 0) {     
            //if (errno = EINTR) continue; // don't want signals to create interrupts
            perror("Poll");
            break;
        
        // should never be called with no timeouts
        } else if (nready == 0) {
            printf ("Timeout occured\n");
            break;
        
        
        } else if (ufds[0].revents & POLLIN) {
            printf("TCP connection established\n");
                
            // Accept the call
            confd = accept (tcpfd, (struct sockaddr*) &client, 
                &client_len);
            
            flags = 0;
            
            struct tcp_args threadArgs { confd, flags, buffer, };
            
            if (pthread_create (&threadID, NULL, tcp_handler, 
                    &threadArgs) != 0) {
                perror ("Thread_create");
                exit (1);
            }
            
        } else if (ufds[1].revents & POLLIN) {
            printf("UDP connection established\n");
            flags = 0; 
            
            struct udp_args threadArgs { udpfd, (sockaddr*) &client,
                client_len, flags, buffer, };
                   
            if (pthread_create (&threadID, NULL, udp_handler,
                    &threadArgs) != 0) {
                perror ("Thread_create");
                exit (1);
            }   
        }
    }
       
    sqlite3_close(db);
}

