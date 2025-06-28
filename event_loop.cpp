#include <cstring>      // For memset, strerror
#include <sys/epoll.h>  // For epoll functions
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h>  // For inet_ntop
#include <unistd.h>     // For close(), read(), write()
#include <fcntl.h>      // For fcntl()
#include <errno.h>      // For errno
#include <cstdio>       // For printf, perror
#include <cstdlib>      // For exit() and EXIT_FAILURE

#define BACKLOG 2
#define PORT 8080
#define MAX_EVENTS 1
#define BUFFER_SIZE 200

/*
* this function will set the flag of the file to NON_BLOCKING
*/
void set_fd_to_non_blocking(int fd){
    errno = 0; 
    int flags = fcntl(fd, F_GETFL, 0); 
    if (flags == -1) {
        perror("fcntl F_GETFL"); 
        exit(EXIT_FAILURE);      
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL"); 
        exit(EXIT_FAILURE);      
    }
}

/*
*
*/
int setup_listening_socket(){
    struct sockaddr_in address; // Structure to hold socket address information
    int server_fd;              // File descriptor for the server socket

    // Create a socket: AF_INET for IPv4, SOCK_STREAM for TCP, 0 for default protocol
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");       // Print error if socket creation fails
        return EXIT_FAILURE;    // Return failure code
    }

    // Initialize the address structure to zero
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;           // IPv4
    address.sin_port = htons(PORT);         // Port number, converted to network byte order
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Listen on localhost (127.0.0.1)

    int val = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("setsockopt SO_REUSEADDR"); 
        close(server_fd); 
        return EXIT_FAILURE;
    }
    
    // Bind the socket to the specified IP address and port
    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");  // Print error if bind fails
        close(server_fd); // Close the socket before returning
        return EXIT_FAILURE;
    }

    // Start listening for incoming connections with a backlog queue size
    if(listen(server_fd, BACKLOG) < 0){
        perror("listen failed"); // Print error if listen fails
        close(server_fd); // Close the socket before returning
        return EXIT_FAILURE;
    }
    printf("Server listening on port %d...\n", PORT);
    return server_fd; // Return the listening socket file descriptor
}

// Function to handle reading data from a client and echoing it back
void handle_client_data(int epollfd, int client_fd) {
    char buffer[BUFFER_SIZE + 1]; // Buffer for received data

    // In edge-triggered mode, read all available data until EAGAIN/EWOULDBLOCK or EOF
    while(true){
        sleep(20);
        // Read data from the client socket
        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if(bytes_read == -1){
            // If EAGAIN or EWOULDBLOCK, no more data to read for now
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("No more data to read from fd %d.\n", client_fd); 
                break; // Break from inner while loop, wait for next EPOLLIN event
            } else {
                // Other read errors
                perror("read error");
                // Remove client from epoll and close socket on error
                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
                    perror("epoll_ctl EPOLL_CTL_DEL (read error)");
                }
                close(client_fd);
                break; // Break from inner while loop
            }
        } else if (bytes_read == 0){
            // Client disconnected (end of file)
            printf("Client disconnected from fd %d.\n", client_fd);
            // Remove client from epoll and close socket
            if (epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
                perror("epoll_ctl EPOLL_CTL_DEL (EOF)");
            }
            close(client_fd);
            break; // Break from inner while loop
        } else {
            // Data received, null-terminate it and print
            buffer[bytes_read] = '\0';
            printf("Received %zd bytes from fd %d: '%s'\n", bytes_read, client_fd, buffer);

            // Echo the received data back to the client
            ssize_t total_bytes_written = 0;
            while (total_bytes_written < bytes_read) {
                ssize_t bytes_written_this_call = write(client_fd, buffer + total_bytes_written, bytes_read - total_bytes_written);
                if (bytes_written_this_call == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // Cannot write more right now. For a simple echo, we might stop here
                        // and lose the remaining part of the echo. For a full-featured server,
                        // you would buffer the remaining data and register for EPOLLOUT.
                        perror("write error (EAGAIN/EWOULDBLOCK)");
                        break; // Break from write loop, remaining data not echoed
                    } else {
                        // Other write errors
                        perror("write error echo");
                        // Remove client from epoll and close socket on error
                        if (epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
                            perror("epoll_ctl EPOLL_CTL_DEL (write error)");
                        }
                        close(client_fd);
                        break; // Break from write loop and outer read loop
                    }
                } else {
                    total_bytes_written += bytes_written_this_call;
                    printf("Echoed %zd bytes back to fd %d.\n", bytes_written_this_call, client_fd);
                }
            }
            if (total_bytes_written != bytes_read) {
                // If not all bytes were written due to EAGAIN/EWOULDBLOCK,
                // we break the outer read loop as well, to avoid trying to read more
                // before the current write buffer is cleared (if we were buffering).
                // In this simple echo, it just means the echo was partial.
                break;
            }
        }
    }
}

