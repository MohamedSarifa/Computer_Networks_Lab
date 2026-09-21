#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 1. Create TCP socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Configure server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost

    // 3. Connect to the server
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Connected to the server successfully.\n");

    // 4. Get input string from user
    printf("Enter a string to check for palindrome: ");
    if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        // Strip trailing newline character added by fgets
        buffer[strcspn(buffer, "\n")] = '\0';

        // 5. Send string to the server
        send(client_fd, buffer, strlen(buffer), 0);

        // 6. Receive response from the server
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        printf("[+] Server response: %s", buffer);
    }

    // 7. Close socket
    close(client_fd);
    return 0;
}
