#include <stdlib.h>
#include <string.h> // for strlen
#include <strings.h> // for bzero
#include <unistd.h>
#include <algorithm>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

#define MAX_CALLS 5
#define MAXLINE 512

// tcp, udp and connection file descriptors
int tcpfd, udpfd, confd;

// called upon hearing the control-C or control-Z signal
void signal_stop (int a) {
    printf ("\nCancelling server, closing all file handlers\n");
    close (tcpfd);
    close (udpfd);
    close (confd);
    fflush (stdout);
    exit (1);
}  

void
udp_handler(int sockfd, sockaddr* client, socklen_t client_len) {
	int n;
	socklen_t len;
	char message [MAXLINE];
	

	for( ; ;) {
		len = client_len;
		n = recvfrom(sockfd, message, MAXLINE, 0, client, &len);
		message[n-1]=0;
		
		if(n==-1) 
		    perror("UDP reading");
		
		printf("received: %s from %s:%d\n", message, inet_ntoa(((sockaddr_in*)client)->sin_addr), ntohs(((sockaddr_in*)client)->sin_port));
		//((sockaddr_in*)pcliaddr)->sin_port=htons(ntohs(((sockaddr_in*)pcliaddr)->sin_port)+1);
		if((n=sendto(sockfd, message, n, 0, client, len))==-1)
			perror("sendto");
		printf("sent:%d bytes %s to %s:%d\n", n, message, inet_ntoa(((sockaddr_in*)client)->sin_addr), ntohs(((sockaddr_in*)client)->sin_port));
	}
}


void hallo (int sockfd) {

  char buf [MAXLINE];
  write (sockfd, "Hello!\n", strlen("Hello!\n"));
  for(int i=0;i<3;i++) {
      int msglen=read (sockfd, &buf, MAXLINE);
      write(sockfd, &buf, msglen);
  }
}

void sig_child (int signo) {
    pid_t pid;
    int stat;

    while ((pid = waitpid (-1, &stat, WNOHANG)) > 0)
        printf ("child %d terminated]n", pid);
    return;
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
    maxfdp1 = std::max (tcpfd, udpfd) + 1;

    for ( ; ; ) {
        FD_SET (tcpfd, &rset);
        FD_SET (udpfd, &rset);
        
        if ((nready = select (maxfdp1, &rset, NULL, NULL, &timeout)) < 0) {
            if (errno == EINTR)
                continue;
            else 
                perror("Select");
        }
        
        printf ("past the select!");
        
        // if a tcp connection is requested
        if (FD_ISSET(tcpfd, &rset)) {
            confd = accept (tcpfd, (struct sockaddr*) &client, 
                    &client_len);

            // if child process
            if ((childpid = fork()) == 0) {
                hallo (confd);
                close (tcpfd);
                //str_echo (confd); // process the request
                exit (0);
            }

            close (confd);
        }

        // if a udp connection is requested
        if (FD_ISSET (udpfd, &rset)) {
            printf("udp works\n");
            //udp_echo (confd, client, client_len);
            recv_len = recvfrom(udpfd, message, MAXLINE, 0, 
                    (struct sockaddr*)&client, &client_len);

            sendto (udpfd, message, recv_len, 0, 
                    (struct sockaddr*)&client, client_len);
                    
        }
    }
}

