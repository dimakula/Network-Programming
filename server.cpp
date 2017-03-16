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

#include "server.h"
using namespace std;

// the maximum number of queued connections allowed
#define MAX_CALLS 5
#define MAXLINE 512

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

// called upon hearing the control-C or control-Z signal
void signal_stop (int a) {
    fprintf (stderr, "\nCancelling server, closing all file handlers\n");
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
  
    peers++;
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

static int gossipCallback(void *message, int argc, char **argv, 
    char **azColName)
{
	char *tempPort;
	char *tempIP;
	int i = 0;
    tempIP = argv[i];
	tempPort = argv[i+1];

	sendUDP ((char *) message, tempIP, tempPort);
	return 0;
}

void sendUDP(char *message, char *address, char *port)
{
    int  sockfd, num, recieved;
    struct addrinfo hints, *servinfo, *p;
    char  buf[MAXLINE];  
	struct sockaddr_in peer;
	int totalSent, bytes; 

    
    bzero (&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    if ((recieved = getaddrinfo (address, port, &hints, &servinfo)) != 0) {
        perror ("Get address info");
        return;
    }
    
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket (p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            perror ("sendUDP");
            continue;
        }
        
        break;
    }

	if (p == NULL) {
	    fprintf (stderr, "sendUDP: Failed to create socket\n");
	    return;
	}
	
	while (totalSent < strlen (message)) {
	    
	    if ((bytes = sendto (sockfd, message, strlen(message), 0, p->ai_addr,
	        p->ai_addrlen)) == -1) {
	        
	        perror ("sendUDP send");
	        return;
	    }
	    	    
	    totalSent += bytes;
	}
	
	freeaddrinfo (servinfo);
	close (sockfd);
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

// Parses the user commands and stores them in an sqlite3
// database
char *reader (string fulltext) {

    int rc;
    string name, port, ip;
    string digest, message, date;
    string sql;
    string values;
    bool broadcast = false;
    bool peersRequest = false; // Set to true when PEERS? appears
    peers = 0; // number of peers currently known


    istringstream stream (fulltext);
    string split;
    
    // Buffer for error handling.
    char* zErrMsg = 0;
    
    getline (stream, split, ':');
    
    if (split == "GOSSIP") {
        getline (stream, digest, ':');
        getline (stream, date, ':');
        getline (stream, message, ':');
        
        values = "\'" + digest + "\', \'" + message +
            "\', \'" + date + "\'";
 
        sql = "INSERT INTO GOSSIP (DIGEST, MESSAGE, GENERATED)" \
              "VALUES (" + values + ");";
        
        broadcast = true;
    
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
        
        sql = "INSERT OR REPLACE INTO PEERS (PEER, PORT, IP)" \
              "VALUES (" + values + ");";
    
    } else if (split == "PEERS?") {
        sql = "SELECT * from PEERS;";
    }
    
    char* result = new char[512];
    char* output = new char[512];
    result[0] = '\0'; // because life is hard, and tears flow freely
    output[0] = '\0';

    rc = sqlite3_exec(db, sql.c_str(), callback, result, &zErrMsg);
    
    string peerNum = to_string (peers);
    strncat ((char *)output, "|PEERS:", 7);
    printf ("%s", output);

    strncat (output, peerNum.c_str(), strlen (peerNum.c_str()));
    strncat (output, "|", 1);
    strncat (output, result, strlen (result));
    strncat (output, "\n", 1);
    
    // Constraint due to primary key or not null
    if (rc == SQLITE_CONSTRAINT) {
        fprintf(stderr, "DISCARDED");
     
    } else if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
       
    } else if (broadcast) {
    	broadcastGossip((char *)message.c_str());
    }

    delete[] result; //free up heap
    
    // return the result of the PEERS? request, if applicable
    return (char *)output;
}

//handler for the tcp socket
void tcp_handler (int confd) {
  
    char buffer [MAXLINE];
    int totalRead = 0; // total number of bytes read from client
    int totalWrite = 0; // total bytes written to client
    char *result; 
    int bytes;
    bool receiving = true;
    
    // keep receiving until client closes connection
    while (receiving) {
    
        if ((bytes = recv (confd, &buffer, MAXLINE, 0)) != -1)
            totalRead += bytes;
        
        // if zero is returned, the client closed connection
        if (bytes == 0)
            receiving = false;
        
        printf ("%d\n", totalRead);
        // if the end of the message is received
        if (buffer[totalRead-1] == '%' || buffer[totalRead-1] == '\n') {
            printf ("TCP: %s\n", buffer);
            buffer[totalRead-1] = '\0';
            result = reader (buffer);
            
            if (peers > 0) {
                
                while (totalWrite < strlen (result)) {
                    if ((bytes = send (confd, result, strlen (result), 0)) != -1)
                        totalWrite += bytes;
                }
            }
                
            totalRead = 0;
            totalWrite = 0;
            buffer[0] = '\0'; //resets the buffer
       }
    }
}


// handler for the udp socket
void
udp_handler (int sockfd, sockaddr *client, socklen_t client_len) {
	int rc;
    int bytes;
	socklen_t len;
	char message [MAXLINE];
	char *split;
	char *result;
	int totalSend, totalRecv;
	bool receiving = true;
	
	// keep receiving until client closes connection
	while (receiving) {
	    len = client_len;
	    
	    // Receive until read all the message
	    if ((bytes = recvfrom(sockfd, message, MAXLINE, 0, client, &len)) != -1)
	        totalRecv += bytes;

		// if zero is received the client closed the connection
	    if(bytes == 0) 
		    receiving = false;

        if (message[totalRecv-1] == '%' || message[totalRecv-1] == '\n')
            message [totalRecv-1] = 0;
        
        printf ("UDP: %s\n", message);
        
        message[totalRecv-1] = '\0';
        result = reader (message);
        
        // resets the buffer
        message[0] = '\0';
        totalRecv = 0;
        
        printf ("Result: %s\n", result);
        
        // if there is a peers list that needs to be returned
        if (peers != 0) {

            while (totalSend < strlen (result)) {
                if((bytes = sendto(sockfd, result, strlen(result), 0, client,       
                    client_len)) != -1)
			        totalSend += bytes;
		    }
        }
	}
	
	delete []result;
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
    
    int maxfdp1, nready;
    int msgLength;
    fd_set rset; // check which file descriptor is set
    char message[MAXLINE];
    pid_t child;
    struct sockaddr_in client, server;

    socklen_t addr_len = sizeof (server);
    socklen_t client_len = sizeof (client);

    int recv_len;

    int tcpPort, udpPort;

    int childpid;
    void sig_child (int);
    
    struct timeval timeout; // timeout for selecting between file descriptors
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    

    // optional value for Setsockopt
    const int on = 1;
    
    // get tcp socket descriptor
    if ((tcpfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("TCP socket");
        exit (1);
    }

    // zeroes out the sin_zero field
    bzero(&server, addr_len);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons (port);
    
    // checking everything converted properly
    tcpPort = ntohs(server.sin_port);
    printf("Binding tcp connection to port %d\n", tcpPort);

    // SO_REUSEADDR allows multiple sockets to listen in on the port
    // also gets rid of "address in use" error
    if (setsockopt (tcpfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0)
        perror ("TCP options");

    // bind server to specified port
    if (bind (tcpfd, (struct sockaddr*) &server, addr_len) < 0) {
        perror ("TCP binding");
        close (tcpfd);
        exit(-1);  
    }

    // listen for incoming connections
    if (listen(tcpfd, MAX_CALLS) < 0)
        perror ("");

    //Build UDP socket 
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);   

    if(udpfd < 0)  
    {  
        perror("UDP socket");  
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
    int err_log = bind(udpfd, (struct sockaddr*) &server, sizeof(server));  
        
    if(err_log != 0)  
    {  
        perror("bind");  
        close(udpfd);        
        exit(-1);  
    } 
    
    // catches signal when child is created
    signal (SIGCHLD, sig_child); 
    
    FD_ZERO(&rset);
    
    // keep track of the biggest file descriptor for the select statement
    maxfdp1 = max (tcpfd, udpfd) + 1;

    for ( ; ; ) {
        FD_SET (tcpfd, &rset);
        FD_SET (udpfd, &rset);
        
        if ((nready = select (maxfdp1, &rset, NULL, NULL, &timeout)) < 0) {
            if (errno == EINTR)
                continue;
            else 
                perror("Select");
        }

        // if a tcp connection is requested
        if (FD_ISSET(tcpfd, &rset)) {
            printf("TCP connection established\n");
            
            // Accept the call
            confd = accept (tcpfd, (struct sockaddr*) &client, 
                    &client_len);

            if ((childpid = fork()) == 0) {
                close (tcpfd); // child doesn't need it
                tcp_handler (confd);
                exit (0);
            }

            close (confd);
        }

        // if a udp connection is requested
        if (FD_ISSET (udpfd, &rset)) {
            printf("UDP connection established\n");
            udp_handler (udpfd, (sockaddr*) &client, 
                client_len);  
            close (udpfd);       
        }
    }
    
    sqlite3_close(db);
}

