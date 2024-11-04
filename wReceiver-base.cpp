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
#include <vector>

using namespace std;

#define MAX_PACKET_SIZE 1472
#define HEADER_SIZE 16
#define DATA_SIZE MAX_PACKET_SIZE-HEADER_SIZE

int expected_seq_num = 0;
bool currently_recieving = false;

ofstream logfile;

string outfile_name;
ofstream outfile;

struct Packet {
    PacketHeader header;
    char data[MAX_PACKET_SIZE];
};
vector<Packet> outstanding_packets;

struct sockaddr_in server_addr, client_addr;
socklen_t client_addr_len = sizeof(client_addr);

unsigned int connection_count = 0;

void send_packet(int server_fd, PacketHeader header, char* data = ""){
    header.checksum = crc32(data, header.length);
    sendto(server_fd,&header.type, 4, 0, (sockaddr*)&client_addr, sizeof(client_addr));
    sendto(server_fd,&header.seqNum, 4, 0, (sockaddr*)&client_addr, sizeof(client_addr));
    sendto(server_fd,&header.length, 4, 0, (sockaddr*)&client_addr, sizeof(client_addr));
    sendto(server_fd,&header.checksum, 4, 0, (sockaddr*)&client_addr, sizeof(client_addr));
    if (header.length > 0){
        sendto(server_fd, data, header.length, 0, (sockaddr*)&client_addr, sizeof(client_addr));
    }
    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    cout << "Sending " << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
}

void recv_packet(int server_fd, PacketHeader& header, char* data){
    socklen_t len = sizeof(server_addr);
    recvfrom(server_fd,&header.type, 4, MSG_WAITALL,(sockaddr*)&client_addr, &len);
    recvfrom(server_fd,&header.seqNum, 4, MSG_WAITALL,(sockaddr*)&client_addr, &len);
    recvfrom(server_fd,&header.length, 4, MSG_WAITALL,(sockaddr*)&client_addr, &len);
    recvfrom(server_fd,&header.checksum, 4, MSG_WAITALL,(sockaddr*)&client_addr, &len);
    if (header.length > 0){
        recvfrom(server_fd, data, header.length, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    }
    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    cout << "Receiving " << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
}


// server
void receiver(int port_num, int window_size, string output_dir, string log_filename) {

    logfile = ofstream(log_filename);

    // create socket and bind to port
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available interface
    server_addr.sin_port = htons(port_num); // Port number

    bind(server_fd, (sockaddr*) &server_addr, sizeof(server_addr));

    char buffer[MAX_PACKET_SIZE];


    while (1) {
        // read packet
        PacketHeader header;
        recv_packet(server_fd, header, buffer);

        // Receive START command and send ACK
        if (currently_recieving == false) {
            if (header.type == TYPE_START) {
                // We are now receiving
                currently_recieving = true;
                expected_seq_num = 0;

                // Setup output file
                outfile_name = output_dir + "File-" + to_string(connection_count) + ".out";
                cout << "Output file location: " << outfile_name << endl;

                // Send ACK
                PacketHeader start_response;
                start_response.type = TYPE_ACK;
                start_response.seqNum = header.seqNum;
                start_response.length = 0;
                send_packet(server_fd, start_response);


            } 
        } else if (currently_recieving == true) {
            if (header.type == TYPE_START) {
                continue;
            } 
            if (header.type == TYPE_DATA) {
                cout << "Here" << endl;
                cout << "Length: " << header.length << endl;
                // check crc because it's a data packet
                if (crc32(buffer, header.length) != header.checksum) {
                    continue;
                }
                cout << "Sequence Number: " << header.seqNum << endl;
                // Packet is the next desired packet
                if ((header.seqNum) == (unsigned int)expected_seq_num) {
                    // TODO: PRINT BUFFER TO FILE
                    cout << buffer << endl;
                    outfile.open(outfile_name);
                    if (outfile.is_open()) {
                        cout << "Printing to " << outfile_name << endl;
                        outfile << buffer;
                    } else {
                        cout << "Failed to open " << outfile_name << endl;
                    }
                    outfile.close();
                    // ======================================================================
                    // sort through outstanding packets to check if we have anything of note
                    while (true) {
                        bool packet_found = false;
                        for (size_t i = 0; i < outstanding_packets.size(); i++) {
                            if (outstanding_packets[i].header.seqNum == expected_seq_num) {
                                expected_seq_num++;
                                packet_found = true;
                                outstanding_packets.erase(outstanding_packets.begin()+i);
                                // TODO: PRINT BUFFER TO FILE
                                cout << buffer << endl;
                                outfile.open(outfile_name);
                                if (outfile.is_open()) {
                                    cout << "Printing to " << outfile_name << endl;
                                    outfile << buffer;
                                } else {
                                    cout << "Failed to open " << outfile_name << endl;
                                }
                                outfile.close();
                                // ======================================================================
                                cout << "Here1" << endl;
                                break;
                            }
                        }

                        // no interesting packets in outstanding set
                        if (packet_found == false) {
                            cout << "Here2" << endl;
                            break;
                        }
                    }
                    cout << "Here3" << endl;
                    PacketHeader ack_header = {TYPE_ACK, (unsigned int)expected_seq_num, 0, 0};
                    expected_seq_num++;
                    send_packet(server_fd, ack_header);
                } else 
                // Packet within range
                if (header.seqNum >= (unsigned int)expected_seq_num + (unsigned int)window_size) {
                    Packet packet;
                    packet.header = header;
                    for (unsigned int i = 0; i < header.length; i++) {
                        packet.data[i] = buffer[i];
                    }

                    // Add packet to list of outstanding packets ONLY if it
                    // it's not already there
                    bool packet_found = false;
                    for (size_t i = 0; i < outstanding_packets.size(); i++) {
                        if (outstanding_packets[i].header.seqNum == header.seqNum) {
                            packet_found = true;
                        }
                    }
                    if (packet_found == false) {
                        outstanding_packets.push_back(packet);
                    } 
                    PacketHeader ack_header = {TYPE_ACK, (unsigned int)expected_seq_num, 0, 0};
                    send_packet(server_fd, ack_header);
                }
                else {
                    // didn't get expected, so send ack for expected seq num
                    PacketHeader ack_header = {TYPE_ACK, (unsigned int)expected_seq_num, 0, 0};
                    send_packet(server_fd, ack_header);
                }
            } 
            if (header.type == TYPE_END) {

                // generate end response
                PacketHeader end_response;
                end_response.type = TYPE_ACK;
                end_response.seqNum = header.seqNum;
                end_response.length = 0;
                send_packet(server_fd, end_response);

                // set internal state
                currently_recieving = false;

                // increase connection count
                connection_count++;
            } 
            if (header.type == TYPE_ACK) {
                // shouldn't happen
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