#include "headerFiles.h"

int main(int argc, char *argv[])
{
    int s;
    int n;

    char *servName;
    int servPort;

    char sendBuffer[256];
    char recvBuffer[256];

    struct sockaddr_in serverAddr;

    if (argc < 3)
    {
        printf("Error: Server IP and Port required!\n");
        exit(1);
    }

    servName = argv[1];
    servPort = atoi(argv[2]);

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;

    inet_pton(AF_INET,
              servName,
              &serverAddr.sin_addr);

    serverAddr.sin_port = htons(servPort);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error: Socket creation failed!");
        exit(1);
    }

    if (connect(s,
                (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0)
    {
        perror("Error: Connection failed!");
        exit(1);
    }

    printf("Connected to TCP Chat Server.\n");
    printf("Chat session started. Type 'exit' to quit.\n\n");

    for (;;)
    {
        printf("CLIENT: ");

        fgets(sendBuffer,
              sizeof(sendBuffer),
              stdin);

        sendBuffer[strcspn(sendBuffer, "\n")] = 0;

        send(s,
             sendBuffer,
             strlen(sendBuffer) + 1,
             0);

        if (strcmp(sendBuffer, "exit") == 0)
        {
            printf("Exiting chat...\n");
            break;
        }

        memset(recvBuffer, 0, sizeof(recvBuffer));

        n = recv(s,
                 recvBuffer,
                 sizeof(recvBuffer) - 1,
                 0);

        if (n <= 0)
        {
            printf("Server disconnected.\n");
            break;
        }

        recvBuffer[n] = '\0';

        if (strcmp(recvBuffer, "exit") == 0)
        {
            printf("Server ended the chat.\n");
            break;
        }

        printf("SERVER: %s\n", recvBuffer);
    }

    close(s);

    return 0;
}
