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

struct Subnet {
    int reqAddresses;
    int allocAddresses;
    int prefix;
    uint32_t netAddr;
    uint32_t mask;
    uint32_t bcastAddr;
    uint32_t firstHost;
    uint32_t lastHost;
    uint32_t nextAvailableHost;
};

void ipToString(uint32_t ip, char *str) {
    struct in_addr addr;
    addr.s_addr = htonl(ip);
    strcpy(str, inet_ntoa(addr));
}

int main(void) {
    int s;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clntAddrLen;
    char buffer[100];

    char baseIPStr[50];
    int basePrefix, numSubnets;
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error: Socket creation failed!");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERV_PORT);

    if (bind(s, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error: Binding failed!");
        exit(1);
    }

    printf("========== DHCP SERVER CONFIGURATION ==========\n");
    printf("Enter Base Network Address : ");
    scanf("%s", baseIPStr);
    printf("Enter Base Prefix          : ");
    scanf("%d", &basePrefix);
    printf("Enter Number of Subnets    : ");
    scanf("%d", &numSubnets);

    struct in_addr baseInAddr;
    inet_pton(AF_INET, baseIPStr, &baseInAddr);
    uint32_t currentAddress = ntohl(baseInAddr.s_addr);

    struct Subnet subnets[numSubnets];

    for (int i = 0; i < numSubnets; i++) {
        printf("Enter addresses required for subnet %d: ", i + 1);
        scanf("%d", &subnets[i].reqAddresses);
    }

    for (int i = 0; i < numSubnets - 1; i++) {
        for (int j = i + 1; j < numSubnets; j++) {
            if (subnets[i].reqAddresses < subnets[j].reqAddresses) {
                struct Subnet temp = subnets[i];
                subnets[i] = subnets[j];
                subnets[j] = temp;
            }
        }
    }

    printf("\n========== CALCULATED VLSM SUBNETS ==========\n");
    for (int i = 0; i < numSubnets; i++) {
        int power = 1;
        int hostBits = 0;
        while (power < subnets[i].reqAddresses) {
            power <<= 1;
            hostBits++;
        }
        subnets[i].allocAddresses = power;
        subnets[i].prefix = 32 - hostBits;
        subnets[i].mask = (hostBits >= 32) ? 0 : (0xFFFFFFFF << hostBits) & 0xFFFFFFFF;

        subnets[i].netAddr = currentAddress;
        subnets[i].bcastAddr = subnets[i].netAddr + subnets[i].allocAddresses - 1;
        subnets[i].firstHost = subnets[i].netAddr + 1;
        subnets[i].lastHost = subnets[i].bcastAddr - 1;
        subnets[i].nextAvailableHost = subnets[i].firstHost;

        char netS[20], bcastS[20], maskS[20], fHostS[20], lHostS[20];
        ipToString(subnets[i].netAddr, netS);
        ipToString(subnets[i].bcastAddr, bcastS);
        ipToString(subnets[i].mask, maskS);
        ipToString(subnets[i].firstHost, fHostS);
        ipToString(subnets[i].lastHost, lHostS);

        printf("Subnet %d: Required = %d | Allocated = %d | Mask = %s (/%d)\n",
               i + 1, subnets[i].reqAddresses, subnets[i].allocAddresses, maskS, subnets[i].prefix);
        printf("          Network = %s | Broadcast = %s | Usable = %s - %s\n\n",
               netS, bcastS, fHostS, lHostS);

        currentAddress = subnets[i].bcastAddr + 1;
    }

    printf("DHCP Server running on port %d. Waiting for DHCP DISCOVER...\n\n", SERV_PORT);

    for (;;) {
        clntAddrLen = sizeof(clientAddr);

        if (recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clntAddrLen) <= 0)
            continue;

        char clientName[50];
        sscanf(buffer, "%s", clientName);
        printf("Received DHCP DISCOVER from Client: '%s'\n", clientName);

        struct DHCPOfferPayload offer;
        offer.numSubnets = numSubnets;
        for (int i = 0; i < numSubnets; i++) {
            offer.subnets[i].id = i + 1;
            offer.subnets[i].reqAddr = subnets[i].reqAddresses;
            offer.subnets[i].allocAddr = subnets[i].allocAddresses;
            offer.subnets[i].prefix = subnets[i].prefix;
            ipToString(subnets[i].mask, offer.subnets[i].mask);
            ipToString(subnets[i].netAddr, offer.subnets[i].netAddr);
            ipToString(subnets[i].bcastAddr, offer.subnets[i].bcastAddr);

            char fHost[20], lHost[20];
            ipToString(subnets[i].firstHost, fHost);
            ipToString(subnets[i].lastHost, lHost);
            sprintf(offer.subnets[i].range, "%s - %s", fHost, lHost);
        }

        sendto(s, &offer, sizeof(offer), 0, (struct sockaddr *)&clientAddr, clntAddrLen);
        printf("Sent DHCP OFFER to '%s'\n", clientName);

        if (recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clntAddrLen) <= 0)
            continue;

        int subnetChoice = 1;
        sscanf(buffer, "%s %d", clientName, &subnetChoice);
        printf("Received DHCP REQUEST from '%s' for Subnet Choice %d\n", clientName, subnetChoice);

        int idx = (subnetChoice >= 1 && subnetChoice <= numSubnets) ? (subnetChoice - 1) : 0;
        struct Subnet *selected = &subnets[idx];

        struct DHCPACKPayload ack;
        strcpy(ack.clientName, clientName);
        ack.prefix = selected->prefix;

        if (selected->nextAvailableHost <= selected->lastHost) {
            ipToString(selected->nextAvailableHost, ack.assignedIP);
            selected->nextAvailableHost++;
        } else {
            strcpy(ack.assignedIP, "POOL_EXHAUSTED");
        }

        ipToString(selected->mask, ack.subnetMask);
        ipToString(selected->netAddr, ack.networkAddr);
        ipToString(selected->bcastAddr, ack.broadcastAddr);

        sendto(s, &ack, sizeof(ack), 0, (struct sockaddr *)&clientAddr, clntAddrLen);
        printf("Sent DHCP ACK to '%s' | Assigned IP: %s\n\n", clientName, ack.assignedIP);
    }

    close(s);
    return 0;
}
