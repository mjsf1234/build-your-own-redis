#include <bits/stdc++.h>
#include "us_xfr.h"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "conn.cpp"

#define BACKLOG 5
#define PORT 8080


int setup_listening_socket(){

    // declare all the variables    
    struct sockaddr_in address;
    int server_fd, cleint_fd;

    /*
    * create a Unix domain socket
    * AF_UNIX is the address family for Unix domain sockets
    * SOCK_STREAM is the type of socket, indicating a stream socket
    */      
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /*
    * Initialize the address structure with zeros
    * sun_family is the address family, which is AF_UNIX
    * sun_path is the path to the INADDR_LOOPBACK
    * The sin_port and sin_addr fields
    * are the port number and the IP address, both in network byte order
    */
    memset(&address,0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT); // convert port number to network byte order
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // use loopback address for local communication
    
    int val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    /*
    * bind the socket to the address
    * The bind function associates the socket with the specified address
    */      
    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return EXIT_FAILURE;
    }

    /*
    * listen for incoming connections
    * The backlog parameter specifies the maximum length of the queue of pending connections
    */
    if(listen(server_fd,BACKLOG) < 0){
        perror("listen failed");
        return EXIT_FAILURE;
    }
    return server_fd;

}

void readFromClient(int cleint_fd, char*buffer) {
    sleep(5);
    int numRead; // read 256 char from the client and store it in buffer 
    char* arr;
    while ((numRead = read(cleint_fd, buffer, BUF_SIZE)) > 0) {
        printf("Received message: %.*s\n", (int)numRead, buffer);
    
    }
    char wbuf[] = "world";
    write(cleint_fd, wbuf, strlen(wbuf));
}

int main(int argc , char* argv[]) {
    ssize_t numRead;    
    char buf[BUF_SIZE]; 
    int cleint_fd;
    int server_fd = setup_listening_socket();

    /*
    * accept the connection and create a new socket for the connection
    */
    while(true){
        cleint_fd = accept(server_fd, nullptr,0); // this is the blocking call 
        if (cleint_fd < 0) {
            perror("accept failed");
            return EXIT_FAILURE;
        } 
        printf("Accepted connection from client\n");
        auto start = std::chrono::high_resolution_clock::now();
        readFromClient(cleint_fd, buf);
        close(cleint_fd);
        auto end = std::chrono::high_resolution_clock::now();
       
    }
    return 0;
}
