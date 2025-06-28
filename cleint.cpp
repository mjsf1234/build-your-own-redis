#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 8080
#define BUF_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    // 1. Create a socket with the ipv4 address family and tcp type
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // 2. Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 3. Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to server." << std::endl;
    sleep(5);

    // 4. Send a message to the server
    const char* msg = "hello my name is mrityunjay saraf and I; am a software engineer and I am working on a project that uses sockets in C++. and I am very excited about it annd I hope to learn a lot from itz83BDv5MNq1AyHutZKfpbGdWYLh9rXe7mFVcogCl4EsRaQJOn6k0UIMNyPw2XxTjHV5LgzYitz83BDv5MNq1AyHutZKfpbGdWYLh9rXe7mFVcogCl4EsRaQJOn6k0UIMNyPw2XxTjHV5LgzY----";
    if (write(sockfd, msg, strlen(msg)) < 0) {
        perror("write");
        close(sockfd);
        return 1;
    }

    

    // 5. Read the server's response
    ssize_t n = read(sockfd, buffer, BUF_SIZE);
    if (n > 0) {
        std::cout << "Server replied: " << std::string(buffer, n) << std::endl;
    } else {
        perror("read");
    }

    // 6. Close the socket
    shutdown(sockfd,SHUT_WR);
    return 0;
}
