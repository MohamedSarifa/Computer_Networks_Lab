#include "header.h"

void receive_message(int s)
{
    char buffer[3000];
    int n;

    memset(buffer, 0, sizeof(buffer));

    n = recv(s, buffer, sizeof(buffer) - 1, 0);

    if (n > 0)
    {
        buffer[n] = '\0';
        printf("\n%s\n", buffer);
    }
}

int main(int argc, char *argv[])
{
    int s;
    int servPort;
    char *servName;

    int choice;
    int trainNo;
    int seats;

    char source[20];
    char destination[20];

    struct sockaddr_in serverAddr;

    if (argc < 3)
    {
        printf("Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(1);
    }

    servName = argv[1];
    servPort = atoi(argv[2]);

    /* Server address */
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;

    inet_pton(AF_INET,
              servName,
              &serverAddr.sin_addr);

    serverAddr.sin_port = htons(servPort);

    /* Create socket */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    /* Connect */
    if (connect(s,
                (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0)
    {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to Railway Reservation Server.\n");

    while (1)
    {
        printf("\n=====================================\n");
        printf(" ONLINE RAILWAY RESERVATION SYSTEM\n");
        printf("=====================================\n");
        printf("1. Search Trains\n");
        printf("2. Check Seat Availability\n");
        printf("3. Book Ticket\n");
        printf("4. Cancel Reservation\n");
        printf("5. View Booking Status\n");
        printf("6. Exit\n");
        printf("-------------------------------------\n");

        printf("Enter your choice: ");
        scanf("%d", &choice);

        send(s, &choice, sizeof(int), 0);

        switch (choice)
        {
            case 1:

                printf("Enter source: ");
                scanf("%s", source);

                printf("Enter destination: ");
                scanf("%s", destination);

                send(s,
                     source,
                     sizeof(source),
                     0);

                send(s,
                     destination,
                     sizeof(destination),
                     0);

                receive_message(s);

                break;

            case 2:

                printf("Enter train number: ");
                scanf("%d", &trainNo);

                send(s,
                     &trainNo,
                     sizeof(int),
                     0);

                receive_message(s);

                break;

            case 3:

                printf("Enter train number: ");
                scanf("%d", &trainNo);

                printf("Enter number of seats: ");
                scanf("%d", &seats);

                send(s,
                     &trainNo,
                     sizeof(int),
                     0);

                send(s,
                     &seats,
                     sizeof(int),
                     0);

                receive_message(s);

                break;

            case 4:

                printf("Enter train number: ");
                scanf("%d", &trainNo);

                printf("Enter number of seats to cancel: ");
                scanf("%d", &seats);

                send(s,
                     &trainNo,
                     sizeof(int),
                     0);

                send(s,
                     &seats,
                     sizeof(int),
                     0);

                receive_message(s);

                break;

            case 5:

                receive_message(s);

                break;

            case 6:

                receive_message(s);

                printf("Disconnected from server.\n");

                close(s);

                return 0;

            default:

                receive_message(s);

                break;
        }
    }

    close(s);

    return 0;
}
