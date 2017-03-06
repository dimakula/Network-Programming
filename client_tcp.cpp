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
   
#define  PORT 8080 
#define  MAXDATASIZE 512 
   
int main(int argc, char *argv[])  {  
		int  sockfd, num;  
		char  buf[MAXDATASIZE];  
		struct hostent *he;  
		struct sockaddr_in server;  
		    if (argc!=2) {  
		        printf("Usage:%s <IP Address>\n",argv[0]);  
		    exit(1);  
		    }  
		    if((he=gethostbyname(argv[1]))==NULL) {  
		        printf("gethostbyname()error\n");  
		    exit(1);  
		    }  
		    if((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1) {  
		        printf("socket()error\n");  
		    exit(1);  
		    }  
		    bzero(&server,sizeof(server));  
		    server.sin_family= AF_INET;  
		    server.sin_port = htons(PORT);  
		    server.sin_addr =*((struct in_addr *)he->h_addr);  
		        if(connect(sockfd,(struct sockaddr *)&server,sizeof(server))==-1) {  
		            printf("connect()error\n");  
		        exit(1);  
		        }  
		            if((num=recv(sockfd,buf,MAXDATASIZE,0)) == -1){  
		                printf("recv() error\n");  
		            exit(1);  
		            }  
		buf[num-1]='\0';  
		printf("Server Message: %s\n",buf);  
		close(sockfd);  
	return 0;  
}
