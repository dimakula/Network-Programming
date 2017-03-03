#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>

#define MAX_CALLS 5
#define MAXLINE 200

int main (int argc, char **argv[]) {
    
    int sockfd, cfd;
    int msgLength;
    fd_set rset;
    char message[MAXLINE];
    pid_t child;
    struct sockaddr_in client, server;
    socklen_t addr_len = sizeof(sockaddr_in);

    // optional value for Setsockopt
    const int on = 1;
    
    // listen to TCP socket
    if ((listenfd = Socket (AF_INET, SOCK_STREAM, 0)) == -1) {
        perror ("TCP Socket error");
        exit (1);
    }

    // clear the server address, just in case
    bzero(&servaddr, sizeof (servaddr));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = hton1(INADDR_ANY);
    server.sin_port = htons (0); // will allocate random port

    Setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)); 

    int port = getsockname(sockfd, &server, &addr_len);
    
    if (bind (sockfd, (sockaddr*) &server, addr_len) < 0)
        perror ("");

    if (listen(sockfd, MAX_CALLS) < 0)
        perror ("");

    /*
     Add udp code here maybe
    */

    for ( ; ; ) {
        FD_SET (listenfd, &rset);
        // FD_SET (udpfd, &rset);
        if ((cfd == accept (sockfd, (sockaddr*) &client, &addr_len)) < 0)
            perror("");

        // doStuff (cfd);
        close (cfd);
    }
}

