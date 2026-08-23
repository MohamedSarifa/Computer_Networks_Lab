#include "headerFiles.h"

struct DHCPOfferPayload {
    int numSubnets;
    struct {
        int id;
        int reqAddr;
        int allocAddr;
        int prefix;
        char mask[50];
        char netAddr[50];
        char bcastAddr[50];
        char range[100];
    } subnets[10];
};

struct DHCPACKPayload {
    char clientName[50];
    char assignedIP[50];
    char subnetMask[50];
    char networkAddr[50];
    char broadcastAddr[50];
    int prefix;
};

int main(int argc, char *argv[]) {
    int s;
    char *servName;
    int servPort;
    char clientName[50];
    int subnetChoice;
    char buffer[100];
    struct sockaddr_in serverAddr;
    socklen_t addrLen;

    if (argc < 3) {
        printf("Error: Server IP and Port required!\n");
        printf("Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(1);
    }

    servName = argv[1];
    servPort = atoi(argv[2]);

    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error: Socket creation failed!");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, servName, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(servPort);

    printf("Enter Client Name: ");
    scanf("%s", clientName);

    printf("\nSending DHCP DISCOVER to server...\n");
    sendto(s, clientName, strlen(clientName) + 1, 0,
           (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    struct DHCPOfferPayload offer;
    addrLen = sizeof(serverAddr);
    recvfrom(s, &offer, sizeof(offer), 0, (struct sockaddr *)&serverAddr, &addrLen);

    printf("\n========== RECEIVED DHCP OFFER ==========\n");
    for (int i = 0; i < offer.numSubnets; i++) {
        printf("Subnet %d:\n", offer.subnets[i].id);
        printf("  - Requirement   : %d addresses (Allocated: %d)\n", offer.subnets[i].reqAddr, offer.subnets[i].allocAddr);
        printf("  - Prefix & Mask : /%d (%s)\n", offer.subnets[i].prefix, offer.subnets[i].mask);
        printf("  - Network / Bcast: %s / %s\n", offer.subnets[i].netAddr, offer.subnets[i].bcastAddr);
        printf("  - Usable Range  : %s\n\n", offer.subnets[i].range);
    }

    do {
        printf("Enter subnet choice (1-%d): ", offer.numSubnets);
        scanf("%d", &subnetChoice);

        if (subnetChoice < 1 || subnetChoice > offer.numSubnets) {
            printf("Invalid choice! Please select a valid subnet number between 1 and %d.\n", offer.numSubnets);
        }
    } while (subnetChoice < 1 || subnetChoice > offer.numSubnets);

    sprintf(buffer, "%s %d", clientName, subnetChoice);

    printf("Sending DHCP REQUEST for Subnet %d...\n", subnetChoice);
    sendto(s, buffer, strlen(buffer) + 1, 0,
           (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    struct DHCPACKPayload ack;
    recvfrom(s, &ack, sizeof(ack), 0, (struct sockaddr *)&serverAddr, &addrLen);

    printf("\n========== DHCP INFORMATION ACKNOWLEDGED ==========\n\n");
    printf("Client Name       : %s\n", ack.clientName);
    printf("Assigned IP       : %s\n", ack.assignedIP);
    printf("Subnet Mask       : %s\n", ack.subnetMask);
    printf("CIDR              : /%d\n", ack.prefix);
    printf("Network Address   : %s\n", ack.networkAddr);
    printf("Broadcast Address : %s\n\n", ack.broadcastAddr);
    printf("===================================================\n");

    close(s);
    exit(0);
}
