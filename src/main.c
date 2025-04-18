#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>


int main(void) {
    printf("Hello, World!\n");

    // 1. Open socket (set to port 8080 for now)
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    // Set up the socket address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any address
    server_addr.sin_port = htons(8080); // Port 8080
    // Bind the socket to the address
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(socket_fd);
        return 1;
    }

    // 2. Listen on the socket
    int listen_result = listen(socket_fd, 5); // 5 is the backlog size
    if (listen_result < 0) {
        perror("Listen failed");
        close(socket_fd);
        return 1;
    }
    printf("Listening on port 8080...\n");
    // 3. Accept incoming connections

    // 4. Read data from the socket as HTTP

    // 5. Parse the HTTP request

    // 6. Generate a response

    // 7. Send the response back to the client

    // 8. Close the socket

    // 9. Repeat for the next connection

    return 0;
}