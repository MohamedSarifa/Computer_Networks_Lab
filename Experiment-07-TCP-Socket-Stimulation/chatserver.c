#include "headerFiles.h"

int main(void)
{
    int ls, s;
    int n;
    char sendBuffer[256];
    char recvBuffer[256];

    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t clntAddrLen;

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERV_PORT);

    if ((ls = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error: Socket creation failed!");
        exit(1);
    }

    if (bind(ls, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0)
    {
        perror("Error: Binding failed!");
        exit(1);
    }

    if (listen(ls, 5) < 0)
    {
        perror("Error: Listening failed!");
        exit(1);
    }

    printf("Concurrent TCP Chat Server started.\n");
    printf("Waiting for clients...\n\n");

    for (;;)
    {
        clntAddrLen = sizeof(clientAddr);

        s = accept(ls, (struct sockaddr *)&clientAddr,
                   &clntAddrLen);

        if (s < 0)
        {
            perror("Error: Accept failed!");
            continue;
        }

        printf("New client connected!\n");

        if (fork() == 0)
        {

            close(ls);

            for (;;)
            {
                memset(recvBuffer, 0, sizeof(recvBuffer));

                n = recv(s, recvBuffer,
                         sizeof(recvBuffer) - 1, 0);

                if (n <= 0)
                {
                    printf("Client disconnected.\n");
                    break;
                }

                recvBuffer[n] = '\0';

                if (strcmp(recvBuffer, "exit") == 0)
                {
                    printf("Client exited.\n");
                    break;
                }

                printf("CLIENT: %s\n", recvBuffer);

                printf("SERVER: ");

                fgets(sendBuffer,
                      sizeof(sendBuffer), stdin);

                sendBuffer[strcspn(sendBuffer, "\n")] = 0;

                send(s, sendBuffer,
                     strlen(sendBuffer) + 1, 0);

                if (strcmp(sendBuffer, "exit") == 0)
                {
                    printf("Closing client connection.\n");
                    break;
                }
            }

            close(s);
            exit(0);
        }
        else
        {
            close(s);

        }

    }
    close(ls);

    return 0;
}
