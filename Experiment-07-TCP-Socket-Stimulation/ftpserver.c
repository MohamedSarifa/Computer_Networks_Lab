#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFER_SIZE 1024

void handle_client(int new_socket)
{
    char buffer[BUFFER_SIZE] = {0};

    memset(buffer, 0, BUFFER_SIZE);

    int valread = read(new_socket, buffer, BUFFER_SIZE);

    if (valread <= 0)
    {
        close(new_socket);
        exit(0);
    }

    buffer[strcspn(buffer, "\n")] = 0;

    printf("Client requested file: %s\n", buffer);

    FILE *file = fopen(buffer, "rb");

    if (file == NULL)
    {
        printf("File not found on server.\n");

        char *msg = "FILE_NOT_FOUND";
        send(new_socket, msg, strlen(msg) + 1, 0);
    }
    else
    {
        fseek(file, 0, SEEK_END);
        long filesize = ftell(file);
        fseek(file, 0, SEEK_SET);

        printf("File found! Size: %ld bytes.\n", filesize);

        char response[BUFFER_SIZE];

        snprintf(response, sizeof(response),
                 "FILE_FOUND:%ld", filesize);

        send(new_socket, response, strlen(response) + 1, 0);

        /* Wait for client READY */
        memset(buffer, 0, BUFFER_SIZE);
        read(new_socket, buffer, BUFFER_SIZE);

        size_t bytesRead;

        while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
        {
            send(new_socket, buffer, bytesRead, 0);
        }

        printf("File transfer complete.\n");

        fclose(file);
    }

    close(new_socket);
    exit(0);
}

int main(int argc, char *argv[])
{
    int server_fd, new_socket;
    int port;

    struct sockaddr_in address;
    socklen_t addrlen;

    if (argc < 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket failed");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd,
             (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("Listen failed");
        exit(1);
    }

    printf("Concurrent FTP Server listening on port %d...\n", port);

    /* Prevent zombie processes */
    signal(SIGCHLD, SIG_IGN);

    while (1)
    {
        printf("\n[Waiting for client connection...]\n");

        addrlen = sizeof(address);

        new_socket = accept(server_fd,
                            (struct sockaddr *)&address,
                            &addrlen);

        if (new_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Client connected!\n");

        if (fork() == 0)
        {
            /* Child process */
            close(server_fd);

            handle_client(new_socket);
        }

        /* Parent process */
        close(new_socket);
    }

    close(server_fd);

    return 0;
}
