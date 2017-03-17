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
        fprintf (stderr, "SendUDP: Address does not exist\n");
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
	    fprintf (stderr, "sendUDP: Failed to create socket\n");
	    return -1;
	}
	
	while (totalSent < strlen (message)) {
	  
	    if ((bytes = sendto (sockfd, message, strlen(message), 0, p->ai_addr,
	        p->ai_addrlen)) == -1) {
	        
	        if (errno == EINTR) continue;
	        return (totalSent == 0) ? -1 : totalSent;
	    }
	    
	    printf ("%d %d\n", totalSent, strlen(message));    
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

// Parses the user commands and stores them in an sqlite3
// database
char *reader (string fulltext) {

    int rc;
    string name, port, ip;
    string digest, message, date;
    string sql;
    string values;
    bool broadcast = false; // check if message needs to be transmitted
    bool sendPeers = false; // check if the client asked for PEERS?
    peers = 0; // number of peers currently known


    istringstream stream (fulltext);
    string split; // contains text delimited by either ':'
    
    // Buffer for error handling.
    char* zErrMsg = 0;
    
    char* result = new char[MAXLINE];
    char* output = new char[MAXLINE];
    result[0] = '\0'; // because life is hard, and tears flow freely
    output[0] = '\0';
    
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
    
    } else if (split == "PEERS?") {
        sql = "SELECT * from PEERS;";
        sendPeers = true;
    }

    rc = sqlite3_exec(db, sql.c_str(), callback, result, &zErrMsg);
    
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
    
    // so that you don't send OK along with |PEERS:0| 
    } else if (!sendPeers) {
        strncat (output, "OK", 2);
    }
    
    strncat (output, "\n", 1);
    delete[] result; //free up heap
    
    // return output for the handler to send
    return (char *)output;
}

//handler for the tcp socket
int tcp_handler (int confd, int flags) {
  
    char buffer [MAXLINE];
    int totalRead = 0; // total number of bytes read from client
    int totalWrite = 0; // total bytes written to client
    char *result; 
    int bytes;
    bool receiving = true;
    
    // keep receiving until client closes connection
    while (receiving) {
    
        if ((bytes = recv (confd, &buffer, MAXLINE, flags)) != -1)
            totalRead += bytes;
        
        // if zero is returned, the client closed connection
        if (bytes == 0)
            receiving = false;
        
        // if the end of the message is received
        if (buffer[totalRead-1] == '\n') {
            
            if (buffer[totalRead-2] == '%')
                buffer[totalRead-2] = '\0';
                
            printf ("TCP: %s\n", buffer);
            result = reader (buffer);

            printf ("Result: %s\n", result);
            
            // Send result to the user
            while (totalWrite < strlen (result)) {
                if ((bytes = send (confd, result, strlen (result), 0)) == -1) {
                    if (errno == EINTR) continue;
                    return (totalWrite == 0) ? -1 : totalWrite;
                }
                    
                totalWrite += bytes;
            }
                
            totalRead = 0;
            totalWrite = 0;
            buffer[0] = '\0'; //resets the buffer
       }
    }
    
    printf ("Client has closed the connection\n");
    delete [] result;
    return 0;
}


// handler for the udp socket
int
udp_handler (int sockfd, sockaddr *client, socklen_t client_len, int flags) {
	int rc;
    int bytes;
	socklen_t len = client_len;
	char message [MAXLINE];
	char *split;
	char *result;
	int totalSent, totalRecv;
	bool receiving = true;
	
	// keep receiving until client closes connection
	while (receiving) {
	    
	    printf ("here\n");
	    // Receive until read all the message
	    if ((bytes = recvfrom(sockfd, message, MAXLINE, flags, client, &len)) != -1)
	        totalRecv += bytes;

		// if zero is received the client closed the connection
	    if(bytes == 0) 
		    receiving = false;

        if (message[totalRecv-1] == '\n') {
            
            if (message[totalRecv-2] == '%')
                message[totalRecv-2] = '\0';
                  
            printf ("UDP: %s\n", message);
            result = reader (message);
        
            // resets the buffer
            message[0] = '\0';
            totalRecv = 0;
        
            printf ("Result: %s\n", result);
        
            while (totalSent < strlen (result)) {
                
                if((bytes = sendto(sockfd, result, strlen(result), 0, client,       
                        client_len)) == -1) {
                    if (errno == EINTR) continue;
                    return (totalSent == 0) ? -1 : totalSent;
                }
			    
			    totalSent += bytes;
		    }
        }
	}
	
	printf ("Client has closed the connection\n");
	delete []result;
	return totalSent;
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
    int msgLength;
    char message[MAXLINE];
    pid_t child;
    struct sockaddr_in client, server;
    struct pollfd ufds[2]; // used to select between the udp and tcp

    socklen_t addr_len = sizeof (server);
    socklen_t client_len = sizeof (client);

    int recv_len;

    int tcpPort, udpPort;
    int childpid;
    void sig_child (int);

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
    
    ufds[0].fd = tcpfd;
    ufds[0].events = POLLIN | POLLPRI; // check for normal or out-of-band data
    
    ufds[1].fd = udpfd;
    ufds[1].events = POLLIN | POLLPRI;

    for ( ; ; ) {
        // -1 means no timeout occurs, ever
        if ((nready = poll(ufds, 2, -1)) < 0) {     
            //if (errno = EINTR) continue; // don't want signals to create interrupts
            perror("Poll");
        
        // should never be called with no timeouts
        } else if (nready == 0) {
            printf ("Timeout occured\n");
        
        } else if (ufds[0].revents) {
            printf("TCP connection established\n");
                
            // Accept the call
            confd = accept (tcpfd, (struct sockaddr*) &client, 
                &client_len);
                
            int flags;
            // check if receiving normal data
            if (ufds[0].revents & POLLIN)
                flags = 0;
                
            // check if out-of-band data
            else if (ufds[0].revents & POLLPRI)
                flags = MSG_OOB;
                
            if ((childpid = fork()) == 0) {
                close (tcpfd); // child doesn't need it
                tcp_handler (confd, flags);
                close (confd);
                exit (0);
            }
            
            close (confd);
            
        } else if (ufds[1].revents) {
            printf("UDP connection established\n");
            
            int flags;
            // check if receiving normal data
            if (ufds[1].revents & POLLIN)
                flags = 0;
                
            // check if out-of-band data
            else if (ufds[1].revents & POLLPRI)
                flags = MSG_OOB;
            
            udp_handler (udpfd, (sockaddr*) &client, 
                client_len, flags);  
            close (udpfd);  
        }
    }
       
    sqlite3_close(db);
}

