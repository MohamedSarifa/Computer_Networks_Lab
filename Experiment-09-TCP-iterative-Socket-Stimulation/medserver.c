#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "calc.h"

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
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
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 3. Bind socket to port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[+] UDP Calculator Server running on port %d...\n", PORT);

    while (1) {
        // 4. Await incoming request from a client
        int bytes_received = recvfrom(sockfd, &request, sizeof(CalcRequest), 0,
                                      (struct sockaddr *)&client_addr, &addr_len);
        if (bytes_received < 0) {
            perror("Receive failed");
            continue;
        }

        printf("[+] Received: %g %c %g from %s:%d\n", 
               request.num1, request.op, request.num2,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 5. Initialize response attributes
        response.status = 0;
        response.result = 0.0;
        memset(response.error_msg, 0, sizeof(response.error_msg));

        // 6. Perform calculation and validate bounds
        switch (request.op) {
            case '+':
                response.result = request.num1 + request.num2;
                break;
            case '-':
                response.result = request.num1 - request.num2;
                break;
            case '*':
                response.result = request.num1 * request.num2;
                break;
            case '/':
                if (request.num2 == 0.0) {
                    response.status = -1;
                    strcpy(response.error_msg, "Math Error: Division by Zero");
                } else {
                    response.result = request.num1 / request.num2;
                }
                break;
            default:
                response.status = -1;
                strcpy(response.error_msg, "System Error: Invalid Operator");
                break;
        }

        // 7. Transmit computation status back to the client
        sendto(sockfd, &response, sizeof(CalcResponse), 0,
               (struct sockaddr *)&client_addr, addr_len);
    }

    close(sockfd);
    return 0;
}
