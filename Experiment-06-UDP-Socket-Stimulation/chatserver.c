#include "headerFiles.h"

int main (void)
{
    int s;
    int n;
    char sendBuffer[256];
    char recvBuffer[256];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t clntAddrLen;

    memset (&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    serverAddr.sin_port = htons (SERV_PORT);

    if ((s = socket (PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror ("Error: Socket creation failed!");
        exit (1);
    }

    if (bind (s, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror ("Error: Binding failed!");
        exit (1);
    }

    printf ("Chat Server started. Waiting for messages...\n\n");

    for ( ; ; )
    {
        clntAddrLen = sizeof(clientAddr);

        n = recvfrom (s, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr*)&clientAddr, &clntAddrLen);
        if (n <= 0) continue;
        if (strcmp(recvBuffer, "exit") == 0)
        {
            printf ("Client disconnected.\n\n");
            continue;
        }

        printf ("CLIENT: %s\n", recvBuffer);

        printf ("SERVER: ");
        fgets (sendBuffer, sizeof(sendBuffer), stdin);
        sendBuffer[strcspn(sendBuffer, "\n")] = 0;

        sendto (s, sendBuffer, strlen(sendBuffer) + 1, 0, (struct sockaddr*)&clientAddr, clntAddrLen);
    }

    close (s);
}
mohamed_sarifa@LENOVO:~/NWL/EX6$ cat dnsclient.c
#include "headerFiles.h"

int main (int argc, char* argv[ ])
{
    int s;
    int n;
    char* servName;
    int servPort;
    char domain[100];
    char ipAddress[100];
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

    printf ("Enter URL: ");
    scanf ("%s", domain);
    sendto (s, domain, strlen(domain) + 1, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    addrLen = sizeof(serverAddr);
    n = recvfrom (s, ipAddress, sizeof(ipAddress), 0, (struct sockaddr*)&serverAddr, &addrLen);

    if (n > 0)
    {
        printf ("IP Address received: %s\n", ipAddress);
    }

    close (s);

    exit (0);
}
