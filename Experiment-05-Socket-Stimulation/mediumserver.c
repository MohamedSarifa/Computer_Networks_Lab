#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to check if a string is a palindrome
int is_palindrome(char *str) {
    int left = 0;
    int right = strlen(str) - 1;

    // Remove newline characters if present from network transmission
    while (right >= 0 && (str[right] == '\n' || str[right] == '\r')) {
        str[right] = '\0';
        right--;
    }

    while (left < right) {
        if (str[left] != str[right]) {
            return 0; // Not a palindrome
        }
        left++;
        right--;
    }
    return 1; // Is a palindrome
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];

    // 1. Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("[+] Server socket created successfully.\n");

    // 2. Configure server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY; 

    // 3. Bind the socket to the port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Binding successfull to port %d.\n", PORT);

    // 4. Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Listening for incoming connections...\n");

    // 5. Infinite loop to keep accepting client connections
    while (1) {
        addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        printf("[+] Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Clear buffer and read data from client
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            printf("[+] Received string from client: %s\n", buffer);

            // Check palindrome and send response
            if (is_palindrome(buffer)) {
                char response[] = "YES, it is a palindrome.\n";
                send(client_fd, response, strlen(response), 0);
            } else {
                char response[] = "NO, it is not a palindrome.\n";
                send(client_fd, response, strlen(response), 0);
            }
        }

        // Close the client communication socket
        close(client_fd);
        printf("[-] Client disconnected.\n\n");
    }

    // Close the server socket (unreachable in infinite loop)
    close(server_fd);
    return 0;
}
