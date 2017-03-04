/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2017 
                Author:  xiaohan2014@my.fit.edu
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
    #include <string.h>  
    #include <unistd.h>  
    #include <sys/socket.h>  
    #include <netinet/in.h>  
    #include <arpa/inet.h>  
      
    int main(int argc, char *argv[])  
    {  
        unsigned short port = 8080; // Local pot. 
      
        //Build UDP socket 
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);   
        if(sockfd < 0)  
        {  
            perror("socket");  
            exit(-1);  
        }  
          
        //Config network
        struct sockaddr_in my_addr;  
        bzero(&my_addr, sizeof(my_addr));   // Clear. 
        my_addr.sin_family = AF_INET;       // IPv4.  
        my_addr.sin_port   = htons(port);   // port.  
        my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // ip.  
          
        printf("Binding server to port %d\n", port);  
          
        //Bind socket.
        int err_log = bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr));  
        if(err_log != 0)  
        {  
            perror("bind");  
            close(sockfd);        
            exit(-1);  
        }  
          
        //Listen, passive socket.  
        err_log = listen(sockfd, 10);   
        if(err_log != 0)  
        {  
            perror("listen");  
            close(sockfd);        
            exit(-1);  
        }     
          
        printf("listen client @port=%d...\n",port);  
      
        while(1)  
        {     
          
            struct sockaddr_in client_addr;          
            char cli_ip[INET_ADDRSTRLEN] = "";       
            socklen_t cliaddr_len = sizeof(client_addr);      
              
            //Get connected link from client
            int connfd;  
            connfd = accept(sockfd, (struct sockaddr*)&client_addr, &cliaddr_len);         
            if(connfd < 0)  
            {  
                perror("accept");  
                continue;  
            }  
      
            //Print client's ip and port.  
            inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);  
            printf("client ip: %s,port: %d\n", cli_ip,ntohs(client_addr.sin_port));  
              
            //Accept data. 
            char recv_buf[512] = {0};  
            int len =  recv(connfd, recv_buf, sizeof(recv_buf), 0);  
              
            //Print the client message out. 
            printf("\nrecv data:\n");  
            printf("%s\n",recv_buf);  
              
            //Return result back. 
            send(connfd, recv_buf, len, 0);  
              
            //Close the connected socket. 
            close(connfd);       
            printf("client closed!\n");  
        }  
          
        //Close the listen sock. 
        close(sockfd);           
          
        return 0;  
    }
	