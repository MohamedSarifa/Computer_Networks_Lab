#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8095
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
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to File Server failed");
        close(sock);
        return 1;
    }

    printf("=== Connected to File Sharing Server ===\n");
    printf("Commands available:\n");
    printf("  LIST\n");
    printf("  GET <filename>\n");
    printf("  EXIT\n\n");

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        printf("File-Client> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "EXIT") == 0 || strcmp(buffer, "exit") == 0) {
            break;
        }

        if (strlen(buffer) == 0) continue;

        
        char cmd[10], filename[256];
        int fields = sscanf(buffer, "%s %s", cmd, filename);

        
        send(sock, buffer, strlen(buffer), 0);

        memset(response, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, response, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            break;
        }

       
        if (strcmp(cmd, "GET") == 0 && strncmp(response, "START_TRANSFER", 14) == 0) {
            char download_name[300];
            snprintf(download_name, sizeof(download_name), "downloaded_%s", filename);
            
            FILE *file = fopen(download_name, "wb");
            if (!file) {
                perror("Could not create local file to save download");
                continue;
            }

            printf("Downloading file as '%s'...\n", download_name);
            
            
            int file_bytes;
            char file_buf[BUFFER_SIZE];
            while ((file_bytes = recv(sock, file_buf, BUFFER_SIZE, 0)) > 0) {
                fwrite(file_buf, 1, file_bytes, file);
            }
            
            fclose(file);
            printf("Download complete. Connection closed.\n");
            break; 
        } else {
            
            printf("%s\n", response);
        }
    }

    close(sock);
    return 0;
}
