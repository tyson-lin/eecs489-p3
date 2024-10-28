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

int packet_size = 1472;
int header_size = sizeof(PacketHeader);

void send_packet(int client_fd, PacketHeader header, char* data = ""){
    send(client_fd,&header.type, 4, 0);
    send(client_fd,&header.seqNum, 4, 0);
    send(client_fd,&header.length, 4, 0);
    send(client_fd,&header.checksum, 4, 0);
    if (header.length > 0){
        send(client_fd,data, header.length, 0);
    }
}
char* recv_packet(int client_fd, PacketHeader& header){
    char data[packet_size - header_size];
    recv(client_fd,&header.type, 4, MSG_WAITALL);
    recv(client_fd,&header.seqNum, 4, MSG_WAITALL);
    recv(client_fd,&header.length, 4, MSG_WAITALL);
    recv(client_fd,&header.checksum, 4, MSG_WAITALL);
    if (header.length > 0){
        recv(client_fd,data, header.length, MSG_WAITALL);
    }
    return data;
}

void sender(string r_ip, int r_port, int window_size, string input, string log){
    std::ifstream f(input, ifstream::binary);
    int size = 0;        
    f.read(reinterpret_cast<char *>(&size), sizeof(size));
    
    // Allocate a string, make it large enough to hold the input
    string s;
    s.resize(size);
    
    // read the text into the string
    f.read(&s[0], s.size());
    f.close();    
    
    //cout << "buffer = \n" << s << '\n';
    int num_packets = (int)ceil((double) s.size() / (packet_size - header_size));
    cout << s.size() << " bytes to send, " << num_packets << " packets" << endl;
   
    cout << s << endl;



    int client_fd  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    const int enable = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // Make socket address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;              // IPv4
    server_addr.sin_port = htons(r_port);            // Port number (convert to network byte order)
    server_addr.sin_addr.s_addr = inet_addr(r_ip.c_str());  // Server IP address

    // Connect to server
    /*
    int success = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (!success){
        exit(1);
    }
    cout << "connected to " << r_ip << endl;
    PacketHeader header;
    header.type = 0;
    header.length = 0;
    send_packet(client_fd, header);
    
    int highest_ack = -1;
    int seq_num = 0;
    while (highest_ack != num_packets - 1){
        for (int i = seq_num; i < seq_num + window_size && i < num_packets; ++i){
            
        }
    }
    */
}

int main (int argc, char* argv[]){
    sender(argv[1], atoi(argv[2]), atoi(argv[3]), argv[4], argv[5]);
}