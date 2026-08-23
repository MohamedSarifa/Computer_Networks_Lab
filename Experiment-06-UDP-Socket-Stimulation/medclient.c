#include "header.h"

int main(int argc, char *argv[])
{
    int s;
    int choice;
    int id, seats;
    char buffer[1000];

    struct sockaddr_in serverAddr;

    if (argc < 3)
    {
        printf("Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[2]));

    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    if (connect(s,
                (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0)
    {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to Movie Ticket Server.\n");

    while (1)
    {
        printf("\n====================================\n");
        printf("     MOVIE TICKET BOOKING SYSTEM\n");
        printf("====================================\n");
        printf("1. View Movie List\n");
        printf("2. Check Seat Availability\n");
        printf("3. Book Tickets\n");
        printf("4. Cancel Booking\n");
        printf("5. Display Booking Details\n");
        printf("6. Exit\n");
        printf("------------------------------------\n");

        printf("Enter your choice: ");
        scanf("%d", &choice);

        send(s, &choice, sizeof(int), 0);

        if (choice == 1)
        {
            recv(s, buffer, sizeof(buffer), 0);

            printf("%s\n", buffer);
        }

        else if (choice == 2)
        {
            printf("Enter Movie ID: ");
            scanf("%d", &id);

            send(s, &id, sizeof(int), 0);

            recv(s, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 3)
        {
            printf("Enter Movie ID: ");
            scanf("%d", &id);

            printf("Enter number of tickets: ");
            scanf("%d", &seats);

            send(s, &id, sizeof(int), 0);
            send(s, &seats, sizeof(int), 0);

            recv(s, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 4)
        {
            recv(s, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 5)
        {
            recv(s, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }

        else if (choice == 6)
        {
            recv(s, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);

            break;
        }

        else
        {
            recv(s, buffer, sizeof(buffer), 0);

            printf("\n%s\n", buffer);
        }
    }

    close(s);

    return 0;
}
