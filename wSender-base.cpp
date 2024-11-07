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
#include <fcntl.h>
#include <cassert>
#include <chrono>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "PacketHeader.h"
#include "crc32.h"
#include <sys/time.h>
#include <ctime>

using namespace std;

struct sockaddr_in server_addr, client_addr;

unsigned int start_seq_num = 0;

ofstream logfile;

int send_data_packet(int client_fd, int seqNum, string& s, int index){
    string data;
    PacketHeader header;
    header.type = 2;
    header.seqNum = seqNum;
    header.length = min(DATA_SIZE, (int)s.size() - index);
    cout << "Size: " << (int)s.size() - index << endl;
    data = s.substr(index, header.length);
    index += header.length;
    send_packet(client_fd, server_addr, header, logfile, data.c_str());
    return index;
}

void sender(string r_ip, int r_port, unsigned int window_size, string input, string log_filename){
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
    unsigned int num_packets = (int)ceil((double) s.size() / (DATA_SIZE));
    cout << s.size() << " bytes to send, " << num_packets << " packets" << endl;
    vector<int> start_indices(num_packets, 0); // Initialize vector of size n with all elements as -1
    for (unsigned int i = 1; i < num_packets; i++) {
        start_indices[i] = start_indices[i-1] + DATA_SIZE;
    }

    int client_fd  = socket(AF_INET, SOCK_DGRAM, 0);
    const int enable = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // Make socket address
    server_addr.sin_family = AF_INET;              // IPv4
    server_addr.sin_addr.s_addr = inet_addr(r_ip.c_str());  // Server IP address
    server_addr.sin_port = htons(r_port);            // Port number (convert to network byte order)
    //bind(server_fd, (sockaddr*) &server_addr, sizeof(server_addr));

    // Connect to server
    cout << "connected to " << r_ip << endl;
    PacketHeader header;
    header.type = 0;
    header.length = 0;
    srand((int)time(0));
    start_seq_num = rand();
    header.seqNum = start_seq_num;
    send_packet(client_fd, server_addr, header, logfile);

    cout << "start sent" << endl;

    char data[DATA_SIZE];
    recv_packet(client_fd, &server_addr, header, logfile, data);

    cout << "ack recved" << endl;
    
    unsigned int highest_ack = 0;
    unsigned int seq_num = 0;
    fd_set rfds;

    unsigned int window_start = 0;
    unsigned int curr_window_end = min(window_size, num_packets);
    unsigned int last_window_end = curr_window_end;

    bool window_moved_forward = false;

    while (highest_ack != num_packets) {
        
        curr_window_end = min(window_start + window_size, num_packets);
        if (window_moved_forward == true) {
            // transmit packets in window that are not in flight
            for (unsigned int i = last_window_end; i < curr_window_end; i++) {
                send_data_packet(client_fd, i, s, start_indices[i]);
            }
        }
        if (window_moved_forward == false) {
            for (unsigned int i = window_start; i < curr_window_end; i++) {
                send_data_packet(client_fd, i, s, start_indices[i]);
            }
        }
        last_window_end = curr_window_end;

        window_moved_forward = false;
        auto start = std::chrono::steady_clock::now();
        auto now = start;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        while (duration.count() < 500 && highest_ack < curr_window_end) {
            cout << "Highest ack: " << highest_ack << endl;
            cout << "Window end: " << curr_window_end << endl;
            FD_ZERO(&rfds);
            FD_SET(client_fd, &rfds);
            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000;
            select(client_fd + 1, &rfds, NULL, NULL, &timeout);
            if (FD_ISSET(client_fd, &rfds)){
                recv_packet(client_fd, &server_addr, header, logfile, data);
                if (header.type == 3 && header.seqNum > highest_ack){
                    highest_ack = header.seqNum;
                    window_moved_forward = true;
                }
            }
            now = std::chrono::steady_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        } 
    }
    header.type = 1;
    header.seqNum = start_seq_num;
    header.length = 0;
    send_packet(client_fd, server_addr, header, logfile);
    recv_packet(client_fd, &server_addr, header, logfile, data);
    close(client_fd);
}

int main (int argc, char* argv[]){
    sender(argv[1], atoi(argv[2]), atoi(argv[3]), argv[4], argv[5]);
}