#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define MAX 100

struct RARPEntry
{
    char mac[20];
    char ip[20];
};

struct RARPEntry serverCache[MAX];

int count = 2;


void initializeCache()
{
    strcpy(serverCache[0].mac, "AA:BB:CC:DD:EE:10");
    strcpy(serverCache[0].ip, "192.168.1.10");

    strcpy(serverCache[1].mac, "AA:BB:CC:DD:EE:20");
    strcpy(serverCache[1].ip, "192.168.1.20");
}


int searchServerCache(char mac[], char ip[])
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(serverCache[i].mac, mac) == 0)
        {
            strcpy(ip, serverCache[i].ip);
            return 1;
        }
    }

    return 0;
}


void addToServerCache(char mac[], char ip[])
{
    strcpy(serverCache[count].mac, mac);
    strcpy(serverCache[count].ip, ip);

    count++;
}


void displayCache()
{
    int i;

    printf("\n========== SERVER RARP CACHE ==========\n");

    printf("MAC Address\t\tIP Address\n");
    printf("------------------------------------------\n");

    for (i = 0; i < count; i++)
    {
        printf("%-20s %s\n",
               serverCache[i].mac,
               serverCache[i].ip);
    }

    printf("=========================================\n");
}


int main()
{
    int serverSocket;
    int clientSocket;

    struct sockaddr_in server;
    struct sockaddr_in client;

    socklen_t clientSize;

    char mac[20];
    char ip[20];

    char response[30];


    initializeCache();


    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }


    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;


    if (bind(serverSocket,
             (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }


    listen(serverSocket, 5);


    printf("====================================\n");
    printf("          RARP SERVER\n");
    printf("====================================\n");


    while (1)
    {
        clientSize = sizeof(client);

        clientSocket = accept(serverSocket,
                              (struct sockaddr *)&client,
                              &clientSize);

        if (clientSocket < 0)
        {
            perror("Accept failed");
            continue;
        }


        memset(mac, 0, sizeof(mac));

        recv(clientSocket,
             mac,
             sizeof(mac),
             0);


        printf("\nReceived MAC from client: %s\n", mac);


        if (searchServerCache(mac, ip))
        {
            printf("\nChecking server cache...\n");

            printf("MAC Address : %s\n", mac);
            printf("IP Address  : %s\n", ip);


            send(clientSocket,
                 ip,
                 strlen(ip) + 1,
                 0);
        }


        else
        {
            printf("\nMAC not found in server cache.\n");
            printf("Asking client for IP address...\n");


            strcpy(response, "NOT_FOUND");

            send(clientSocket,
                 response,
                 strlen(response) + 1,
                 0);


            memset(ip, 0, sizeof(ip));

            recv(clientSocket,
                 ip,
                 sizeof(ip),
                 0);


            printf("\nClient returned IP.\n");

            printf("MAC Address : %s\n", mac);
            printf("IP Address  : %s\n", ip);


            addToServerCache(mac, ip);


            printf("\nAdded to server local cache.\n");

            strcpy(response, "ADDED");

            send(clientSocket,
                 response,
                 strlen(response) + 1,
                 0);


            displayCache();
        }


        close(clientSocket);

        printf("\nClient connection closed.");
        printf("\nWaiting for next client...\n");
    }


    close(serverSocket);

    return 0;
}
