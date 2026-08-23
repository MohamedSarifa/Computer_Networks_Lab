#include "header.h"

typedef struct
{
    int id;
    char name[50];
    char time[20];
    int seats;
    int price;
} Movie;

Movie movies[3] =
{
    {1, "Avengers", "10:00 AM", 50, 150},
    {2, "Leo",      "2:00 PM",  40, 180},
    {3, "Vikram",   "6:00 PM",  60, 200}
};

int bookingMovie = -1;
int bookingSeats = 0;
int bookingAmount = 0;

void sendMessage(int s, char *msg)
{
    send(s, msg, strlen(msg) + 1, 0);
}

void movieList(int s)
{
    char msg[1000] = "";

    strcat(msg, "\n===== MOVIE LIST =====\n");

    for (int i = 0; i < 3; i++)
    {
        char temp[200];

        sprintf(temp,
                "ID: %d | Movie: %s | Time: %s | Seats: %d | Price: Rs.%d\n",
                movies[i].id,
                movies[i].name,
                movies[i].time,
                movies[i].seats,
                movies[i].price);

        strcat(msg, temp);
    }

    sendMessage(s, msg);
}

void checkSeats(int s)
{
    int id;
    char msg[200];

    recv(s, &id, sizeof(int), 0);

    if (id < 1 || id > 3)
    {
        sendMessage(s, "Invalid movie ID.");
        return;
    }

    sprintf(msg,
            "Movie: %s\nShow Time: %s\nAvailable Seats: %d",
            movies[id - 1].name,
            movies[id - 1].time,
            movies[id - 1].seats);

    sendMessage(s, msg);
}

void bookTicket(int s)
{
    int id, seats;
    char msg[300];

    recv(s, &id, sizeof(int), 0);
    recv(s, &seats, sizeof(int), 0);

    if (id < 1 || id > 3)
    {
        sendMessage(s, "Invalid movie ID.");
        return;
    }

    if (seats <= 0)
    {
        sendMessage(s, "Invalid number of seats.");
        return;
    }

    if (seats > movies[id - 1].seats)
    {
        sendMessage(s, "Booking failed: Not enough seats available.");
        return;
    }

    movies[id - 1].seats -= seats;

    bookingMovie = id;
    bookingSeats = seats;
    bookingAmount = seats * movies[id - 1].price;

    sprintf(msg,
            "Booking successful!\n"
            "Movie: %s\n"
            "Show Time: %s\n"
            "Tickets: %d\n"
            "Total Amount: Rs.%d\n"
            "Remaining Seats: %d",
            movies[id - 1].name,
            movies[id - 1].time,
            seats,
            bookingAmount,
            movies[id - 1].seats);

    sendMessage(s, msg);
}

void cancelTicket(int s)
{
    char msg[300];

    if (bookingMovie == -1)
    {
        sendMessage(s, "No active booking found.");
        return;
    }

    movies[bookingMovie - 1].seats += bookingSeats;

    sprintf(msg,
            "Booking cancelled successfully!\n"
            "Movie: %s\n"
            "Tickets cancelled: %d\n"
            "Seats now available: %d",
            movies[bookingMovie - 1].name,
            bookingSeats,
            movies[bookingMovie - 1].seats);

    bookingMovie = -1;
    bookingSeats = 0;
    bookingAmount = 0;

    sendMessage(s, msg);
}

void bookingDetails(int s)
{
    char msg[300];

    if (bookingMovie == -1)
    {
        sendMessage(s, "No booking found.");
        return;
    }

    sprintf(msg,
            "===== BOOKING DETAILS =====\n"
            "Movie: %s\n"
            "Show Time: %s\n"
            "Tickets: %d\n"
            "Total Amount: Rs.%d",
            movies[bookingMovie - 1].name,
            movies[bookingMovie - 1].time,
            bookingSeats,
            bookingAmount);

    sendMessage(s, msg);
}

int main()
{
    int ls, s;
    int choice;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen;

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERV_PORT);

    ls = socket(AF_INET, SOCK_STREAM, 0);

    if (ls < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    if (bind(ls, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    if (listen(ls, 5) < 0)
    {
        perror("Listen failed");
        exit(1);
    }

    printf("====================================\n");
    printf(" ITERATIVE TCP MOVIE SERVER\n");
    printf("====================================\n");
    printf("Server started on port %d...\n", SERV_PORT);

    while (1)
    {
        /*
         * ITERATIVE SERVER:
         * Accept one client.
         * Complete all operations.
         * Close client.
         * Then accept next client.
         */

        clientLen = sizeof(clientAddr);

        s = accept(ls,
                   (struct sockaddr *)&clientAddr,
                   &clientLen);

        if (s < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("\nClient connected.\n");

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
                    movieList(s);
                    break;

                case 2:
                    checkSeats(s);
                    break;

                case 3:
                    bookTicket(s);
                    break;

                case 4:
                    cancelTicket(s);
                    break;

                case 5:
                    bookingDetails(s);
                    break;

                case 6:
                    sendMessage(s, "Thank you! Connection terminated.");
                    printf("Client session completed.\n");
                    close(s);
                    s = -1;
                    break;

                default:
                    sendMessage(s, "Invalid choice. Please try again.");
            }

            if (s == -1)
                break;
        }

        if (s != -1)
            close(s);
    }

    close(ls);

    return 0;
}
