#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>

#define MAX_CALLS 5
#define MAXLINE 512

int main (int argc, char **argv[]) {
    
    int tcpfd, connfd, udpfd, cfd;
    int msgLength;
    fd_set rset;
    char message[MAXLINE];
    pid_t child;
    struct sockaddr_in client, server;
    socklen_t addr_len = sizeof(sockaddr_in);
    int tcpPort, udpPort;
    void sig_child (int);
    

    // optional value for Setsockopt
    const int on = 1;
    
    // create listening TCP socket
    if ((tcpfd = Socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("TCP Socket error");
        exit (1);
    }

    // clear the server address, just in case
    bzero(&servaddr, sizeof (servaddr));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = hton1(INADDR_ANY);
    server.sin_port = htons (0); // will allocate random port
    
    // get the port number for the tcp connection
    tcpPort = getsockname(tcpfd, &server, &addr_len);

    Setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)); 

    
    printf("Binding tcp connection to port %d\n", tcpPort);  
    
    if (bind (tcpfd, (sockaddr*) &server, addr_len) < 0)
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
    server.sin_port   = htons(port);   // port.  
    server.sin_addr.s_addr = htonl(INADDR_ANY); // ip.  
        
    udpPort = getsockname(udpfd, &server, &addr_len);
      
    printf("Binding udp connection to port %d\n", udpPort);
          
    //Bind socket.
    int err_log = bind(udpfd, (struct sockaddr*)&server, sizeof(server));  
        
    if(err_log != 0)  
    {  
        perror("bind");  
        close(udpfd);        
        exit(-1);  
    } 
    
    Signal (SIGCHILD, sig_child); 
    
    FD_ZERO(&rset);
    maxfdp1 = max (listenfd, udpfd) + 1;

    for ( ; ; ) {
        FD_SET (listenfd, &rset);
        FD_SET (udpfd, &rset);
        
        if ((nready = select (maxfdp1, &rset, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR)
                continue;
        }
        
        

        // doStuff (cfd);
        close (cfd);
    }
}