// Function to handle new client connections
void handle_new_connections(int epollfd, int server_fd) {
    while(true){
        struct sockaddr_in client_sock_addr; // Structure to store client address
        socklen_t client_sock_len = sizeof(client_sock_addr); // Size of client address structure

        // Accept a new connection. SOCK_NONBLOCK makes the accepted socket non-blocking.
        int client_fd = accept4(server_fd, (struct sockaddr*)&client_sock_addr, &client_sock_len, SOCK_NONBLOCK);
        if(client_fd == -1){
            // If no more connections are ready (EAGAIN or EWOULDBLOCK), break the accept loop
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // This is not an error, just no more connections to accept right now
                // printf("No more connections to accept.\n"); // Can uncomment for debugging
                break;
            } else {
                perror("error accepting new connection"); // Print error for other accept failures
                break; // Break from the accept loop on error
            }
        }

        // Prepare epoll event for the new client socket
        struct epoll_event client_epoll_event;
        client_epoll_event.events = EPOLLIN | EPOLLET; // Monitor for input, and use Edge-Triggered mode
        client_epoll_event.data.fd = client_fd;       // Associate with the new client socket

        // Add the new client socket to the epoll instance
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &client_epoll_event) == -1) {
            perror("epoll_ctl EPOLL_CTL_ADD client_sock"); // Print error if adding client_sock fails
            close(client_fd); // Close client socket on error
            continue;         // Continue to the next event if this one failed
        }

        char client_ip[INET_ADDRSTRLEN]; // Buffer for client IP address string
        // Convert client IP address from network to presentation format (thread-safe)
        inet_ntop(AF_INET, &(client_sock_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Accepted new connection from %s:%d (client_sock fd: %d)\n",
               client_ip, ntohs(client_sock_addr.sin_port), client_fd);
    }
}

// Main event loop for handling epoll events
void main_event_loop(int epollfd, int server_fd) {
    struct epoll_event events[MAX_EVENTS]; // Array to store events returned by epoll_wait
    bool running = true;                   // Flag to control the main server loop

    while(running){
        printf("Server is ready, waiting for events...\n");
        // Wait for events on the registered file descriptors indefinitely (-1 timeout)
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if(nfds == -1){
            // If epoll_wait was interrupted by a signal, continue the loop
            if(errno == EINTR){
                printf("epoll_wait: got an interrupt, continuing the loop.\n");
                continue;
            }
            // For any other epoll_wait error, print error and shut down
            perror("epoll_wait failure");
            running = false; // Set running to false to exit the loop
            break;           // Break from the loop
        }

        printf("Received %d events.\n", nfds);
        // Process all ready events
        for (int i = 0; i < nfds; i++){
            sleep(100);
            int current_fd = events[i].data.fd; // Get the file descriptor for the current event

            // If the event is on the listening socket, it means a new connection is pending
            if(current_fd == server_fd){
                handle_new_connections(epollfd, server_fd);
            } else {
                // This is a client socket ready to send or receive data
                handle_client_data(epollfd, current_fd);
            }
        }
    }
}

int main(int argc , char* argv[]) {
    int server_fd, epollfd;                  // Listening socket,Epoll instance file descriptor
    server_fd = setup_listening_socket();
    if (server_fd == EXIT_FAILURE) {
        fprintf(stderr, "Error: Failed to set up listening socket.\n");
        exit(EXIT_FAILURE);
    }

    // Set the listening socket to non-blocking mode
    set_fd_to_non_blocking(server_fd);

    // Create a new epoll instance
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1"); // Print error if epoll_create1 fails
        close(server_fd); // Close the server socket before exiting
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev; // Structure to define an event for epoll_ctl
    ev.events = EPOLLIN;   // Monitor for input events (data available to read)
    ev.data.fd = server_fd; // Associate the event with the server socket

    // Add the server socket to the epoll instance
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: listen_sock"); // Print error if adding listen_sock fails
        close(server_fd); // Close sockets before exiting
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    // Start the main event loop
    main_event_loop(epollfd, server_fd);

    // Clean up: Close the listening socket and the epoll instance
    close(server_fd);
    close(epollfd);
    printf("Listening socket and Epoll instance closed. Server shutting down.\n");
    return 0;
}
