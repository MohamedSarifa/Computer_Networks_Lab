#include "header.h"

int main (int argc, char* argv[ ])
{
    int s;
    int n;
    char* servName;
    int servPort;
    int numCount;
    int integers[100];
    int sum = 0;
    struct sockaddr_in serverAddr;

    if (argc < 3)
    {
        printf ("Error: Server IP and Port are required!\n");
        exit (1);
    }

    servName = argv[1];
    servPort = atoi (argv[2]);

    printf ("Enter number of integers: ");
    scanf ("%d", &numCount);

    printf ("Enter %d integers: ", numCount);
    for (int i = 0; i < numCount; i++)
    {
        scanf ("%d", &integers[i]);
    }

    memset (&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    inet_pton (AF_INET, servName, &serverAddr.sin_addr);
    serverAddr.sin_port = htons (servPort);

    if ((s = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("Error: socket creation failed!");
        exit (1);
    }

    if (connect (s, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror ("Error: connection failed!");
        exit (1);
    }

    send (s, &numCount, sizeof(int), 0);
    send (s, integers, numCount * sizeof(int), 0);

    recv (s, &sum, sizeof(int), 0);

    printf ("Sum received from server: %d\n", sum);

    close (s);

    exit (0);
}
