#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8090
#define BUFFER_SIZE 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to Hotel Server failed");
        close(sock);
        return 1;
    }

    printf("=== Connected to Hotel Reservation System ===\n");
    printf("Commands available:\n");
    printf("  VIEW\n");
    printf("  BOOK <room_number> <your_name>\n");
    printf("  CANCEL <room_number>\n");
    printf("  STATUS <room_number>\n");
    printf("  EXIT\n\n");

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE * 4];

    while (1) {
        printf("Hotel-Client> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        
        // Strip newline character
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "EXIT") == 0 || strcmp(buffer, "exit") == 0) {
            printf("Disconnecting...\n");
            break;
        }

        if (strlen(buffer) == 0) continue;

        // Send command to server
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("Failed to send command");
            break;
        }

        // Receive response from server
        memset(response, 0, sizeof(response));
        int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
        if (bytes_received <= 0) {
            printf("Server closed connection.\n");
            break;
        }

        printf("%s\n", response);
    }

    close(sock);
    return 0;
}
