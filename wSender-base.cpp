#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <fcntl.h>
#include <cassert>
#include <chrono>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "PacketHeader.h"
#include "crc32.h"

using namespace std;

#define MAX_PACKET_SIZE 1472
#define HEADER_SIZE 16
#define DATA_SIZE MAX_PACKET_SIZE-HEADER_SIZE

struct sockaddr_in server_addr;

ofstream logfile;

void send_packet(int client_fd, PacketHeader header, const char* data = ""){
    header.checksum = crc32(data, header.length);
    sendto(client_fd,&header.type, 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    sendto(client_fd,&header.seqNum, 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    sendto(client_fd,&header.length, 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    sendto(client_fd,&header.checksum, 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (header.length > 0){
        sendto(client_fd,data, header.length, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    }
    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
}
char* recv_packet(int client_fd, PacketHeader& header){
    char data[DATA_SIZE];
    socklen_t len = sizeof(server_addr);
    recvfrom(client_fd,&header.type, 4, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    recvfrom(client_fd,&header.seqNum, 4, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    recvfrom(client_fd,&header.length, 4, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    recvfrom(client_fd,&header.checksum, 4, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    if (header.length > 0){
        recvfrom(client_fd,data, header.length, MSG_WAITALL,(sockaddr*)&server_addr, &len);
    }
    logfile << header.type << " " << header.seqNum << " " << header.length << " " << header.checksum << endl;
    return data;
}

void sender(string r_ip, int r_port, int window_size, string input, string log_filename){
    logfile = ofstream(log_filename);

    int fd = open(input.c_str(), 0);

    string s = "";
    char buf;
    while (true){
        int bytes = read(fd, &buf, 1);
        if (bytes < 1 || buf == '\0'){
            break;
        }
        s += buf;
    }
    
    //cout << "buffer = \n" << s << '\n';
    int num_packets = (int)ceil((double) s.size() / (DATA_SIZE));
    cout << s.size() << " bytes to send, " << num_packets << " packets" << endl;

    int client_fd  = socket(AF_INET, SOCK_DGRAM, 0);
    const int enable = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // Make socket address
    server_addr.sin_family = AF_INET;              // IPv4
    server_addr.sin_port = htons(r_port);            // Port number (convert to network byte order)
    server_addr.sin_addr.s_addr = inet_addr(r_ip.c_str());  // Server IP address

    // Connect to server
    cout << "connected to " << r_ip << endl;
    PacketHeader header;
    header.type = 0;
    header.length = 0;
    send_packet(client_fd, header);

    cout << "start sent" << endl;

    recv_packet(client_fd, header);

    cout << "ack recved" << endl;
    
    int highest_ack = -1;
    int seq_num = 0;
    fd_set rfds;
    int curr_index = 0;
    while (highest_ack != num_packets - 1) {
        for (int i = seq_num; i < seq_num + window_size && i < num_packets; ++i){
            string data;
            header.type = 2;
            header.seqNum = i;
            header.length = min(DATA_SIZE, (int)s.size() - curr_index);
            data = s.substr(curr_index, header.length);
            curr_index += header.length;
            header.checksum = crc32(data.c_str(),header.length);
            send_packet(client_fd, header, data.c_str());
        }
        for (auto start = std::chrono::steady_clock::now(), now = start; now < start + std::chrono::milliseconds{500} || highest_ack < seq_num + 19; now = std::chrono::steady_clock::now()){
            FD_ZERO(&rfds);
            FD_SET(client_fd, &rfds);
            int activity = select(client_fd + 1, &rfds, NULL, NULL, NULL);
            if (FD_ISSET(client_fd, &rfds)){
                start = std::chrono::steady_clock::now();
                string data = recv_packet(client_fd, header);
                if (header.type == 3){
                    cout << header.seqNum << endl;
                    highest_ack = header.seqNum;
                }
            }
        } 
        if (highest_ack > seq_num){
            seq_num = highest_ack + 1;
        } 
    }
    header.type = 1;
    header.seqNum = 0;
    header.length = 0;
    send_packet(client_fd, header);
    recv_packet(client_fd, header);
    close(client_fd);
}

int main (int argc, char* argv[]){
    sender(argv[1], atoi(argv[2]), atoi(argv[3]), argv[4], argv[5]);
}