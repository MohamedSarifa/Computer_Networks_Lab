#include "headerFiles.h"

int main (int argc, char* argv[ ])
{
    int s;
    int n;
    char* servName;
    int servPort;
    char sendBuffer[256];
    char recvBuffer[256];
    struct sockaddr_in serverAddr;
    socklen_t addrLen;

    if (argc < 3)
    {
        printf ("Error: Server IP and Port required!\n");
        exit (1);
    }

    servName = argv[1];
    servPort = atoi (argv[2]);

    memset (&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    inet_pton (AF_INET, servName, &serverAddr.sin_addr);
    serverAddr.sin_port = htons (servPort);

    if ((s = socket (PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror ("Error: Socket creation failed!");
        exit (1);
    }

    printf ("Chat session started. Type 'exit' to quit.\n\n");

    for ( ; ; )
    {
        printf ("CLIENT: ");
        fgets (sendBuffer, sizeof(sendBuffer), stdin);
        sendBuffer[strcspn(sendBuffer, "\n")] = 0;
        sendto (s, sendBuffer, strlen(sendBuffer) + 1, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (strcmp(sendBuffer, "exit") == 0)
        {
            printf ("Exiting chat...\n");
            break;
        }

        addrLen = sizeof(serverAddr);
        n = recvfrom (s, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr*)&serverAddr, &addrLen);
        if (n > 0)
        {
            printf ("SERVER: %s\n", recvBuffer);
        }
    }

    close (s);

    exit (0);
}
