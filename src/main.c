#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/sendfile.h>


int create_and_bind_socket(long port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

void handle_client_request(int client_fd) {
    char buffer[1024] = {0}; // Buffer to store incoming data
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("Receive failed");
        close(client_fd);
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received data
    printf("Received request: %s\n", buffer);

        // This server will only accept GET requests
    // GET /index.html for example
    char* file = buffer + 5; // Skip "GET /"
    *strchr(file, ' ') = '\0'; // Null-terminate at the first space
    printf("Requested file: %s\n", file);
    // Open the requested file
    int file_fd = open(file, O_RDONLY);
    if (file_fd < 0) {
        perror("File open failed");
        const char* not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(client_fd, not_found_response, strlen(not_found_response), 0);
        close(client_fd);
        return;
    }
    // 6. Generate a response
    const char* response_header = "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: ";
    // Get the file size
    off_t file_size = lseek(file_fd, 0, SEEK_END);
    lseek(file_fd, 0, SEEK_SET); // Reset the file offset
    // Create the response header
    char response[256];
    snprintf(response, sizeof(response), "%s%ld\r\n\r\n", response_header, file_size);
    // Send the response header
    send(client_fd, response, strlen(response), 0);
    // 7. Send the response back to the client
    sendfile(client_fd, file_fd, NULL, 256); // Send the file to the client
    close(file_fd); // Close the file descriptor
    close(client_fd); // Close the client socket
}

int main(int argc, char* argv[]) {
    char* port_string = argv[1];
    if (port_string == NULL) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    char* endptr;
    errno = 0; // Reset errno before strtol
    long port = strtol(port_string, &endptr, 10);
    if (errno != 0) {
        perror("Invalid port number");
        return 1;
    }
    int socket_fd = create_and_bind_socket(port);

    // 2. Listen on the socket
    int listen_result = listen(socket_fd, 5); // 5 is the backlog size
    if (listen_result < 0) {
        perror("Listen failed");
        close(socket_fd);
        return 1;
    }
    printf("Listening on port 8080...\n");
    // 3. Accept incoming connections
    bool running = true;

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("Accept failed");
            close(socket_fd);
            return 1;
        }
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        // 4. Handle the client request
        handle_client_request(client_fd);
    }

    // 8. Close the socket
    close(socket_fd);

    return 0;
}