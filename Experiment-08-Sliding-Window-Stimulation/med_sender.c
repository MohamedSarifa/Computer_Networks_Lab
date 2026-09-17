#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

struct Frame {
    int seq_num;
    int lost_frame;
};

int main() {
    int total_frames, window_size, lost_frame;
    
    printf("--- Go-Back-N Sender ---\n");
    printf("Enter total frames: ");
    scanf("%d", &total_frames);
    printf("Enter window size: ");
    scanf("%d", &window_size);
    printf("Enter frame to lose (-1 for none): ");
    scanf("%d", &lost_frame);

    int sockfd;
    struct sockaddr_in recv_addr;
    socklen_t addr_len = sizeof(recv_addr);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(PORT);
    recv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int base = 1;
    int next_seq_num = 1;
    int total_transmissions = 0;
    bool loss_simulated = false;

    printf("\n--- Starting Transmission ---\n\n");

    while (base <= total_frames) {
        while (next_seq_num < base + window_size && next_seq_num <= total_frames) {
            struct Frame frame_to_send;
            frame_to_send.seq_num = next_seq_num;
            
            if (next_seq_num == lost_frame && !loss_simulated) {
                frame_to_send.lost_frame = lost_frame;
                loss_simulated = true;
                printf("Transmitting Frame %d... (SIMULATING LOSS)\n", next_seq_num);
            } else {
                frame_to_send.lost_frame = -1;
                printf("Transmitting Frame %d...\n", next_seq_num);
            }

            sendto(sockfd, &frame_to_send, sizeof(frame_to_send), 0, (const struct sockaddr *)&recv_addr, addr_len);
            next_seq_num++;
            total_transmissions++;
        }

        int ack_received;
        int n = recvfrom(sockfd, &ack_received, sizeof(ack_received), 0, (struct sockaddr *)&recv_addr, &addr_len);

        if (n < 0) {
            printf("\nTIMEOUT! No ACK received. Resetting window back to Frame %d\n", base);
            next_seq_num = base;
            if (lost_frame == base) {
                lost_frame = -1;
            }
            printf("--------------------------------------------------\n\n");
        } else {
            printf("Received ACK %d\n", ack_received);
            if (ack_received >= base + 1) {
                base = ack_received;
            }
        }
    }

    struct Frame end_frame = {-1, -1};
    sendto(sockfd, &end_frame, sizeof(end_frame), 0, (const struct sockaddr *)&recv_addr, addr_len);

    printf("\n--- Transmission Analysis ---\n");
    printf("Total successful frames delivered: %d\n", total_frames);
    printf("Total transmissions attempted:     %d\n", total_transmissions);
    double efficiency = ((double)total_frames / total_transmissions) * 100;
    printf("Protocol Efficiency:               %.2f%%\n", efficiency);

    close(sockfd);
    return 0;
}
