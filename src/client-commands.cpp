// GOSSIP:mBHL7IKilvdcOFKR03ASvBNX//ypQkTRUvilYmB1/OY=:2017-01-09-16-18-20-001Z:Tom eats Jerry%

#include "../include/client-commands.h"

using namespace std;

#define MAXLINE 2048

asn1_node definitions = NULL;	
asn1_node structure = NULL;
char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];
int result;


const asn1_static_node ApplicationList_asn1_tab[] = {
  { "ApplicationList", 536875024, NULL },
  { NULL, 1073741836, NULL },
  { "Gossip", 1610620933, NULL },
  { NULL, 1073744904, "1"},
  { "sha256hash", 1073741831, NULL },
  { "timestamp", 1073741861, NULL },
  { "message", 34, NULL },
  { "Peer", 1610620933, NULL },
  { NULL, 1073746952, "2"},
  { "name", 1073741858, NULL },
  { "port", 1073741827, NULL },
  { "ip", 31, NULL },
  { "PeersQuery", 1610620948, NULL },
  { NULL, 5128, "3"},
  { "PeersAnswer", 1610620939, NULL },
  { NULL, 1073743880, "1"},
  { NULL, 2, "Peer"},
  { "UTF8String", 1610620935, NULL },
  { NULL, 4360, "12"},
  { "PrintableString", 536879111, NULL },
  { NULL, 4360, "19"},
  { NULL, 0, NULL }
};


// call before doing any other method
int init_parseTree () {
    
    if ((result = asn1_array2tree (ApplicationList_asn1_tab, &definitions, errorDescription))
            != ASN1_SUCCESS) {
    
        asn1_perror (result); 
        fprintf(stderr, "Array2tree error = \"%s\"\n", errorDescription);
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
    if (timestamp.empty()) {       

        typedef chrono::system_clock clock;

        auto now = clock::now();
        auto seconds = chrono::time_point_cast<chrono::seconds>(now);
        auto fraction = now - seconds;
 
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>   
                (fraction);
        
        // conforms to the asn1 definition for Generalized time
        // YYYYMMDDhhmmss.sZ
        time_t time = chrono::system_clock::to_time_t (now);
        sstream << put_time(localtime(&time), "%G%m%d%H%M%S");
        sstream << "." << milliseconds.count() << "Z";
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
	asn1_delete_structure (&structure);
	asn1_delete_structure (&definitions);
	
	if(result != ASN1_SUCCESS) {
		asn1_perror (result);
		fprintf(stderr, "Encoding error = \"%s\"\n", errorDescription);
		return -1;
	}

	return 0;
}

int PeerEncode(char *peer, char *ip, char *port, char *dataBuff) {
    
    init_parseTree();
    asn1_create_element(definitions, "ApplicationList.Peer", &structure);
		
	if ((result = asn1_write_value(structure, "name", peer, strlen(peer))) 
	        != ASN1_SUCCESS) 
	    asn1_perror (result);
	if ((result = asn1_write_value(structure, "port", port, strlen(port))) 
	        != ASN1_SUCCESS)
	    asn1_perror (result);
	if ((result = asn1_write_value(structure, "ip", ip, strlen(ip))) 
	        != ASN1_SUCCESS)
	    asn1_perror (result);
	
	int size = MAXLINE;
		
	result = asn1_der_coding (structure, "", dataBuff, &size, errorDescription);
	asn1_delete_structure (&structure);
	asn1_delete_structure (&definitions);
	
	if(result != ASN1_SUCCESS) {
		asn1_perror (result);
		fprintf(stderr, "Encoding error = \"%s\"\n", errorDescription);
		return -1;
	}

	/*	    result = asn1_create_element(definitions, "ApplicationList.Peer", &structure);
		    result = asn1_der_decoding (&structure, dataBuff, strlen(dataBuff), errorDescription);

		    if(result != ASN1_SUCCESS)  {
			    asn1_perror (result);
			    printf("Decoding error = \"%s\"\n", errorDescription);
		    }

            char *charname = new char[MAXLINE];
            char *charport = new char[MAXLINE];
            char *charip   = new char[MAXLINE];
            int length = MAXLINE;
            
		    printf("{APPLICATION} recieved..\n");
		    result = asn1_read_value(structure, "name", charname, &length);
		    printf("\tName =\"%s\"\n", charname);

		    result = asn1_read_value(structure, "port", charport, &length);
		    printf("\tPort =\"%s\"\n", charport);
		    
		    result = asn1_read_value(structure, "ip", charip, &length);
		    printf("\tIP =\"%s\"\n", charip);
		    

	*/
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
	
    init_parseTree(); // Have to do this every time because reasons
    
	result = asn1_create_element(definitions, "ApplicationList.Gossip", &structure);
	string sha256;
	
	timestampAndHash (gossip, timestamp, sha256);

	if ((result = asn1_write_value(structure, "sha256hash", sha256.c_str(),
	        strlen (sha256.c_str()))) != ASN1_SUCCESS) {
	    asn1_perror (result);
	}

	if ((result = asn1_write_value(structure, "timestamp", timestamp.c_str(),
	        1)) != ASN1_SUCCESS) {
	    asn1_perror (result);
	}
	
	if ((result = asn1_write_value(structure, "message", gossip.c_str(),
	        strlen (gossip.c_str()))) != ASN1_SUCCESS) {
	    asn1_perror (result);
	}

	result = asn1_der_coding (structure, "", dataBuff, &size, errorDescription);
	
	asn1_delete_structure (&structure);
	asn1_delete_structure (&definitions);

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
}


