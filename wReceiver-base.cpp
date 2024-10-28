#include <iostream>
#include <fstream>
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
#include "PacketHeader.h"

using namespace std;

#define MAX_PACKET_SIZE 1472

int header_size = sizeof(PacketHeader);

int expected_seq_num = 0;
bool currently_recieving = false;

void send_packet(int server_fd, PacketHeader header, char* data = ""){
    send(server_fd,&header.type, 4, 0);
    send(server_fd,&header.seqNum, 4, 0);
    send(server_fd,&header.length, 4, 0);
    send(server_fd,&header.checksum, 4, 0);
    if (header.length > 0){
        send(server_fd, data, header.length, 0);
    }
}

char* recv_packet(int server_fd, PacketHeader& header){
    char data[MAX_PACKET_SIZE - header_size];
    recv(server_fd,&header.type, 4, MSG_WAITALL);
    recv(server_fd,&header.seqNum, 4, MSG_WAITALL);
    recv(server_fd,&header.length, 4, MSG_WAITALL);
    recv(server_fd,&header.checksum, 4, MSG_WAITALL);
    if (header.length > 0){
        recv(server_fd, data, header.length, MSG_WAITALL);
    }
    return data;
}


// server
void receiver(int port_num, int window_size, string output_dir, string log_filename) {

    char buffer[MAX_PACKET_SIZE];

    // create socket and bind to port
    int server_fd = socket(PF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available interface
    server_addr.sin_port = htons(port_num); // Port number
    
    // open log file
    ofstream logfile(log_filename);

    while (1) {
        // read packet
        PacketHeader header;
        buffer = recv_packet(server_fd, header);
        // check crc
        if (crc32(buffer, sizeof(buffer)) != header.checksum) {
            // dropped packet
            continue;
        }

        // Receive START command and send ACK
        if (currently_receiving == false) {
            if (header.type = TYPE_START) {
                PacketHeader start_response;
                start_response.type = TYPE_ACK;
                start_response.seqNum = header.seqNum;
                start_response.length = 0;
                send_packet(server_fd, start_response);
                currently_recieving == true;
                expected_seq_num  = 0;
            } 
        } 
        if (currently_recieving == true) {
            
            if (header.type == TYPE_START) {
                continue;
            } 
            if (header.type == TYPE_DATA) {
                if (header.seqNum = expected_seq_num) {
                    
                } else {
                    // didn't get expected, so send ack for expected seq num
                    PacketHeader ac = {TYPE_ACK, expected_seq_num, 0, 0};
                    send_packet(server_fd, ack_);
                }
            } 
            if (header.type == TYPE_END) {
                PacketHeader end_response;
                end_response.type = TYPE_ACK;
                end_response.seqNum = header.seqNum;
                end_response.length = 0;
                send_packet(server_fd, end_response);
            } 
            if (header.type == TYPE_ACK) {
                continue;
            } 
        }
    }
}

// ./wReceiver <port-num> <window-size> <output-dir> <log>
int main (int argc, char* argv[]) {
    if (argc != 5) {
        cout << "Usage: ./wReceiver <port-num> <window-size> <output-dir> <log>\n";
        return 1;
    }
    receiver(atoi(argv[1]), atoi(argv[2]), argv[3], argv[4]);
}