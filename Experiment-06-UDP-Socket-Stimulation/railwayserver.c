#include "header.h"
#include <time.h>

#define MAX_TRAINS 5

typedef struct
{
    int trainNo;
    char trainName[30];
    char source[20];
    char destination[20];
    int seats;
    float fare;
} Train;

Train trains[MAX_TRAINS] =
{
    {101, "Chennai Express", "Chennai", "Madurai", 50, 350.0},
    {102, "Pandian Express", "Madurai", "Chennai", 40, 400.0},
    {103, "Nellai Express", "Chennai", "Tirunelveli", 60, 450.0},
    {104, "Pearl City Express", "Tuticorin", "Chennai", 30, 300.0},
    {105, "Kanyakumari Express", "Chennai", "Kanyakumari", 45, 500.0}
};

void get_time(char *buffer)
{
    time_t now;
    struct tm *t;

    time(&now);
    t = localtime(&now);

    strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", t);
}

void log_activity(char *message)
{
    FILE *fp;
    char timeBuffer[100];

    get_time(timeBuffer);

    fp = fopen("railway_log.txt", "a");

    if (fp != NULL)
    {
        fprintf(fp, "[%s] %s\n", timeBuffer, message);
        fclose(fp);
    }
}

void send_string(int s, char *message)
{
    send(s, message, strlen(message) + 1, 0);
}

void search_trains(int s)
{
    char source[20];
    char destination[20];
    char result[2000] = "";
    char temp[300];
    int found = 0;

    recv(s, source, sizeof(source), 0);
    recv(s, destination, sizeof(destination), 0);

    for (int i = 0; i < MAX_TRAINS; i++)
    {
        if (strcasecmp(trains[i].source, source) == 0 &&
            strcasecmp(trains[i].destination, destination) == 0)
        {
            sprintf(temp,
                    "Train No: %d\n"
                    "Train Name: %s\n"
                    "Source: %s\n"
                    "Destination: %s\n"
                    "Available Seats: %d\n"
                    "Fare: %.2f\n\n",
                    trains[i].trainNo,
                    trains[i].trainName,
                    trains[i].source,
                    trains[i].destination,
                    trains[i].seats,
                    trains[i].fare);

            strcat(result, temp);
            found = 1;
        }
    }

    if (!found)
    {
        strcpy(result, "No trains found for this route.\n");
    }

    send_string(s, result);
}

void check_seats(int s)
{
    int trainNo;
    char result[300];

    recv(s, &trainNo, sizeof(int), 0);

    for (int i = 0; i < MAX_TRAINS; i++)
    {
        if (trains[i].trainNo == trainNo)
        {
            sprintf(result,
                    "Train: %s\nAvailable Seats: %d\nFare: %.2f\n",
                    trains[i].trainName,
                    trains[i].seats,
                    trains[i].fare);

            send_string(s, result);
            return;
        }
    }

    send_string(s, "Invalid train number.\n");
}

void book_ticket(int s)
{
    int trainNo;
    int seatsRequired;
    char result[300];

    recv(s, &trainNo, sizeof(int), 0);
    recv(s, &seatsRequired, sizeof(int), 0);

    for (int i = 0; i < MAX_TRAINS; i++)
    {
        if (trains[i].trainNo == trainNo)
        {
            if (seatsRequired <= 0)
            {
                send_string(s, "Invalid number of seats.\n");
                return;
            }

            if (seatsRequired <= trains[i].seats)
            {
                trains[i].seats -= seatsRequired;

                sprintf(result,
                        "Booking successful!\n"
                        "Train: %s\n"
                        "Seats Booked: %d\n"
                        "Total Fare: %.2f\n"
                        "Remaining Seats: %d\n",
                        trains[i].trainName,
                        seatsRequired,
                        seatsRequired * trains[i].fare,
                        trains[i].seats);

                log_activity("Ticket booking completed.");
            }
            else
            {
                strcpy(result, "Booking failed! Not enough seats.\n");
            }

            send_string(s, result);
            return;
        }
    }

    send_string(s, "Invalid train number.\n");
}

