#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8090
#define MAX_ROOMS 10
#define BUFFER_SIZE 1024

typedef struct {
    int room_number;
    int is_booked;
    char guest_name[50];
} Room;

Room hotel_rooms[MAX_ROOMS];
pthread_mutex_t hotel_mutex = PTHREAD_MUTEX_INITIALIZER;
const char* LOG_FILE = "hotel_bookings.log";

void log_transaction(const char* message) {
    FILE *file = fopen(LOG_FILE, "a");
    if (file) {
        time_t now = time(NULL);
        char *dt = ctime(&now);
        dt[strlen(dt) - 1] = '\0'; // Remove newline
        fprintf(file, "[%s] %s\n", dt, message);
        fclose(file);
    }
}

void init_rooms() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        hotel_rooms[i].room_number = 101 + i;
        hotel_rooms[i].is_booked = 0;
        strcpy(hotel_rooms[i].guest_name, "None");
    }
}

void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int read_size = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0) break;

        buffer[strcspn(buffer, "\n")] = 0; // Strip newline
        char command[20], arg1[50], arg2[50];
        int num_args = sscanf(buffer, "%s %s %s", command, arg1, arg2);

        memset(response, 0, BUFFER_SIZE);

        if (strcmp(command, "VIEW") == 0) {
            pthread_mutex_lock(&hotel_mutex);
            strcpy(response, "\n--- Room Status ---\n");
            for (int i = 0; i < MAX_ROOMS; i++) {
                char line[100];
                snprintf(line, sizeof(line), "Room %d: %s\n", 
                         hotel_rooms[i].room_number, 
                         hotel_rooms[i].is_booked ? "BOOKED" : "AVAILABLE");
                strcat(response, line);
            }
            pthread_mutex_unlock(&hotel_mutex);
        } 
        else if (strcmp(command, "BOOK") == 0 && num_args == 3) {
            int target_room = atoi(arg1);
            char *guest = arg2;
            int found = 0;

            pthread_mutex_lock(&hotel_mutex);
            for (int i = 0; i < MAX_ROOMS; i++) {
                if (hotel_rooms[i].room_number == target_room) {
                    found = 1;
                    if (hotel_rooms[i].is_booked) {
                        snprintf(response, BUFFER_SIZE, "ERROR: Room %d is already booked.\n", target_room);
                    } else {
                        hotel_rooms[i].is_booked = 1;
                        strncpy(hotel_rooms[i].guest_name, guest, 50);
                        snprintf(response, BUFFER_SIZE, "SUCCESS: Room %d booked for %s.\n", target_room, guest);
                        
                        char log_msg[100];
                        snprintf(log_msg, sizeof(log_msg), "BOOKED Room %d for %s", target_room, guest);
                        log_transaction(log_msg);
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&hotel_mutex);
            if (!found) snprintf(response, BUFFER_SIZE, "ERROR: Invalid room number.\n");
        } 
        else if (strcmp(command, "CANCEL") == 0 && num_args == 2) {
            int target_room = atoi(arg1);
            int found = 0;

            pthread_mutex_lock(&hotel_mutex);
            for (int i = 0; i < MAX_ROOMS; i++) {
                if (hotel_rooms[i].room_number == target_room) {
                    found = 1;
                    if (!hotel_rooms[i].is_booked) {
                        snprintf(response, BUFFER_SIZE, "ERROR: Room %d is not currently booked.\n", target_room);
                    } else {
                        char log_msg[100];
                        snprintf(log_msg, sizeof(log_msg), "CANCELLED Room %d (Previous Guest: %s)", target_room, hotel_rooms[i].guest_name);
                        log_transaction(log_msg);

                        hotel_rooms[i].is_booked = 0;
                        strcpy(hotel_rooms[i].guest_name, "None");
                        snprintf(response, BUFFER_SIZE, "SUCCESS: Reservation for Room %d cancelled.\n", target_room);
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&hotel_mutex);
            if (!found) snprintf(response, BUFFER_SIZE, "ERROR: Invalid room number.\n");
        } 
        else if (strcmp(command, "STATUS") == 0 && num_args == 2) {
            int target_room = atoi(arg1);
            int found = 0;

            pthread_mutex_lock(&hotel_mutex);
            for (int i = 0; i < MAX_ROOMS; i++) {
                if (hotel_rooms[i].room_number == target_room) {
                    found = 1;
                    snprintf(response, BUFFER_SIZE, "Room %d Details: Status=%s, Guest=%s\n", 
                             hotel_rooms[i].room_number, 
                             hotel_rooms[i].is_booked ? "Booked" : "Available", 
                             hotel_rooms[i].guest_name);
                    break;
                }
            }
            pthread_mutex_unlock(&hotel_mutex);
            if (!found) snprintf(response, BUFFER_SIZE, "ERROR: Invalid room number.\n");
        } 
        else {
            snprintf(response, BUFFER_SIZE, "Invalid Command. Use: VIEW, BOOK <room> <name>, CANCEL <room>, or STATUS <room>\n");
        }

        send(client_sock, response, strlen(response), 0);
    }

    close(client_sock);
    return NULL;
}

int main() {
    init_rooms();
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 10);
    printf("Hotel System Server listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int* client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);

        printf("Connected to new client.\n");
        pthread_t t_id;
        pthread_create(&t_id, NULL, handle_client, client_sock);
        pthread_detach(t_id);
    }
    return 0;
}
