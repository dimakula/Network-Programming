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
    printf ("\nCancelling server, closing all file handlers\n");
    close (tcpfd);
    close (udpfd);
    close (confd);
    sqlite3_close (db);
    fflush (stdout);
    exit (1);
}

static int
callback (void *result, int argc, char **argv, char **azColName) {
    
    // 3 because 3 columns in the table
    string peerNum = to_string(argc / 3);
    
    char *output = new char [MAXLINE];
    strcat (output, "|PEERS:");
    printf ("here\n");

    strcat (output, peerNum.c_str());
    strcat (output, "|");
    
    int i;
    for (i = 0; i < argc; i++) {
        
        if (strcmp (azColName[i], "PORT"))
            strcat (output, "PORT=");
        
        else if (strcmp (azColName[i], "IP"))
            strcat (output, "IP=");
            
        strcat (output, argv[i]);
        strcat (output, ":");
    }

    strcat (output, "\n");
    result = output;
    return 0;
}

static int gossipCallback(void *result, int argc, char **argv, char **azColName)
{
	string tempPort;
	string tempIP;

	for( int i = 0; i < argc; i++ )
	{
		if( azColName[i] == "PORT")
		{
			tempPort = argv[i];
		}
		else if(azColName[i] == "IP")
		{
			tempIP = argv[i];
 		}
		
		//TODO: Add needed logic to switch between UDP and TCP.
	    //sendTCP( latestGossip, tempIP, tempPort );
	}
	
	return 0;
}

void sendUDP(string message, string address, int port)
{

	return;
}

void sendTCP(string message, string address, int port)
{

	return;
}

void broadcastGossip() {

	string sql;
	int rc;
	sql = "SELECT (IP, PORT) FROM PEERS";

	// Storage for error code.
	char* zErrMsg = new char();
	(*zErrMsg) = 0;

	rc = sqlite3_exec(db, sql.c_str(), gossipCallback, 0, &zErrMsg);

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


    istringstream stream (fulltext);
    string split;
    
    char *zErrMsg = 0;
    
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
        latestGossip = message;
    
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
    
    void *result;
    rc = sqlite3_exec(db, sql.c_str(), callback, &result, &zErrMsg);
    
    if (rc == SQLITE_CONSTRAINT_UNIQUE) {
        fprintf(stderr, "DISCARDED");
     
    } else if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
       
    } else if (broadcast) {
        ////////////// CALL BROADCAST CODE HERE ////////////////////////
    	broadcastGossip();
    }

    // result contains the string for PEERS? request
    return (char *)result;
}

//handler for the tcp socket
void tcp_handler (int confd) {
  
    char buffer [MAXLINE] = "\0";
    int totalRead = 0; // total number of bytes read
    char *result; 
    
    for( ; ;) {
        totalRead += read (confd, &buffer, MAXLINE);
        printf ("%s\n", buffer);
      
        // if the end of the message is received
        if (buffer[totalRead - 1] == '%') {
            buffer[totalRead - 1] = '\0';
            result = reader (buffer);
            
            if (result != NULL && result[0] != '\0') {
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
        
        // datagram arrives in one package, so no need to check for more input
        split = strtok (message, "%\n");
        result = reader (split);
        printf ("%s\n", result);
        if (result != NULL && result[0] != '\0') {

            if((n=sendto(sockfd, result, n, 0, client, strlen(result)))==-1)
			    perror("sendto");
			  
        }
	}
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
    bzero(&server, sizeof(server));   // Clear.
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
            udp_handler (udpfd, (sockaddr*) &client, 
                client_len);  
            close (udpfd);       
        }
    }
    
    sqlite3_close(db);
}

