#ifndef CLIENT_COMMANDS
#define CLIENT_COMMANDS

#include <iostream>
#include <iomanip>
#include <ctime>
#include <stdio.h>      /* printf */
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include <string.h>     /* for strlen */
#include <chrono>       /* chrono clock */
#include <sstream>      /* stringstream */
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "hash-library/sha256.h" // external hashing library

int safeFork ();

std::string fullPeerMessage (std::string, std::string, std::string);

std::string fullGossipMessage (std::string, std::string);

#endif
