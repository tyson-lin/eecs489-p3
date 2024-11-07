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

unsigned int expected_seq_num = 0;
bool currently_recieving = false;

ofstream logfile;

string outfile_name;
ofstream outfile;

struct Packet {
    PacketHeader header;
    char data[PACKET_SIZE];
};
vector<Packet> outstanding_packets;

struct sockaddr_in server_addr, client_addr;
socklen_t client_addr_len = sizeof(client_addr);

unsigned int connection_count = 0;

// server
void receiver(int port_num, int window_size, string output_dir, string log_filename) {

    logfile = ofstream(log_filename);

    // create socket and bind to port
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available interface
    server_addr.sin_port = htons(port_num); // Port number
    bind(server_fd, (sockaddr*) &server_addr, sizeof(server_addr));

    char buffer[PACKET_SIZE];

    fd_set rfds;
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(server_fd, &rfds);
        select(server_fd + 1, &rfds, NULL, NULL, NULL);
        if (!FD_ISSET(server_fd, &rfds)){
            continue;
        }
        // read packet
        PacketHeader header;
        bool success = recv_packet(server_fd, &client_addr, header, logfile, buffer);
        if (!success){
            continue;
        }

        // Receive START command and send ACK
        if (currently_recieving == false) {
            if (header.type == TYPE_START) {
                // We are now receiving
                currently_recieving = true;
                expected_seq_num = 0;

                // Setup output file
                outfile_name = output_dir + "/FILE-" + to_string(connection_count) + ".out";
                cout << "Output file location: " << outfile_name << endl;
                outfile.open(outfile_name);
                outfile.close();

                // Send ACK
                PacketHeader start_response;
                start_response.type = TYPE_ACK;
                start_response.seqNum = header.seqNum;
                start_response.length = 0;
                send_packet(server_fd, client_addr, start_response, logfile);
            } 
        } else if (currently_recieving == true) {
            if (header.type == TYPE_START) {
                continue;
            } 
            if (header.type == TYPE_DATA) {
                cout << "Expected seq num: " << expected_seq_num << endl;
                //cout << header.seqNum << " " <<  (unsigned int)expected_seq_num << " " << (unsigned int)window_size << endl;
                // Packet is the next desired packet
                if ((header.seqNum) == (unsigned int)expected_seq_num) {
                    // TODO: PRINT BUFFER TO FILE
                    expected_seq_num++;
                    cout << "Printing packet " << header.seqNum << endl;
                    outfile.open(outfile_name, ios::app);
                    if (outfile.is_open()) {
                        outfile << buffer;
                    }
                    outfile.close();
                    // ======================================================================
                    // sort through outstanding packets to check if we have anything of note
                    while (true) {
                        bool packet_found = false;
                        for (size_t i = 0; i < outstanding_packets.size(); i++) {
                            if (outstanding_packets[i].header.seqNum == expected_seq_num) {
                                //cout << "removing packet " << outstanding_packets[i].header.seqNum << endl;
                                expected_seq_num++;
                                packet_found = true;
                                cout << "Printing packet " << outstanding_packets[i].header.seqNum << endl;
                                outfile.open(outfile_name,ios::app);
                                if (outfile.is_open()) {
                                    outfile << outstanding_packets[i].data;
                                } 
                                outfile.close();
                                outstanding_packets.erase(outstanding_packets.begin()+i);
                                // ======================================================================
                                break;
                            }
                        }

                        // no interesting packets in outstanding set
                        if (packet_found == false) {
                            break;
                        }
                    }
                    PacketHeader ack_header = {TYPE_ACK, header.seqNum, 0, 0};
                    send_packet(server_fd, client_addr, ack_header, logfile);
                } else 
                // Packet within range
                if (header.seqNum < (unsigned int)expected_seq_num + (unsigned int)window_size) {
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
                            //cout << "already outstanding" << endl;
                            packet_found = true;
                        }
                    }
                    if (packet_found == false) {
                        //cout << "adding packet " << packet.header.seqNum << endl;
                        outstanding_packets.push_back(packet);
                    } 
                    PacketHeader ack_header = {TYPE_ACK, header.seqNum, 0, 0};
                    send_packet(server_fd, client_addr, ack_header, logfile);
                }
            } 
            if (header.type == TYPE_END) {

                // generate end response
                PacketHeader end_response;
                end_response.type = TYPE_ACK;
                end_response.seqNum = header.seqNum;
                end_response.length = 0;
                send_packet(server_fd, client_addr, end_response, logfile);

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
         
        cout << "==================================" << endl << endl;
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