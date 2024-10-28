#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "crc32.h"

using namespace std;

#define MAX_FRAME_SIZE 1500

struct PacketHeader {
    unsigned int type;     // 0: START; 1: END; 2: DATA; 3: ACK
    unsigned int seqNum;   // Sequence number
    unsigned int length;   // Length of data
    unsigned int checksum; // 32-bit CRC
};

// server
//
void receiver(int port_num, int window_size, string output_dir, string log_filename) {

    char buffer[MAX_FRAME_SIZE]
    int server_fd; 
    struct sockaddr_in server_addr;

    // create socket
    int server_fd = socket(PF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available interface
    server_addr.sin_port = htons(port_num); // Port number

    
    // ...

    // 
}

// ./wReceiver <port-num> <window-size> <output-dir> <log>
int main (int argc, char* argv[]) {
    if (argc != 5) {
        cout << "Usage: ./wReceiver <port-num> <window-size> <output-dir> <log>\n";
        return 0;
    }
    receiver(atoi(argv[1]), atoi(argv[2]), argv[3], argv[4]);
}