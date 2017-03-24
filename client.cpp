/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2017 
                Author: xiaohan2014@my.fit.edu
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

#include<stdio.h>  
#include <stdlib.h>  
#include<unistd.h>  
#include<string.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  
#include<netdb.h>  

//Maximum the buff size as 512.
#define MAXDATASIZE 512 

void udp_client(int argc, char *argv[]){
        struct hostent *he;  
        struct sockaddr_in server,peer;  
		
		int port;
		int sockfd, num;  
  
		//Check whether the socket is created successfully.
		if ((sockfd=socket(AF_INET, SOCK_DGRAM,0))==-1) {  
			printf("Fail to create the scoket! \n",argv[0]);  
        exit(1);  
        }  
		
		// Initial UDP client.	
		bzero(&server,sizeof(server));  
        server.sin_family = AF_INET;  
        server.sin_port = htons(port);  
        server.sin_addr= *((struct in_addr *)he->h_addr);  
		
		//Send data.
        sendto(sockfd, argv[2],strlen(argv[2]),0,(struct sockaddr *)&server,sizeof(server));  
        socklen_t  addrlen; 
		
		//Size of the server.
        addrlen=sizeof(server);  
	
		//Print the buff.
		buf[num]='\0';  
		printf("Server Message:%s\n",buf);  
		
		//Create the socket.
		udpfd = socket(AF_INET, SOCK_DGRAM, 0);
}

		//Use the same sockaddr for tcp as well.
	void tcp_client(){
		
		//Check whether the socket is created successfully.
		if ((sockfd=socket(AF_INET, SOCK_DGRAM,0))==-1) {  
			printf("Fail to create the scoket! \n",argv[0]);  
        exit(1);  
        }  
		
		tcpfd = socket(AF_INET, SOCK_STREAM, 0);
		
		if (connect(tcpfd, (struct sockaddr *) &server, sizeof(server)) < 0)
        printf("TCP connection error!\n");
}

void parallel(){
	
	
}

void inteface() {
	
}
