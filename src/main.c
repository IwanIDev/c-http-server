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
#include <signal.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

volatile sig_atomic_t stop = 0;
void handle_sigint(int sig) {
    (void)sig; // Unused parameter
    printf("\nReceived SIGINT, stopping server...\n");
    stop = 1;
}

int create_and_bind_socket(long port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = INADDR_ANY,
      .sin_port = htons(port)
    };

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

char* recieve_client_request(int client_fd) {
    char* buffer = malloc(BUFFER_SIZE); // Buffer to store incoming data
    int total_recieved = 0;
    while (1) {
      ssize_t bytes_received = recv(client_fd, buffer + total_recieved, BUFFER_SIZE - total_recieved - 1, 0);
      if (bytes_received <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("No data received, client may have closed the connection\n");
            free(buffer);
            close(client_fd);
            return NULL;
        }
        // Handle other errors
        perror("Receive failed");
        free(buffer);
        close(client_fd);
        return NULL;
      }

      total_recieved += bytes_received;
      buffer[total_recieved] = '\0'; // Null-terminate the received data
    
      // Check if we have received the full request
      if (strstr(buffer, "\r\n\r\n") != NULL) {
          break; // End of HTTP request
      }

      // Check if we have reached the buffer limit
      if (total_recieved >= BUFFER_SIZE - 1) {
          printf("Buffer limit reached, closing connection\n");
          free(buffer);
          close(client_fd);
          return NULL;
      }
    }
    return buffer;
}

void handle_client_request(int client_fd) {
    char* buffer = recieve_client_request(client_fd);
    if (buffer == NULL) {
        return; // Error in receiving request
    }
    printf("Received request: %s\n", buffer);

    // This server will only accept GET requests
    // GET /index.html for example
    char* file = buffer + 5; // Skip "GET /"
    char* end_of_file = strchr(file, ' '); // Find the first space after the file name 
    if (end_of_file == NULL) {
      printf("Malformed request\n");
      return;
    }
    *end_of_file = '\0'; // Null-terminate the file name
    printf("Requested file: %s\n", file);
    free(buffer); // Free the request buffer
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
    if (file_size < 0) {
        perror("lseek failed");
        close(file_fd);
        close(client_fd);
        return;
    }
    void* file_data = malloc(file_size); // Allocate memory for the file file_data
    if (read(file_fd, file_data, file_size) != file_size) {
        perror("File read failed");
        free(file_data);
        close(file_fd);
        close(client_fd);
        return;
    }

    // Create the response header
    char response[256];
    snprintf(response, sizeof(response), "%s%ld\r\n\r\n", response_header, file_size);
    // Send the response header
    send(client_fd, response, strlen(response), 0);
    // 7. Send the response back to the client
    printf("Sending file of size %ld bytes\n", file_size);
    send(client_fd, file_data, file_size, 0); // Send the file to the client
    free(file_data);
    printf("File sent successfully\n");
    close(file_fd); // Close the file descriptor
    close(client_fd); // Close the client socket
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint); // Register signal handler for SIGINT
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
    // Get port number as string
    int length = snprintf(NULL, 0, "%ld", port);
    char* port_str = calloc(length, sizeof(char));
    snprintf(port_str, length, "%ld", port);

    printf("Listening on port %s...\n", port_string);
    free(port_str);
    // 3. Accept incoming connections

    while (!stop) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
        struct timeval timeout;
        timeout.tv_sec = 1; // 1 second timeout
        timeout.tv_usec = 0;
        int activity = select(socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (activity < 0) {
            perror("Select failed");
            close(socket_fd);
            return 1;
        }

        if (stop) {
            break;
        }

        if (FD_ISSET(socket_fd, &read_fds)) {
            // Accept a new connection
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd < 0) {
                perror("Accept failed");
                continue; // Continue to the next iteration
            }
            // Print the client's IP address and port
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
            // 4. Handle the client request
            handle_client_request(client_fd);
        }
    }

    // 8. Close the socket
    close(socket_fd);

    return 0;
}
