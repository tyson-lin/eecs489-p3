#ifndef __PACKET_HEADER_H__
#define __PACKET_HEADER_H__

#include <sys/socket.h>
#include <iostream>
#include <fstream>
#include <cstring>

#include "crc32.h"

using namespace std;

#define PACKET_SIZE 1472
#define HEADER_SIZE 16
#define DATA_SIZE   PACKET_SIZE-HEADER_SIZE

#define TYPE_START 0
#define TYPE_END 1
#define TYPE_DATA 2
#define TYPE_ACK 3

struct PacketHeader {
    unsigned int type;     // 0: START; 1: END; 2: DATA; 3: ACK
    unsigned int seqNum;   // Described below
    unsigned int length;   // Length of data; 0 for ACK packets
    unsigned int checksum; // 32-bit CRC
};

void send_packet(int client_fd, sockaddr_in addr, PacketHeader header, ofstream& logfile, const char* data = ""){
    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    cout << "Sending " << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;

    header.checksum = crc32(data, header.length);

    unsigned int host_order_length = header.length;

    header.type = htonl(header.type);
    header.seqNum = htonl(header.seqNum);
    header.length = htonl(header.length);
    header.checksum = htonl(header.checksum);

    char send_data[PACKET_SIZE];
    memcpy(send_data, &header, HEADER_SIZE);
    memcpy(send_data+HEADER_SIZE, data, DATA_SIZE);
    cout << send_data << endl << endl << endl;
    sendto(client_fd, send_data, PACKET_SIZE, 0, (sockaddr*)&addr, sizeof(addr));
    

    // sendto(client_fd,&header.type, 4, 0, (sockaddr*)&addr, sizeof(addr));
    // sendto(client_fd,&header.seqNum, 4, 0, (sockaddr*)&addr, sizeof(addr));
    // sendto(client_fd,&header.length, 4, 0, (sockaddr*)&addr, sizeof(addr));
    // sendto(client_fd,&header.checksum, 4, 0, (sockaddr*)&addr, sizeof(addr));
    // if (header.length > 0){
    //     sendto(client_fd,data, host_order_length, 0, (sockaddr*)&addr, sizeof(addr));
    // }
}

bool recv_packet(int client_fd, sockaddr_in * addr, PacketHeader * header, ofstream& logfile, char* data){
    socklen_t len = sizeof(*addr);

    char recv_data[PACKET_SIZE];
    recvfrom(client_fd, recv_data, PACKET_SIZE, MSG_WAITALL, (sockaddr*)addr, &len);

    // recvfrom(client_fd,&header.type, 4, MSG_WAITALL,(sockaddr*)addr, &len);
    // recvfrom(client_fd,&header.seqNum, 4, MSG_WAITALL,(sockaddr*)addr, &len);
    // recvfrom(client_fd,&header.length, 4, MSG_WAITALL,(sockaddr*)addr, &len);
    // recvfrom(client_fd,&header.checksum, 4, MSG_WAITALL,(sockaddr*)addr, &len);
    cout << recv_data << endl << endl << endl;
    memcpy(header, recv_data, HEADER_SIZE);
    memcpy(data, recv_data+HEADER_SIZE, DATA_SIZE);
    

    header->type = ntohl(header->type);
    header->seqNum = ntohl(header->seqNum);
    header->length = ntohl(header->length);
    header->checksum = ntohl(header->checksum);

    logfile << header->type << " " << header->seqNum << " " << header->length << " " << header->checksum << endl;
    cout << "Receiving " << header->type << " " << header->seqNum << " " << header->length << " " << header->checksum << endl;

    if (crc32(data, header->length) != header->checksum) {
        return false;
    }
    return true;
}

#endif
