/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2017 
                Authors:  Dmitry Kulakov, Xiaohan Yang, Nicholas Zynko
                Florida Tech, Computer Science
   
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
#include <sqlite3.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include<netdb.h>
#include <fcntl.h>
#include <sys/poll.h> // for poll
#include <pthread.h>
#include <sys/ioctl.h>

///////////////////////////
// Forwarded Subroutines //
///////////////////////////

void gossipCallback(void* , int, char, char);

int broadcast( char *, char *, char * );

void signal_stop (int);

void broadcastGossip(char *);

// Parses the user commands and stores them in an sqlite3
// database
char* reader (char *, int &);

//handler for the tcp socket
void* tcp_handler ( int, int );

// handler for the udp socket
void* udp_handler (int , sockaddr*, socklen_t, int );

// run when child is created
void sig_child (int signo);

// sets up sqlite database for storing gossip
void setup_database (char *filePath);
