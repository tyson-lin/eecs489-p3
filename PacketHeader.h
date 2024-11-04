#ifndef __PACKET_HEADER_H__
#define __PACKET_HEADER_H__

#include <sys/socket.h>
#include <iostream>
#include <fstream>

using namespace std;

#define TYPE_START 0
#define TYPE_END 1
#define TYPE_DATA 2
#define TYPE_ACK 3

struct PacketHeader
{
    unsigned int type;     // 0: START; 1: END; 2: DATA; 3: ACK
    unsigned int seqNum;   // Described below
    unsigned int length;   // Length of data; 0 for ACK packets
    unsigned int checksum; // 32-bit CRC
};

void send_packet(int client_fd, PacketHeader header, ofstream& logfile, const char* data = ""){
    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    cout << "Sending " << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    header.checksum = crc32(data, header.length);

    unsigned int host_order_length = header.length;

    header.type = htonl(header.type);
    header.seqNum = htonl(header.seqNum);
    header.length = htonl(header.length);
    header.checksum = htonl(header.checksum);

    sendto(client_fd,&header.type, 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    sendto(client_fd,&header.seqNum, 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    sendto(client_fd,&header.length, 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    sendto(client_fd,&header.checksum, 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (header.length > 0){
        sendto(client_fd,data, host_order_length, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    }
}

void recv_packet(int client_fd, PacketHeader& header, ofstream& logfile, char* data){
    socklen_t len = sizeof(server_addr);
    recvfrom(client_fd,&header.type, 4, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    recvfrom(client_fd,&header.seqNum, 4, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    recvfrom(client_fd,&header.length, 4, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    recvfrom(client_fd,&header.checksum, 4, MSG_WAITALL,(sockaddr*)&server_addr, &len);

    header.type = ntohl(header.type);
    header.seqNum = ntohl(header.seqNum);
    header.length = ntohl(header.length);
    header.checksum = ntohl(header.checksum);

    if (header.length > 0){
        recvfrom(client_fd,data, header.length, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    }
    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    cout << "Receiving " << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
}

#endif
