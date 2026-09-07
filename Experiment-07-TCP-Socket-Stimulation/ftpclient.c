#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    int sock;
    int port;

    struct sockaddr_in serv_addr;

    char filename[BUFFER_SIZE] = {0};
    char buffer[BUFFER_SIZE] = {0};

    if (argc < 3)
    {
        printf("Usage: %s <server-ip> <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[2]);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket creation error");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET,
                  argv[1],
                  &serv_addr.sin_addr) <= 0)
    {
        printf("Invalid server IP address\n");
        close(sock);
        return 1;
    }

    if (connect(sock,
                (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)) < 0)
    {
        perror("Connection Failed");
        close(sock);
        return 1;
    }

    printf("=========================================\n");
    printf("              FTP CLIENT\n");
    printf("=========================================\n");

    printf("Connected to FTP Server %s:%d\n",
           argv[1],
           port);

    printf("Enter the filename to request: ");

    fgets(filename, sizeof(filename), stdin);

    filename[strcspn(filename, "\n")] = '\0';

    send(sock,
         filename,
         strlen(filename) + 1,
         0);

    memset(buffer, 0, sizeof(buffer));

    read(sock,
         buffer,
         sizeof(buffer) - 1);

    if (strcmp(buffer, "FILE_NOT_FOUND") == 0)
    {
        printf("\nError: File '%s' is not available on the server.\n",
               filename);
    }

    else if (strncmp(buffer, "FILE_FOUND:", 11) == 0)
    {
        long filesize;

        filesize = atol(buffer + 11);

        printf("\n--- File Details ---\n");
        printf("File Name : %s\n", filename);
        printf("File Size : %ld bytes\n", filesize);
        printf("Status    : File available on server.\n");

        printf("Receiving data...\n");

        char *ack = "READY";

        send(sock,
             ack,
             strlen(ack) + 1,
             0);

        char save_name[BUFFER_SIZE];

        snprintf(save_name,
                 sizeof(save_name),
                 "downloaded_%.1000s",
                 filename);

        FILE *file = fopen(save_name, "wb");

        if (file == NULL)
        {
            printf("Error creating local file.\n");
        }
        else
        {
            long totalReceived = 0;
            int bytesReceived;

            while (totalReceived < filesize)
            {
                bytesReceived = read(sock,
                                     buffer,
                                     BUFFER_SIZE);

                if (bytesReceived <= 0)
                {
                    break;
                }

                fwrite(buffer,
                       1,
                       bytesReceived,
                       file);

                totalReceived += bytesReceived;
            }

            fclose(file);

            if (totalReceived == filesize)
            {
                printf("\nDownload complete!\n");
            }
            else
            {
                printf("\nDownload incomplete!\n");
            }

            printf("Saved as: %s\n",
                   save_name);

            printf("Received: %ld bytes\n",
                   totalReceived);
        }
    }
    else
    {
        printf("\nUnknown response from FTP server.\n");
        printf("Server response: %s\n", buffer);
    }


    close(sock);
    printf("\nConnection closed.\n");

    return 0;
}
