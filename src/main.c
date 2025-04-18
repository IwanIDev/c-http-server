#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/sendfile.h>


int main(void) {
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
    int client_fd = accept(socket_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("Accept failed");
        close(socket_fd);
        return 1;
    }
    printf("Accepted a connection...\n");
    // 4. Read data from the socket as HTTP
    char buffer[1024] = {0}; // Buffer to store incoming data, filled with zeros
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("Receive failed");
        close(client_fd);
        close(socket_fd);
        return 1;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received data
    // 5. Parse the HTTP request
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
        close(socket_fd);
        return 1;
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

    // 8. Close the socket
    close(file_fd);
    close(client_fd);
    close(socket_fd);

    return 0;
}