#include "header.h"

int main (void)
{
    int ls;
    int s;
    int integers[100];
    char* ptr;
    int numCount = 0;
    int sum = 0;
    int bytesToRecv = 0;
    int n = 0;
    int waitSize = 16;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clntAddrLen;

    memset (&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    serverAddr.sin_port = htons (SERV_PORT);

    if ((ls = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("Error: Listen socket failed!");
        exit (1);
    }

    if (bind (ls, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror ("Error: binding failed!");
        exit (1);
    }

    if (listen (ls, waitSize) < 0)
    {
        perror ("Error: listening failed!");
        exit (1);
    }

    for ( ; ; )
    {
        clntAddrLen = sizeof(clientAddr);
        if ((s = accept (ls, (struct sockaddr*)&clientAddr, &clntAddrLen)) < 0)
        {
            perror ("Error: accepting failed!");
            exit (1);
        }

        if (recv (s, &numCount, sizeof(int), 0) <= 0)
        {
            close (s);
            continue;
        }

        bytesToRecv = numCount * sizeof(int);
        ptr = (char*)integers;

        while (bytesToRecv > 0 && (n = recv (s, ptr, bytesToRecv, 0)) > 0)
        {
            ptr += n;
            bytesToRecv -= n;
        }

        sum = 0;
        for (int i = 0; i < numCount; i++)
        {
            sum += integers[i];
        }

        printf ("Calculated sum for client: %d\n", sum);

        send (s, &sum, sizeof(int), 0);

        close (s);
    }
}
