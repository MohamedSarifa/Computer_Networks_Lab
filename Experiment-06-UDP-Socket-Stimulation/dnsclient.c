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
