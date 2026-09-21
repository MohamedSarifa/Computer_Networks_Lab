#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "library.h"

Book db[MAX_BOOKS];
int book_count = 0;

// Log Transactions with Timestamps
void logger(const char *msg) {
    FILE *lf = fopen(LOG_FILE, "a");
    if (!lf) return;
    time_t now = time(NULL);
    char *dt = ctime(&now);
    dt[strlen(dt) - 1] = '\0'; // strip newline
    fprintf(lf, "[%s] %s\n", dt, msg);
    fclose(lf);
    printf("[%s] %s\n", dt, msg);
}

// Flat-file Database Mechanics
void load_db() {
    FILE *f = fopen(DB_FILE, "r");
    if (!f) {
        // Seed default books if file missing
        f = fopen(DB_FILE, "w");
        fprintf(f, "9781118063330 C_Programming Dennis_Ritchie 1\n");
        fprintf(f, "9780132350884 Clean_Code Robert_Martin 1\n");
        fprintf(f, "9780201633610 Design_Patterns Erich_Gamma 0\n");
        fclose(f);
        f = fopen(DB_FILE, "r");
    }
    book_count = 0;
    while (fscanf(f, "%s %s %s %d", db[book_count].isbn, db[book_count].title, 
                  db[book_count].author, &db[book_count].available) != EOF) {
        book_count++;
    }
    fclose(f);
}

void save_db() {
    FILE *f = fopen(DB_FILE, "w");
    for (int i = 0; i < book_count; i++) {
        fprintf(f, "%s %s %s %d\n", db[i].isbn, db[i].title, db[i].author, db[i].available);
    }
    fclose(f);
}

// Command Business Logic
void execute_command(Packet *req, Packet *res) {
    load_db();
    res->transaction_id = req->transaction_id;
    res->packet_type = PKT_RES;
    res->command = req->command;
    memset(res->payload, 0, BUFFER_SIZE);

    if (req->command == CMD_LIST) {
        char line[128];
        strcat(res->payload, "\nISBN          | Title                | Author               | Status\n---------------------------------------------------------------------\n");
        for (int i = 0; i < book_count; i++) {
            snprintf(line, sizeof(line), "%-13s | %-20s | %-20s | %s\n", 
                     db[i].isbn, db[i].title, db[i].author, db[i].available ? "Available" : "Borrowed");
            strcat(res->payload, line);
        }
    } 
    else if (req->command == CMD_SEARCH) {
        int found = 0;
        for (int i = 0; i < book_count; i++) {
            if (strcasecmp(db[i].title, req->payload) == 0 || strcmp(db[i].isbn, req->payload) == 0) {
                snprintf(res->payload, BUFFER_SIZE, "\nFound: %s by %s [%s] - %s\n", 
                         db[i].title, db[i].author, db[i].isbn, db[i].available ? "Available" : "Borrowed");
                found = 1;
                break;
            }
        }
        if (!found) strcpy(res->payload, "\nError: Book not found.\n");
    } 
    else if (req->command == CMD_ISSUE) {
        int found = 0;
        for (int i = 0; i < book_count; i++) {
            if (strcmp(db[i].isbn, req->payload) == 0) {
                found = 1;
                if (db[i].available == 1) {
                    db[i].available = 0;
                    save_db();
                    strcpy(res->payload, "\nSuccess: Book issued successfully.\n");
                } else {
                    strcpy(res->payload, "\nError: Book is already borrowed.\n");
                }
                break;
            }
        }
        if (!found) strcpy(res->payload, "\nError: Invalid ISBN.\n");
    } 
    else if (req->command == CMD_RETURN) {
        int found = 0;
        for (int i = 0; i < book_count; i++) {
            if (strcmp(db[i].isbn, req->payload) == 0) {
                found = 1;
                if (db[i].available == 0) {
                    db[i].available = 1;
                    save_db();
                    strcpy(res->payload, "\nSuccess: Book returned successfully.\n");
                } else {
                    strcpy(res->payload, "\nError: Book was not borrowed.\n");
                }
                break;
            }
        }
        if (!found) strcpy(res->payload, "\nError: Invalid ISBN.\n");
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    Packet rx_packet, tx_packet, ack_packet;

    // Transaction History Log to catch duplicates
    uint32_t past_transactions[1000];
    int tx_history_count = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("Socket failed"); exit(1); }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed"); exit(1);
    }

    logger("Reliable UDP Library Server operational...");

    while (1) {
        int bytes = recvfrom(sockfd, &rx_packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (bytes < 0) continue;

        if (rx_packet.packet_type == PKT_REQ) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Rx Request ID: %u | Cmd: %d", rx_packet.transaction_id, rx_packet.command);
            logger(log_msg);

            // 1. Immediately send ACK for the request
            ack_packet.transaction_id = rx_packet.transaction_id;
            ack_packet.packet_type = PKT_ACK;
            sendto(sockfd, &ack_packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, addr_len);

            // 2. Check for Duplicates
            int is_duplicate = 0;
            for (int i = 0; i < tx_history_count; i++) {
                if (past_transactions[i] == rx_packet.transaction_id) {
                    is_duplicate = 1;
                    break;
                }
            }

            if (is_duplicate) {
                logger("Duplicate request encountered. Dropped execution, sending cached payload confirmation.");
            } else {
                past_transactions[tx_history_count++] = rx_packet.transaction_id;
            }

            // 3. Process database mechanics and respond
            execute_command(&rx_packet, &tx_packet);

            // Reliable Response Delivery Loop (Wait for Client ACK)
            int retries = 0;
            struct timeval tv;
            tv.tv_sec = TIMEOUT_SEC;
            tv.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            while (retries < MAX_RETRIES) {
                sendto(sockfd, &tx_packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, addr_len);
                
                Packet client_ack;
                int ack_bytes = recvfrom(sockfd, &client_ack, sizeof(Packet), 0, (struct sockaddr *)&client_addr, &addr_len);
                
                if (ack_bytes > 0 && client_ack.packet_type == PKT_ACK && client_ack.transaction_id == rx_packet.transaction_id) {
                    logger("Client confirmed data packet arrival via ACK.");
                    break;
                }
                retries++;
                logger("Response ACK timeout. Initiating Retransmission...");
            }

            // Reset socket to blocking mode
            tv.tv_sec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        }
    }
    close(sockfd);
    return 0;
}
