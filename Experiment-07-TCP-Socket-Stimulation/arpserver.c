#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define MAX_ENTRIES 100
#define ARP_FILE "arp_table.txt"

struct ARPEntry
{
    char ip[100];
    char mac[100];
};

struct ARPEntry arp_table[MAX_ENTRIES];

int entry_count = 0;


void load_arp_table()
{
    FILE *fp;

    fp = fopen(ARP_FILE, "r");

    if (fp == NULL)
    {

        strcpy(arp_table[0].ip, "192.168.1.1");
        strcpy(arp_table[0].mac, "AA:BB:CC:DD:EE:01");

        strcpy(arp_table[1].ip, "192.168.1.2");
        strcpy(arp_table[1].mac, "AA:BB:CC:DD:EE:02");

        entry_count = 2;

        return;
    }

    entry_count = 0;

    while (entry_count < MAX_ENTRIES &&
           fscanf(fp,
                  "%99s %99s",
                  arp_table[entry_count].ip,
                  arp_table[entry_count].mac) == 2)
    {
        entry_count++;
    }

    fclose(fp);
}

void save_arp_table()
{
    FILE *fp;

    fp = fopen(ARP_FILE, "w");

    if (fp == NULL)
    {
        printf("Unable to save ARP table.\n");
        return;
    }

    for (int i = 0; i < entry_count; i++)
    {
        fprintf(fp,
                "%s %s\n",
                arp_table[i].ip,
                arp_table[i].mac);
    }

    fclose(fp);
}


int find_ip(char *ip)
{
    for (int i = 0; i < entry_count; i++)
    {
        if (strcmp(arp_table[i].ip, ip) == 0)
        {
            return i;
        }
    }

    return -1;
}


void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    char ip_request[100];

    memset(buffer, 0, sizeof(buffer));


    if (recv(client_socket,
             buffer,
             sizeof(buffer) - 1,
             0) <= 0)
    {
        close(client_socket);
        exit(0);
    }

    strcpy(ip_request, buffer);

    printf("\nClient requested MAC for IP: %s\n",
           ip_request);

    int index = find_ip(ip_request);

    if (index != -1)
    {
        char response[BUFFER_SIZE];

        snprintf(response,
                 sizeof(response),
                 "FOUND:%s",
                 arp_table[index].mac);

        send(client_socket,
             response,
             strlen(response) + 1,
             0);

        printf("IP found in ARP table.\n");
        printf("MAC = %s\n",
               arp_table[index].mac);
    }


    else
    {
        printf("IP not found in table.\n");

        send(client_socket,
             "NOT_FOUND",
             strlen("NOT_FOUND") + 1,
             0);


        memset(buffer, 0, sizeof(buffer));

        if (recv(client_socket,
                 buffer,
                 sizeof(buffer) - 1,
                 0) > 0)
        {
            if (strncmp(buffer, "ADD:", 4) == 0)
            {
                char add_ip[100];
                char add_mac[100];


                sscanf(buffer,
                       "ADD:%99[^:]:%99s",
                       add_ip,
                       add_mac);

                printf("\nReceived ADD request:\n");
                printf("IP  = %s\n", add_ip);
                printf("MAC = %s\n", add_mac);


                int existing = find_ip(add_ip);

                if (existing != -1)
                {
                    send(client_socket,
                         "ALREADY_EXISTS",
                         strlen("ALREADY_EXISTS") + 1,
                         0);

                    printf("Entry already exists.\n");
                }

                else if (entry_count >= MAX_ENTRIES)
                {
                    send(client_socket,
                         "TABLE_FULL",
                         strlen("TABLE_FULL") + 1,
                         0);

                    printf("ARP table is full.\n");
                }

                else
                {
                    strcpy(arp_table[entry_count].ip,
                           add_ip);

                    strcpy(arp_table[entry_count].mac,
                           add_mac);

                    entry_count++;


                    save_arp_table();

                    send(client_socket,
                         "ADDED_SUCCESSFULLY",
                         strlen("ADDED_SUCCESSFULLY") + 1,
                         0);

                    printf("\nAdded new entry:\n");
                    printf("IP  = %s\n", add_ip);
                    printf("MAC = %s\n", add_mac);
                }
            }
        }
    }

    printf("Client session completed.\n");

    close(client_socket);

    exit(0);
}

int main()
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t client_len;


    signal(SIGCHLD, SIG_IGN);


    load_arp_table();


    server_socket = socket(AF_INET,
                           SOCK_STREAM,
                           0);

    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }


    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr =
        htonl(INADDR_ANY);

    server_addr.sin_port =
        htons(PORT);


    if (bind(server_socket,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_socket);
        exit(1);
    }


    if (listen(server_socket, 10) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        exit(1);
    }

    printf("=========================================\n");
    printf("       CONCURRENT ARP SERVER\n");
    printf("=========================================\n");
    printf("Server listening on port %d...\n",
           PORT);

    while (1)
    {
        client_len = sizeof(client_addr);


        client_socket =
            accept(server_socket,
                   (struct sockaddr *)&client_addr,
                   &client_len);

        if (client_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("\nClient connected!\n");


        if (fork() == 0)
        {

            close(server_socket);

            handle_client(client_socket);
        }


        close(client_socket);

        printf("Waiting for client connection...\n");
    }

    close(server_socket);

    return 0;
}
