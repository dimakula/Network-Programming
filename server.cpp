#include "server.h"
using namespace std;

#define MAX_CALLS 5
#define MAXLINE 128

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
	//sendTCP (message, tempIP, tempPort);
	return 0;
}

void sendUDP(char *message, char *address, char *port)
{
    int  sockfd, num;  
    char  buf[MAXLINE];  
	struct sockaddr_in peer;
	int portnum = atoi (port);  

      		    
	if((sockfd = socket(AF_INET, SOCK_DATAGRAM, 0))==-1) {  
        perror ("Socket:");
		return;    
	}
		
	// set socket to non-blocking: if the client ain't listening,
	// then they won't get the message
	fcntl (sockfd, F_SETFL, O_NONBLOCK);
		    
	bzero(&peer, sizeof(peer));
	peer.sin_family= AF_INET;  
	peer.sin_port = htons(portnum);  
	inet_pton(AF_INET, address, &(peer.sin_addr));
	
	fd_set rset;
	int nready;
		
	FD_ZERO (&rset);
	FD_SET (sockfd, &rset);
        
    if ((nready = select (sockfd + 1, &rset, NULL, NULL, 0)) < 0) {

		if (FD_ISSET(sockfd, &rset)) {
	        for ( ; ;) {
	            printf ("Blocking\n");
	            if (sendto (sockfd, message, sizeof(message), 0,
		            (struct sockaddr *) &peer, sizeof(peer)) < 0) {
		            perror ("sendto");
		        }
		    }
		}
	}
}

void sendTCP(char *message, char *address, char *port)
{
    int  sockfd, num;  
    char  buf[MAXLINE];  
	struct sockaddr_in peer;
	int portnum = atoi (port);  
      	
    if (inet_pton (AF_INET, address, &(peer.sin_addr.s_addr) == -1)
        perror ("inet_pton");
    	    
	bzero (&peer, sizeof(peer));
	peer.sin_family = AF_INET;
	peer.sin_port = htons (port);
	
	if (sockfd = socket (AF_INET, SOCK_STREAM, 0) {
	    perror ("socket");
	 
    //Something needs to go here	 
   
    if ((nready = select (sockfd + 1, &rset, NULL, NULL, 0)) < 0) {

		if (FD_ISSET(sockfd, &rset)) {
            // send stuff here
		}
	}
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
    char *input;
    int totalRead = 0; // total number of bytes read from client
    int totalWrite = 0; // total bytes written to client
    char *result; 
    
    for( ; ;) {
        totalRead += read (confd, &buffer, MAXLINE);
      
        // if the end of the message is received
        if (buffer[totalRead - 1] == '%' || buffer[totalRead - 1] == '\n') {
            printf ("TCP: %s\n", buffer);
            input = strtok (buffer, "%\n");
            result = reader (input);
            
            if (peers > 0) {
                write (confd, result, strlen (result));
            }
                
            totalRead = 0;
            buffer[0] = '\0'; //resets the buffer
       }
    }
}


// handler for the udp socket
void
udp_handler (int sockfd, sockaddr *client, socklen_t client_len) {
	int rc;
    int n;
	socklen_t len;
	char message [MAXLINE];
	char *split;
	char *result;
	
	for( ; ;) {
	    len = client_len;
	    n = recvfrom(sockfd, message, MAXLINE, 0, client, &len);
		
	    if(n == -1) 
		    perror("UDP reading");

        message [n-1] = 0;
        
        printf ("UDP: %s\n", message);
        
        // datagram arrives in one package, so no need to check for more input
        split = strtok (message, "%\n");
        result = reader (split);
        printf ("Result: %s\n", result);
        
        // if there is a peers list that needs to be returned
        if (peers != 0) {   
            // sends 
            if((n=sendto(sockfd, result, strlen(result), 0, client, client_len))==-1)
			    perror("sendto");		  
        }
	}
	
	delete []result;
}

// run when child is created
void sig_child (int signo) {
    pid_t pid;
    int stat;

    while ((pid = waitpid (-1, &stat, WNOHANG)) > 0)
        printf ("child %d terminated]n", pid);
    return;
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
    
    // create listening TCP socket
    if ((tcpfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("TCP socket");
        exit (1);
    }

    // clear the server structure, just in case
    bzero(&server, addr_len);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons (port);
    
    // checking everything converted properly
    tcpPort = ntohs(server.sin_port);
    printf("Binding tcp connection to port %d\n", tcpPort);

    // SO_REUSEADDR allows multiple sockets to listen in on the port
    if (setsockopt (tcpfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0)
        perror ("TCP options");

    if (bind (tcpfd, (struct sockaddr*) &server, addr_len) < 0) {
        perror ("TCP binding");
        close (tcpfd);
        exit(-1);  
    }

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
            confd = accept (tcpfd, (struct sockaddr*) &client, 
                    &client_len);

            if ((childpid = fork()) == 0) {;
                close (tcpfd);
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

