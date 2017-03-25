// GOSSIP:mBHL7IKilvdcOFKR03ASvBNX//ypQkTRUvilYmB1/OY=:2017-01-09-16-18-20-001Z:Tom eats Jerry%

#include "client-commands.h"

using namespace std;

#define MAXLINE 512


int safeFork () {
    
    int pid;
    
    if ((pid = fork()) == -1) {
        perror ("Fork");
        exit (EXIT_FAILURE);
    }
    
    return pid;
}

#define PEERS_QUERY 20
#define PEER_ADD 21
#define GOSSIP_ADD 22
#define INVALID 23

int callbuffer(const char* callbuffer){
  char* peers_query = strstr(callbuffer, "PEERS?");
  char* peer_add = strstr(callbuffer, "PEER:");
  char* gossip_add = strstr(callbuffer, "GOSSIP:");
  if (peers_query && !peer_add && !gossip_add){
    return PEERS_QUERY;
  } else if (peer_add && !peers_query && !gossip_add){
    return PEER_ADD;
  } else if (gossip_add && !peers_query && !peer_add){
    return GOSSIP_ADD;
  }

  return INVALID;
}

bool stopbuffer(char* buffer){
  int i = 0;
  while (buffer[i] != 0){
    if (buffer[i] == '%' || buffer[i] == '\n')
      return true;
    i++;
  }
  return false;
}



char* buffer;
int buffer_offset = 0;

string fullGossipMessage (string gossip, string timestamp) {
    
    stringstream sstream;
    char *buffer = new char [MAXLINE];
    string timestampAndMessage;
    
    // Create timestamp from current time if no timestamp was provided
    if (timestamp.empty()) {       

        typedef chrono::system_clock clock;

        auto now = clock::now();
        auto seconds = chrono::time_point_cast<chrono::seconds>(now);
        auto fraction = now - seconds;
 
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>   
                (fraction);
           
        time_t time = chrono::system_clock::to_time_t (now);
        sstream << put_time(localtime(&time), "%F-%H-%M-%S-");
        sstream << milliseconds.count() << "Z:";
        sstream << gossip;
        timestampAndMessage = sstream.str();
    
    } else {
        sstream << timestamp << ":";
        sstream << gossip;
        timestampAndMessage = sstream.str();
    }
       
    SHA256 sha256;
    sstream.str(string());
    sstream << "GOSSIP:" << sha256 (timestampAndMessage); 
    sstream << ":" << timestampAndMessage << "%";
    
    string fullMessage = sstream.str();
    printf ("%s\n", fullMessage.c_str());
    
    return fullMessage;
}

int main () {

    fullGossipMessage ("stuff", "");
    return 0;
}

