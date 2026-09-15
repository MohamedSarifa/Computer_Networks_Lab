#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <dirent.h>

#define PORT 8095
#define BUFFER_SIZE 1024
#define FILE_DIR "./shared_files/" // Put files you want to share in this folder

void* handle_file_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE * 4];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int read_size = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0) break;

        buffer[strcspn(buffer, "\n")] = 0; // Clean command
        char command[20], filename[100];
        int num_args = sscanf(buffer, "%s %s", command, filename);

        if (strcmp(command, "LIST") == 0) {
            DIR *d = opendir(FILE_DIR);
            struct dirent *dir;
            memset(response, 0, sizeof(response));
            strcpy(response, "\n--- Available Files ---\n");

            if (d) {
                while ((dir = readdir(d)) != NULL) {
                    if (dir->d_type == DT_REG) { // Regular file
                        strcat(response, dir->d_name);
                        strcat(response, "\n");
                    }
                }
                closedir(d);
            } else {
                strcpy(response, "ERROR: Shared file directory missing.\n");
            }
            send(client_sock, response, strlen(response), 0);
        } 
        else if (strcmp(command, "GET") == 0 && num_args == 2) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s%s", FILE_DIR, filename);

            FILE *file = fopen(filepath, "rb");
            if (!file) {
                strcpy(response, "ERROR: File not found.\n");
                send(client_sock, response, strlen(response), 0);
            } else {
                strcpy(response, "START_TRANSFER\n");
                send(client_sock, response, strlen(response), 0);
                usleep(10000); // Small delay to separate headers

                int bytes_read;
                char file_buffer[BUFFER_SIZE];
                while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
                    send(client_sock, file_buffer, bytes_read, 0);
                }
                fclose(file);
                printf("Successfully sent file '%s' to client.\n", filename);
                break; // Break loop to close connection after file delivery completes
            }
        } 
        else {
            strcpy(response, "Invalid Command. Use LIST or GET <filename>\n");
            send(client_sock, response, strlen(response), 0);
        }
    }

    close(client_sock);
    printf("Client disconnected safely.\n");
    return NULL;
}

int main() {
    // Ensure the shared directory exists
    system("mkdir -p " FILE_DIR);

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Reuse port mapping optimization
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    listen(server_sock, 10);
    printf("File Sharing Server running on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int* client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);

        pthread_t t_id;
        pthread_create(&t_id, NULL, handle_file_client, client_sock);
        pthread_detach(t_id); // Clean up thread resources automatically on finish
    }
    return 0;
}
