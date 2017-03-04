#include <stdlib.h>
#include <string.h>
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

void sig_child (int signo) {
    pid_t pid;
    int stat;

    while ((pid = waitpid (-1, &stat, WNOHANG)) > 0)
        printf ("child %d terminated]n", pid);
    return;
}

int main (int argc, char *argv[]) {
    
    // tcp, udp and connection file descriptors
    int tcpfd, udpfd, confd;
    int maxfdp1, nready;
    int msgLength;
    fd_set rset;
    char message[MAXLINE];
    pid_t child;
    struct sockaddr_in client, server;

    socklen_t addr_len = sizeof (server);
    socklen_t client_len = sizeof (client);

    // Port numbers for tcp and udp
    int tcpPort, udpPort;

    int recv_len;

    int childpid;
    void sig_child (int);
    

    // optional value for Setsockopt
    const int on = 1;
    
    // create listening TCP socket
    if ((tcpfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("TCP Socket error");
        exit (1);
    }

    // clear the server address, just in case
    bzero(&server, addr_len);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons (0); // will allocate random port
    
    // get the port number for the tcp connection
    if (getsockname(tcpfd, (struct sockaddr*) &server, &addr_len) == -1)
        perror ("get sock name error");

    else {
        tcpPort = ntohs(server.sin_port);
        printf("Binding tcp connection to port %d\n", tcpPort);  
    }

    setsockopt (tcpfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)); 

    
    if (bind (tcpfd, (struct sockaddr*) &server, addr_len) < 0)
        perror ("");

    if (listen(tcpfd, MAX_CALLS) < 0)
        perror ("");

    //Build UDP socket 
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);   
    
    if(udpfd < 0)  
    {  
        perror("socket");  
        exit(-1);  
    }  
          
    //Config network
    bzero(&server, sizeof(server));   // Clear. 
    server.sin_family = AF_INET;       // IPv4.  
    server.sin_port   = htons(0);   // port.  
    server.sin_addr.s_addr = htonl(INADDR_ANY); // ip.  
    
    // get udp port
    if (getsockname(udpfd, (struct sockaddr*) &server, &addr_len) == -1)
        perror ("get sock name error");

    else {
        udpPort = ntohs(server.sin_port);
        printf("Binding udp connection to port %d\n", udpPort);
    }
          
    //Bind socket.
    int err_log = bind(udpfd, (struct sockaddr*) &server, sizeof(server));  
        
    if(err_log != 0)  
    {  
        perror("bind");  
        close(udpfd);        
        exit(-1);  
    } 
    
    signal (SIGCHLD, sig_child); 
    
    FD_ZERO(&rset);
    maxfdp1 = std::max (tcpfd, udpfd) + 1;

    for ( ; ; ) {
        FD_SET (tcpfd, &rset);
        FD_SET (udpfd, &rset);
        
        if ((nready = select (maxfdp1, &rset, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR)
                continue;
            else 
                perror("");
        }
        
        // if a tcp connection is requested
        if (FD_ISSET(tcpfd, &rset)) {
            confd = accept (tcpfd, (struct sockaddr*) &client, 
                    &client_len);

            // if child process
            if ((childpid = fork()) == 0) {
                close (tcpfd);
                //str_echo (confd); // process the request
                exit (0);
            }

            close (confd);
        }

        // if a udp connection is requested
        if (FD_ISSET (udpfd, &rset)) {
            recv_len = recvfrom(udpfd, message, MAXLINE, 0, 
                    (struct sockaddr*)&client, &client_len);

            sendto (udpfd, message, recv_len, 0, 
                    (struct sockaddr*)&client, client_len);
        }
    }
}