void cancel_ticket(int s)
{
    int trainNo;
    int seats;
    char result[300];

    recv(s, &trainNo, sizeof(int), 0);
    recv(s, &seats, sizeof(int), 0);

    for (int i = 0; i < MAX_TRAINS; i++)
    {
        if (trains[i].trainNo == trainNo)
        {
            if (seats <= 0)
            {
                send_string(s, "Invalid number of seats.\n");
                return;
            }

            trains[i].seats += seats;

            sprintf(result,
                    "Cancellation successful!\n"
                    "Train: %s\n"
                    "Seats Cancelled: %d\n"
                    "Available Seats: %d\n",
                    trains[i].trainName,
                    seats,
                    trains[i].seats);

            log_activity("Ticket cancellation completed.");

            send_string(s, result);
            return;
        }
    }

    send_string(s, "Invalid train number.\n");
}

void view_status(int s)
{
    char result[2000] = "";
    char temp[300];

    for (int i = 0; i < MAX_TRAINS; i++)
    {
        sprintf(temp,
                "Train %d - %s - Available Seats: %d - Fare: %.2f\n",
                trains[i].trainNo,
                trains[i].trainName,
                trains[i].seats,
                trains[i].fare);

        strcat(result, temp);
    }

    send_string(s, result);
}

int main()
{
    int ls;
    int s;
    int choice;
    int waitSize = 5;

    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;

    socklen_t clientLen;

    char clientIP[INET_ADDRSTRLEN];
    char logMessage[200];

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERV_PORT);

    /* Create socket */
    if ((ls = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    /* Bind */
    if (bind(ls, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0)
    {
        perror("Binding failed");
        exit(1);
    }

    /* Listen */
    if (listen(ls, waitSize) < 0)
    {
        perror("Listening failed");
        exit(1);
    }

    printf("=====================================\n");
    printf(" ITERATIVE TCP RAILWAY SERVER\n");
    printf("=====================================\n");
    printf("Server started...\n");
    printf("Waiting for clients...\n");

    while (1)
    {
        clientLen = sizeof(clientAddr);

        /* Accept one client */
        s = accept(ls, (struct sockaddr *)&clientAddr, &clientLen);

        if (s < 0)
        {
            perror("Accept failed");
            continue;
        }

        inet_ntop(AF_INET,
                  &clientAddr.sin_addr,
                  clientIP,
                  sizeof(clientIP));

        printf("\nClient connected.\n");
        printf("Client IP   : %s\n", clientIP);
        printf("Client Port : %d\n",
               ntohs(clientAddr.sin_port));

        sprintf(logMessage,
                "Client connected: IP=%s Port=%d",
                clientIP,
                ntohs(clientAddr.sin_port));

        log_activity(logMessage);

        /* Serve this client completely */
        while (1)
        {
            if (recv(s, &choice, sizeof(int), 0) <= 0)
            {
                printf("Client disconnected.\n");
                break;
            }

            switch (choice)
            {
                case 1:
                    printf("Client requested train search.\n");
                    log_activity("Client searched for trains.");
                    search_trains(s);
                    break;

                case 2:
                    printf("Client checked seat availability.\n");
                    log_activity("Client checked seat availability.");
                    check_seats(s);
                    break;

                case 3:
                    printf("Client requested ticket booking.\n");
                    book_ticket(s);
                    break;

                case 4:
                    printf("Client requested ticket cancellation.\n");
                    cancel_ticket(s);
                    break;

                case 5:
                    printf("Client requested booking status.\n");
                    log_activity("Client viewed booking status.");
                    view_status(s);
                    break;

                case 6:
                    printf("Client terminated session.\n");
                    log_activity("Client terminated session.");
                    send_string(s, "Session terminated.\n");
                    close(s);
                    goto next_client;

                default:
                    printf("Invalid request received.\n");
                    log_activity("Invalid request received.");
                    send_string(s, "Invalid choice. Try again.\n");
            }
        }

        close(s);

next_client:
        printf("Waiting for next client...\n");
    }

    close(ls);

    return 0;
}
