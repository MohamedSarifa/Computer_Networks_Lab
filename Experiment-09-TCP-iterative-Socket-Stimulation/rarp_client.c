#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080


int main()
{
    int clientSocket;

    struct sockaddr_in server;

    char mac[20];
    char ip[20];

    char response[30];


    printf("====================================\n");
    printf("          RARP CLIENT\n");
    printf("====================================\n");

    printf("\nEnter MAC address: ");
    scanf("%s", mac);


    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    server.sin_addr.s_addr = inet_addr("127.0.0.1");


    if (connect(clientSocket,
                (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        perror("Connection failed");
        exit(1);
    }

    send(clientSocket,
         mac,
         strlen(mac) + 1,
         0);


    printf("\nMAC address sent to server.\n");


    memset(response, 0, sizeof(response));

    recv(clientSocket,
         response,
         sizeof(response),
         0);


    if (strcmp(response, "NOT_FOUND") != 0)
    {
        strcpy(ip, response);

        printf("\nMAC found in server cache.\n");

        printf("\nMAC Address : %s\n", mac);
        printf("IP Address  : %s\n", ip);
    }


    else
    {
        printf("\nMAC not found in server cache.\n");

        printf("\nEnter IP address manually: ");
        scanf("%s", ip);


        send(clientSocket,
             ip,
             strlen(ip) + 1,
             0);


        printf("\nIP address sent to server.\n");

        printf("\nMAC Address : %s\n", mac);
        printf("IP Address  : %s\n", ip);

        printf("\nNew MAC-IP entry added to server cache.\n");
    }


    close(clientSocket);

    return 0;
}
