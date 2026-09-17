#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

struct Frame {
    int seq_num;
    int lost_frame;
};

int main() {
    int sockfd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("--- Go-Back-N Receiver Started. Waiting for frames... ---\n\n");

    int expected_seq = 1;

    while (true) {
        struct Frame incoming_frame;
        recvfrom(sockfd, &incoming_frame, sizeof(incoming_frame), 0, (struct sockaddr *)&client_addr, &addr_len);

        if (incoming_frame.seq_num == -1) {
            printf("\nAll frames received successfully. Terminating.\n");
            break;
        }

        if (incoming_frame.seq_num == incoming_frame.lost_frame) {
            printf("! Error: Frame %d was lost/corrupted (Dropping frame)\n", incoming_frame.seq_num);
            continue; 
        }

        if (incoming_frame.seq_num == expected_seq) {
            printf("Frame %d received successfully. Sending ACK %d\n", incoming_frame.seq_num, expected_seq + 1);
            expected_seq++;
            sendto(sockfd, &expected_seq, sizeof(expected_seq), 0, (const struct sockaddr *)&client_addr, addr_len);
        } else {
            printf("Discarding out-of-order Frame %d. Expected Frame %d\n", incoming_frame.seq_num, expected_seq);
        }
    }

    close(sockfd);
    return 0;
}
