#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define CACHE_FILE "arp_cache.txt"

int check_local_cache(char *ip, char *mac)
{
    FILE *fp;
    char file_ip[100];
    char file_mac[100];

    fp = fopen(CACHE_FILE, "r");

    if (fp == NULL)
        return 0;

    while (fscanf(fp, "%s %s", file_ip, file_mac) == 2)
    {
        if (strcmp(file_ip, ip) == 0)
        {
            strcpy(mac, file_mac);
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

void add_to_local_cache(char *ip, char *mac)
{
    FILE *fp;

    fp = fopen(CACHE_FILE, "a");

    if (fp == NULL)
    {
        printf("Unable to open local ARP cache.\n");
        return;
    }

    fprintf(fp, "%s %s\n", ip, mac);

    fclose(fp);
}

int main()
{
    int sock;
    struct sockaddr_in serv_addr;

    char ip_request[100];
    char mac_address[100];
    char buffer[BUFFER_SIZE];

    printf("=========================================\n");
    printf("              ARP CLIENT\n");
    printf("=========================================\n");

    printf("Enter IP address to search: ");
    scanf("%99s", ip_request);


    if (check_local_cache(ip_request, mac_address))
    {
        printf("\nIP address found in local ARP cache.\n");

        printf("\n--- Local ARP Cache Match ---\n");
        printf("IP Address  : %s\n", ip_request);
        printf("MAC Address : %s\n", mac_address);
        printf("-----------------------------\n");

        return 0;
    }

    printf("\nIP address not found in local ARP cache.\n");
    printf("Sending ARP request to server...\n");


    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket creation failed");
        return 1;
    }


    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1",
                  &serv_addr.sin_addr) <= 0)
    {
        printf("Invalid server address.\n");
        close(sock);
        return 1;
    }


    if (connect(sock,
                (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    printf("Connected to ARP Server.\n");


    send(sock,
         ip_request,
         strlen(ip_request) + 1,
         0);


    memset(buffer, 0, sizeof(buffer));

    recv(sock,
         buffer,
         sizeof(buffer) - 1,
         0);


    if (strncmp(buffer, "FOUND:", 6) == 0)
    {
        strcpy(mac_address, buffer + 6);

        printf("\n--- ARP Match Found ---\n");
        printf("IP Address  : %s\n", ip_request);
        printf("MAC Address : %s\n", mac_address);
        printf("-----------------------\n");


        add_to_local_cache(ip_request, mac_address);

        printf("\nEntry added to local ARP cache.\n");
    }


    else if (strcmp(buffer, "NOT_FOUND") == 0)
    {
        char choice;

        printf("\nIP address '%s' not found in server ARP table.\n",
               ip_request);

        printf("Would you like to add it? (y/n): ");
        scanf(" %c", &choice);

        if (choice == 'y' || choice == 'Y')
        {
            char mac_input[100];
            char add_message[BUFFER_SIZE];

            printf("Enter corresponding MAC address: ");
            scanf("%99s", mac_input);


            snprintf(add_message,
                     sizeof(add_message),
                     "ADD:%s:%s",
                     ip_request,
                     mac_input);

            send(sock,
                 add_message,
                 strlen(add_message) + 1,
                 0);


            memset(buffer, 0, sizeof(buffer));

            recv(sock,
                 buffer,
                 sizeof(buffer) - 1,
                 0);

            if (strcmp(buffer, "ADDED_SUCCESSFULLY") == 0)
            {
                printf("\nEntry successfully added to server ARP table.\n");


                add_to_local_cache(ip_request, mac_input);

                printf("Entry also added to local ARP cache.\n");

                printf("\nIP Address  : %s\n", ip_request);
                printf("MAC Address : %s\n", mac_input);
            }

            else if (strcmp(buffer, "ALREADY_EXISTS") == 0)
            {
                printf("\nEntry already exists in server ARP table.\n");
            }

            else
            {
                printf("\nFailed to add ARP entry.\n");
            }
        }

        else
        {
            printf("\nEntry was not added.\n");
        }
    }

    close(sock);

    return 0;
}
