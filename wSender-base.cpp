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

using namespace std;

struct sockaddr_in server_addr, client_addr;

ofstream logfile;

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
    server_addr.sin_addr.s_addr = inet_addr(r_ip.c_str());  // Server IP address
    server_addr.sin_port = htons(r_port);            // Port number (convert to network byte order)
    //bind(server_fd, (sockaddr*) &server_addr, sizeof(server_addr));

    // Connect to server
    cout << "connected to " << r_ip << endl;
    PacketHeader header;
    header.type = 0;
    header.length = 0;
    send_packet(client_fd, server_addr, header, logfile);

    cout << "start sent" << endl;

    char data[DATA_SIZE];
    recv_packet(client_fd, &server_addr, &header, logfile, data);

    cout << "ack recved" << endl;
    
    int highest_ack = -1;
    int seq_num = 0;
    fd_set rfds;
    int curr_index = 0;
    while (highest_ack != num_packets - 1) {
        int w_size = min(window_size, num_packets - seq_num);
        for (int i = seq_num; i < seq_num + w_size; ++i){
            string data;
            header.type = 2;
            header.seqNum = i;
            header.length = min(DATA_SIZE, (int)s.size() - curr_index);
            data = s.substr(curr_index, header.length);
            curr_index += header.length;
            send_packet(client_fd, server_addr, header, logfile, data.c_str());
        }
        for (auto start = std::chrono::steady_clock::now(), now = start; now < start + std::chrono::milliseconds{500} && highest_ack < seq_num + w_size - 1; now = std::chrono::steady_clock::now()){
            //cout << "Here 1" << endl;
            FD_ZERO(&rfds);
            FD_SET(client_fd, &rfds);
            int activity = select(client_fd + 1, &rfds, NULL, NULL, NULL);
            if (FD_ISSET(client_fd, &rfds)){
                start = std::chrono::steady_clock::now();
                cout << "Here 2" << endl;
                recv_packet(client_fd, &server_addr, &header, logfile, data);
                cout << "Here 3" << endl;
                if (header.type == 3){
                    cout << header.seqNum << endl;
                    highest_ack = header.seqNum;
                }
            }
            auto start_printable = chrono::duration_cast<chrono::milliseconds>(start);
            auto now_printable = chrono::duration_cast<chrono::milliseconds>(now);
            cout << "Timer: " << start_printable << " " << now_printable << endl;
        } 
        cout << "Here 4" << endl;
        if (highest_ack > seq_num){
            seq_num = highest_ack + 1;
        } 
        cout << highest_ack << " " << seq_num << " " << num_packets << endl;
    }
    header.type = 1;
    header.seqNum = 0;
    header.length = 0;
    send_packet(client_fd, server_addr, header, logfile);
    recv_packet(client_fd, &server_addr, &header, logfile, data);
    close(client_fd);
}

int main (int argc, char* argv[]){
    sender(argv[1], atoi(argv[2]), atoi(argv[3]), argv[4], argv[5]);
}