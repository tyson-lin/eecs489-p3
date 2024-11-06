#ifndef __PACKET_HEADER_H__
#define __PACKET_HEADER_H__

#include <sys/socket.h>
#include <iostream>
#include <fstream>
#include <cstring>

#include "crc32.h"

using namespace std;

#define TYPE_START 0
#define TYPE_END 1
#define TYPE_DATA 2
#define TYPE_ACK 3

#define MAX_PACKET_SIZE 1472
#define HEADER_SIZE 16

struct PacketHeader
{
    unsigned int type;     // 0: START; 1: END; 2: DATA; 3: ACK
    unsigned int seqNum;   // Described below
    unsigned int length;   // Length of data; 0 for ACK packets
    unsigned int checksum; // 32-bit CRC
};

void int_to_byte_array(int n, unsigned char* message){
    message[0] = (n >> 24) & 0xFF;
    message[1] = (n >> 16) & 0xFF;
    message[2] = (n >> 8) & 0xFF;
    message[3] = (n >> 0) & 0xFF;
}

void send_packet(int client_fd, sockaddr_in addr, PacketHeader header, ofstream& logfile, const char* data = ""){
    header.checksum = crc32(data, header.length);

    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    cout << "Sending " << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    unsigned int host_order_length = header.length;

    header.type = htonl(header.type);
    header.seqNum = htonl(header.seqNum);
    header.length = htonl(header.length);
    header.checksum = htonl(header.checksum);

    unsigned char message[MAX_PACKET_SIZE];
    int_to_byte_array(header.type, message);
    int_to_byte_array(header.seqNum, message + 4);
    int_to_byte_array(header.length, message + 8);
    int_to_byte_array(header.checksum, message + 12);
    memcpy(message+16, data, host_order_length);
    sendto(client_fd,message, MAX_PACKET_SIZE, 0, (sockaddr*)&addr, sizeof(addr));
}

void recv_packet(int client_fd, sockaddr_in *addr, PacketHeader& header, ofstream& logfile, char* data){
    socklen_t len = sizeof(*addr);
    char message[MAX_PACKET_SIZE];
    int size = recvfrom(client_fd, message, MAX_PACKET_SIZE, 0,(sockaddr*)addr, &len);
    strcpy(data, message+16);
    header.type = *static_cast<int *>(static_cast<void*>(message));
    header.seqNum = *static_cast<int *>(static_cast<void*>(message + 4));
    header.length = *static_cast<int *>(static_cast<void*>(message + 8));
    header.checksum = *static_cast<int *>(static_cast<void*>(message + 12));
    data[header.length] = '\0';

    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    cout << "Receiving " << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
}

#endif
