// GOSSIP:mBHL7IKilvdcOFKR03ASvBNX//ypQkTRUvilYmB1/OY=:2017-01-09-16-18-20-001Z:Tom eats Jerry%

#include "../include/client-commands.h"

using namespace std;

#define MAXLINE 512

asn1_node definitions = NULL;	
asn1_node structure = NULL;
char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];
int result;

// call before doing any other method
int init_parseTree () {
    
    if ((result = asn1_parser2tree ("ApplicationList.asn", &definitions, errorDescription))
            != ASN1_SUCCESS) {
    
        asn1_perror (result); 
        fprintf(stderr, "Parser2tree error = \"%s\"\n", errorDescription);
        return -1; 
    }
    
    return 0;
}

int timestampAndHash (string gossip, string &timestamp, string &shaString) {

    stringstream sstream;
    string temp;
    string timestampAndMessage;
    char *returnTime;
    
    // Create timestamp from current time if no timestamp was provided
    if (!timestamp.empty()) {       

        typedef chrono::system_clock clock;

        auto now = clock::now();
        auto seconds = chrono::time_point_cast<chrono::seconds>(now);
        auto fraction = now - seconds;
 
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>   
                (fraction);
           
        time_t time = chrono::system_clock::to_time_t (now);
        sstream << put_time(localtime(&time), "%F-%H-%M-%S-");
        sstream << milliseconds.count() << "Z";
        timestamp = sstream.str();
    
    } else {
        sstream << timestamp;
    }
       
    SHA256 sha256;
    sstream << ":" << gossip;
    timestampAndMessage = sstream.str();
    sstream.str(string());
    sstream << sha256 (timestampAndMessage);
    shaString = sstream.str();
    
    return 0;
}

int PeersEncode (char *dataBuff) {

	//PeersQuery ::= [APPLICATION 3] IMPLICIT NULL
	result = asn1_create_element(definitions, "ApplicationList.PeersAnswer", &structure);
		
	result = asn1_write_value(structure, "NULL", NULL, 1);
		
	int size = MAXLINE;
		
	result = asn1_der_coding (structure, "", dataBuff, &size, errorDescription);
	//asn1_delete_structure (structure);
	
	if(result != ASN1_SUCCESS) {
		asn1_perror (result);
		printf("Encoding error = \"%s\"\n", errorDescription);
		return -1;
	}

	return 0;
}

int PeerEncode(string peer, string ip, string port, char *dataBuff) {
    
    if ((result = asn1_create_element(definitions, "ApplicationList.Peer", &structure)) != 1)
        asn1_perror (result);
		
	if ((result = asn1_write_value(structure, "name", peer.c_str(), peer.length())) != ASN1_SUCCESS) 
	    asn1_perror (result);
	if ((result = asn1_write_value(structure, "port", port.c_str(), port.length())) != ASN1_SUCCESS)
	    asn1_perror (result);
	if ((result = asn1_write_value(structure, "ip",   ip.c_str(),   ip.length()))   != ASN1_SUCCESS)
	    asn1_perror (result);
	
	int size = MAXLINE;
		
	result = asn1_der_coding (structure, "", dataBuff, &size, errorDescription);
	//asn1_delete_structure (structure);
	
	if(result != ASN1_SUCCESS) {
		asn1_perror (result);
		printf("Encoding error = \"%s\"\n", errorDescription);
		return -1;
	}
	
	return 0;
}


int MessageEncode(string gossip, string timestamp, char *dataBuff) {
	
	int result = 0;
	int size = MAXLINE;

	//Gossip ::= [APPLICATION 1] EXPLICIT SEQUENCE {
	//sha256hash OCTET STRING,
	//timestamp GeneralizedTime,
	//message UTF8String
	//}
	result = asn1_create_element(definitions, "ApplicationList.Gossip", &structure);
	string sha256 = NULL;
	
	timestampAndHash (gossip, timestamp, sha256);

	result = asn1_write_value(structure, "sha256hash", sha256.c_str(), sha256.length());
	result = asn1_write_value(structure, "time", timestamp.c_str(), timestamp.length());
	result = asn1_write_value(structure, "message", gossip.c_str(), gossip.length());
		
	result = asn1_der_coding (structure, "", dataBuff, &size, errorDescription);
	//asn1_delete_structure (structure);
	
	if(result != ASN1_SUCCESS) {
		asn1_perror (result);
		printf("Encoding error = \"%s\"\n", errorDescription);
	}
	
	return 0;
}

string fullPeerMessage (string peer, string ip, string port) {

    stringstream sstream;
    string fullPeer;
    
    sstream << "PEER:" << peer << ":PORT=" << port << ":IP=" << ip << "%";
    fullPeer = sstream.str();
    return fullPeer;
}

string fullGossipMessage (string gossip, string timestamp) {
    
    stringstream sstream;
    char *buffer = new char [MAXLINE];
    string timestampAndMessage;
    string sha256;
    
    timestampAndHash (gossip, timestamp, sha256);
    
    sstream << "GOSSIP:" << sha256 << ":" << timestamp << ":" << gossip << "%";
    string fullMessage = sstream.str();
    printf ("%s\n", fullMessage.c_str());
    
    return fullMessage;
    /*
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
    */
}


