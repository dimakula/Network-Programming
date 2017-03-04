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
        unsigned short port = 8081; // Local pot. 
      
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
          
        printf("receive data...\n");  
        while(1)  
        {  
            int recv_len;  
            char recv_buf[512] = {0};  
            struct sockaddr_in client_addr;  
            char cli_ip[INET_ADDRSTRLEN] = "";//INET_ADDRSTRLEN=16  
            socklen_t cliaddr_len = sizeof(client_addr);  
              
            // Receive the request from client.  
            recv_len = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&client_addr, &cliaddr_len);  
              
            // Print the client message out. 
            inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);  
            printf("\nip:%s ,port:%d\n",cli_ip, ntohs(client_addr.sin_port)); // ip of client.  
            printf("data(%d):%s\n",recv_len,recv_buf);  // Data of client.  
              
            //Return result back. 
            sendto(sockfd, recv_buf, recv_len, 0, (struct sockaddr*)&client_addr, cliaddr_len);  
        }  
          
        close(sockfd);  
          
        return 0;  
    }
	
