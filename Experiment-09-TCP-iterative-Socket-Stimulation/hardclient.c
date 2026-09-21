#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "library.h"

uint32_t current_tx_id = 100;

int send_reliable_request(int fd, struct sockaddr_in *dest, cmd_t cmd, const char *payload, Packet *output_res) {
    Packet req, ack, res;
    socklen_t len = sizeof(*dest);
    
    req.transaction_id = current_tx_id++;
    req.packet_type = PKT_REQ;
    req.command = cmd;
    memset(req.payload, 0, BUFFER_SIZE);
    if (payload) strcpy(req.payload, payload);

    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int retries = 0;
    while (retries < MAX_RETRIES) {
        // 1. Send Data Request
        sendto(fd, &req, sizeof(Packet), 0, (struct sockaddr *)dest, len);
        printf("[Tx] Sending Request ID: %u (Attempt %d/%d)\n", req.transaction_id, retries + 1, MAX_RETRIES);

        // 2. Await Server ACK
        int bytes = recvfrom(fd, &ack, sizeof(Packet), 0, (struct sockaddr *)dest, &len);
        if (bytes > 0 && ack.packet_type == PKT_ACK && ack.transaction_id == req.transaction_id) {
            printf("[Rx] ACK received from Server. Processing payload...\n");
            
            // 3. Collect Computed Data Response Packet
            while (1) {
                int res_bytes = recvfrom(fd, &res, sizeof(Packet), 0, (struct sockaddr *)dest, &len);
                if (res_bytes > 0 && res.packet_type == PKT_RES && res.transaction_id == req.transaction_id) {
                    *output_res = res;

                    // Send ACK back to server to finalize loop
                    Packet client_ack;
                    client_ack.transaction_id = req.transaction_id;
                    client_ack.packet_type = PKT_ACK;
                    sendto(fd, &client_ack, sizeof(Packet), 0, (struct sockaddr *)dest, len);
                    
                    // Reset to blocking operations
                    tv.tv_sec = 0;
                    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                    return 1; 
                }
            }
        }
        retries++;
        printf("[Warning] Timeout. Server unresponsive. Re-trying transaction...\n");
    }
    
    tv.tv_sec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return 0; // Transaction Failed
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    Packet response;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("Socket creation error"); exit(1); }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (1) {
        printf("\n======= LIBRARY SYSTEM MENU =======");
        printf("\n1. View Available Books\n2. Search for a Book\n3. Issue a Book\n4. Return a Book\n5. Exit\nChoose Option: ");
        int choice;
        if (scanf("%d", &choice) <= 0) break;
        getchar(); // clear line buffer

        if (choice == 5) break;

        cmd_t cmd;
        char payload[256] = {0};

        switch (choice) {
            case 1:
                cmd = CMD_LIST;
                break;
            case 2:
                cmd = CMD_SEARCH;
                printf("Enter Book Title or ISBN: ");
                fgets(payload, sizeof(payload), stdin);
                payload[strcspn(payload, "\n")] = '\0';
                break;
            case 3:
                cmd = CMD_ISSUE;
                printf("Enter Book ISBN to Issue: ");
                scanf("%s", payload);
                break;
            case 4:
                cmd = CMD_RETURN;
                printf("Enter Book ISBN to Return: ");
                scanf("%s", payload);
                break;
            default:
                printf("Invalid selection.\n");
                continue;
        }

        if (send_reliable_request(sockfd, &server_addr, cmd, payload, &response)) {
            printf("%s\n", response.payload);
        } else {
            printf("\n[Error] System Error: Network transaction dropped completely.\n");
        }
    }

    close(sockfd);
    return 0;
}
