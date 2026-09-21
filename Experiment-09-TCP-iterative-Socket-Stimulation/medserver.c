#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "calc.h"

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    CalcRequest request;
    CalcResponse response;

    // 1. Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Configure server address layout
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Target localhost

    while (1) {
        printf("\n========== UDP CALCULATOR ==========");
        printf("\nEnter operation (+, -, *, /) or 'q' to exit: ");
        scanf(" %c", &request.op);

        if (request.op == 'q' || request.op == 'Q') {
            printf("[+] Exiting calculation environment. Goodbye!\n");
            break;
        }

        // Simple runtime validation of operator inputs
        if (request.op != '+' && request.op != '-' && request.op != '*' && request.op != '/') {
            printf("[!] Invalid operator selected. Please use +, -, *, or /.\n");
            continue;
        }

        printf("Enter first operand: ");
        if (scanf("%lf", &request.num1) != 1) {
            printf("[!] Error: Numerical input required.\n");
            while (getchar() != '\n'); // flush invalid input buffer
            continue;
        }

        printf("Enter second operand: ");
        if (scanf("%lf", &request.num2) != 1) {
            printf("[!] Error: Numerical input required.\n");
            while (getchar() != '\n'); 
            continue;
        }

        // 3. Dispatch structured packet to the Server
        sendto(sockfd, &request, sizeof(CalcRequest), 0, 
               (struct sockaddr *)&server_addr, addr_len);

        // 4. Collect computation payload back from Server
        int bytes_received = recvfrom(sockfd, &response, sizeof(CalcResponse), 0, 
                                      (struct sockaddr *)&server_addr, &addr_len);

        if (bytes_received > 0) {
            // 5. Parse execution success metrics
            if (response.status == 0) {
                printf("[=] Server Response Result: %g\n", response.result);
            } else {
                printf("[X] Server Returned Error: %s\n", response.error_msg);
            }
        } else {
            printf("[X] Connection error: No response received from server.\n");
        }
    }

    close(sockfd);
    return 0;
}
