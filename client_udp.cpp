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

    #include <stdio.h>  
    #include <stdlib.h>  
    #include <unistd.h>  
    #include <string.h>  
    #include <sys/types.h>  
    #include <sys/socket.h>  
    #include <netinet/in.h>  
    #include <netdb.h>  
      
    #define PORT 8081 
    #define MAXDATASIZE 512  
      
    int main(int argc, char *argv[]) {  
        int sockfd, num;  
        char buf[MAXDATASIZE];  
      
        struct hostent *he;  
        struct sockaddr_in server,peer;  
      
        if (argc !=3) {  
			printf("Usage: %s <IP Address><message>\n",argv[0]);  
        exit(1);  
        }  
      
        if ((he=gethostbyname(argv[1]))==NULL) {  
			printf("gethostbyname()error\n");  
        exit(1);  
        }  
      
        if ((sockfd=socket(AF_INET, SOCK_DGRAM,0))==-1) {  
			printf("socket() error\n");  
		exit(1);  
        }  
      
        bzero(&server,sizeof(server));  
        server.sin_family = AF_INET;  
        server.sin_port = htons(PORT);  
        server.sin_addr= *((struct in_addr *)he->h_addr);  
        sendto(sockfd, argv[2],strlen(argv[2]),0,(struct sockaddr *)&server,sizeof(server));  
        socklen_t  addrlen;  
        addrlen=sizeof(server);  
			while (1) {  
				if((num=recvfrom(sockfd,buf,MAXDATASIZE,0,(struct sockaddr *)&peer,&addrlen))== -1) {  
					printf("recvfrom() error\n");  
				exit(1);  
			}  
				if (addrlen != sizeof(server) ||memcmp((const void *)&server, (const void *)&peer,addrlen) != 0) {  
					printf("Receive message from otherserver.\n");  
				continue;  
				}  
      
		buf[num]='\0';  
		printf("Server Message:%s\n",buf);  
        break;  
        }  
      
        close(sockfd);  
        }  
 
