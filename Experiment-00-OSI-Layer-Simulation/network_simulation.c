#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

struct NetworkTable {
    char url[30];
    int ip[4];
    char mac[20];
};

struct NetworkTable table[20];
int entryCount = 0;


int Bin(int num, char *str) {
    int temp = num;
    int i;
    for(i = 0; i < 8; i++) {
        str[i] = '0';
    }
    str[8] = '\0';
    for (i = 7; i >= 0; i--) {
        if (temp % 2 == 1) {
            str[i] = '1';
        } else {
            str[i] = '0';
        }
        temp = temp / 2;
    }
    return 1;
}

int portToBin(int port, char *str) {
    int highByte = port / 256;
    int lowByte = port % 256;
    int i;
    int index = 0;
    char hbin[9], lbin[9];
    Bin(highByte, hbin);
    Bin(lowByte, lbin);
    for(i = 0; i < 8; i++) {
        str[index] = hbin[i];
        index++;
    }
    str[index] = ' ';
    index++;
    for(i = 0; i < 8; i++) {
        str[index] = lbin[i];
        index++;
    }
    str[index] = '\0';
    return 1;
}

int macToBin(const char *mac, char *str) {
    int len = (int)strlen(mac);
    int bIdx = 0;
    int digitCount = 0;
    int i, j;

    for (i = 0; i < len; i++) {
        char c;
        int digit = 0;
        int temp;

        if (mac[i] == ':') {
            continue;
        }
        c = mac[i];
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;

        temp = digit;
        for (j = 3; j >= 0; j--) {
            if (temp % 2 == 1) {
                str[bIdx + j] = '1';
            } else {
                str[bIdx + j] = '0';
            }
            temp = temp / 2;
        }
        bIdx += 4;
        digitCount++;

        if (digitCount % 2 == 0 && i < len - 1) {
            str[bIdx] = ' ';
            bIdx++;
        }
    }
    str[bIdx] = '\0';
    return 1;
}

void displayTable() {
    int i;
    printf("\n==================== NETWORK ROUTING TABLE ====================\n");
    printf("%-20s | %-15s | %-15s\n", "URL", "IP Address", "MAC Address");
    printf("------------------------------------------------------------\n");
    for (i = 0; i < entryCount; i++) {
        printf("%-20s | %3d.%-3d.%-3d.%-3d | %-15s\n",
               table[i].url, table[i].ip[0], table[i].ip[1], table[i].ip[2], table[i].ip[3], table[i].mac);
    }
    printf("===============================================================\n");
}


int printApplicationLayer(const char *buffer, int totalLen) {
    int i;
    printf("========================================================================\n");
    printf(" LAYER 1 : APPLICATION LAYER\n");
    printf("------------------------------------------------------------------------\n");
    printf("  Message        : %s\n", buffer);
    printf("  Binary Format  : ");
    for (i = 0; i < totalLen; i++) {
        char binStr[9];
        Bin((int)buffer[i], binStr);
        printf("%s%s", binStr, (i < totalLen - 1) ? " " : "");
    }
    printf("\n\n========================================================================\n");
    return 1;
}

int printTransportLayer(int srcPort, const char *buffer, int totalLen) {
    int destPort = 50054;
    int i;
    char srcPortBin[30], destPortBin[30];

    portToBin(srcPort, srcPortBin);
    portToBin(destPort, destPortBin);

    printf("========================================================================\n");
    printf(" LAYER 2 : TRANSPORT LAYER (SEGMENT)\n");
    printf("------------------------------------------------------------------------\n");
    printf("  Source Port              : %d \n", srcPort);
    printf("  Destination Port         : %d\n", destPort);
    printf("------------------------------------------------------------------------\n");
    printf(" Binary Format\n");
    printf(" Source Port               : %s\n", srcPortBin);
    printf(" Destination Port          : %s\n", destPortBin);
    printf("------------------------------------------------------------------------\n");
    printf(" Layer Output:\n");
    printf("%s %s ", srcPortBin, destPortBin);
    for (i = 0; i < totalLen; i++) {
        char binStr[9];
        Bin((int)buffer[i], binStr);
        printf("%s%s", binStr, (i < totalLen - 1) ? " " : "");
    }
    printf("\n========================================================================\n");
    return 1;
}

int printNetworkLayer(int sIdx, int dIdx, const char *buffer, int totalLen, int packetBitSize, int srcPort) {
    int i, k;
    int packetCounter = 0;
    char pBin[9];
    char ipBinSrc[50], ipBinDest[50], tempIp[9];
    char srcPortBin[30], destPortBin[30];

    int charSize = packetBitSize / 8;
    if (charSize < 1) charSize = 2;

    portToBin(srcPort, srcPortBin);
    portToBin(50054, destPortBin);

    ipBinSrc[0] = '\0';
    for(i = 0; i < 4; i++) {
        Bin(table[sIdx].ip[i], tempIp); strcat(ipBinSrc, tempIp); if(i < 3) strcat(ipBinSrc, " ");
    }
    ipBinDest[0] = '\0';
    for(i = 0; i < 4; i++) {
        Bin(table[dIdx].ip[i], tempIp); strcat(ipBinDest, tempIp); if(i < 3) strcat(ipBinDest, " ");
    }

    printf("========================================================================\n");
    printf(" LAYER 3 : NETWORK LAYER (PACKET FRAGMENTATION)\n");
    printf("----------------------------------------------------------------------\n");

    for (i = 0; i < totalLen; i += charSize) {
        int currentLength = charSize;
        if (i + currentLength > totalLen) {
            currentLength = totalLen - i;
        }
        packetCounter++;

        printf("  [ Frame %d PACKET ]\n", packetCounter);
        printf("  Source IP        : %d.%d.%d.%d\n", table[sIdx].ip[0], table[sIdx].ip[1], table[sIdx].ip[2], table[sIdx].ip[3]);
        printf("  Destination IP   : %d.%d.%d.%d\n", table[dIdx].ip[0], table[dIdx].ip[1], table[dIdx].ip[2], table[dIdx].ip[3]);
        printf("----------------------------------------------------------------------\n");
        printf(" Packet Binary Output:\n");

        printf("%s %s %s %s ", ipBinSrc, ipBinDest, srcPortBin, destPortBin);
        for (k = 0; k < currentLength; k++) {
            Bin((int)buffer[i + k], pBin);
            printf("%s%s", pBin, (k < currentLength - 1) ? " " : "");
        }
        printf("\n\n");
    }

    printf(" Combined Fragmented Stream:\n ");
    for (i = 0; i < totalLen; i += charSize) {
        int currentLength = charSize;
        if (i + currentLength > totalLen) {
            currentLength = totalLen - i;
        }
        printf("%s %s %s %s ", ipBinSrc, ipBinDest, srcPortBin, destPortBin);
        for (k = 0; k < currentLength; k++) {
            Bin((int)buffer[i + k], pBin);
            printf("%s", pBin);
            if (k < currentLength - 1) printf(" ");
        }
        if (i + charSize < totalLen) printf(" ");
    }
    printf("\n");

    return packetCounter;
}


int printDataLinkLabel(int sIdx, int dIdx, const char *buffer, int totalLen, int srcPort, int packetBitSize, int frameBitSize) {
    int i;
    int charSize = packetBitSize / 8;
    if (charSize < 1) charSize = 2;

    int totalPackets = (totalLen + (charSize - 1)) / charSize;
    int totalFrames = totalPackets * 2;

    char srcMacBin[100], destMacBin[100];
    char srcPortBin[30], destPortBin[30];
    char ipBinSrc[50], ipBinDest[50], tempIp[9];

    macToBin(table[sIdx].mac, srcMacBin);
    macToBin(table[dIdx].mac, destMacBin);
    portToBin(srcPort, srcPortBin);
    portToBin(50054, destPortBin);

    ipBinSrc[0] = '\0';
    for(i = 0; i < 4; i++) {
        Bin(table[sIdx].ip[i], tempIp); strcat(ipBinSrc, tempIp); if(i < 3) strcat(ipBinSrc, " ");
    }
    ipBinDest[0] = '\0';
    for(i = 0; i < 4; i++) {
        Bin(table[dIdx].ip[i], tempIp); strcat(ipBinDest, tempIp); if(i < 3) strcat(ipBinDest, " ");
    }

    int currentFrameNum = 1;
    int pIdx, fNumInPacket;

    for (pIdx = 0; pIdx < totalPackets; pIdx++) {
        int byteStart = pIdx * charSize;
        int bitsAvailableInPacket = charSize * 8;

        if (byteStart + charSize > totalLen) {
            bitsAvailableInPacket = (totalLen - byteStart) * 8;
        }

        for (fNumInPacket = 1; fNumInPacket <= 2; fNumInPacket++) {
            int payloadBits = frameBitSize;
            int paddingBits = 0;
            char payloadString[128];
            int bitOffsetStart = (fNumInPacket - 1) * frameBitSize;

            if (bitOffsetStart >= bitsAvailableInPacket) {
                payloadBits = 0;
                paddingBits = frameBitSize;
            } else if (bitOffsetStart + payloadBits > bitsAvailableInPacket) {
                payloadBits = bitsAvailableInPacket - bitOffsetStart;
                paddingBits = frameBitSize - payloadBits;
            }

            int b;
            for (b = 0; b < payloadBits; b++) {
                int overallBitPos = (byteStart * 8) + bitOffsetStart + b;
                int charPos = overallBitPos / 8;
                int bitShift = 7 - (overallBitPos % 8);
                int bitVal = (buffer[charPos] >> bitShift) & 1;
                payloadString[b] = bitVal ? '1' : '0';
            }
            payloadString[payloadBits] = '\0';

            printf("\n==============================================================\n");
            printf("FRAME %d\n", currentFrameNum);
            printf("==============================================================\n\n");
            printf("Source MAC      : %s\n", table[sIdx].mac);
            printf("Destination MAC : %s\n\n", table[dIdx].mac);
            printf("Source IP       : %d.%d.%d.%d\n", table[sIdx].ip[0], table[sIdx].ip[1], table[sIdx].ip[2], table[sIdx].ip[3]);
            printf("Destination IP  : %d.%d.%d.%d\n\n", table[dIdx].ip[0], table[dIdx].ip[1], table[dIdx].ip[2], table[dIdx].ip[3]);
            printf("Source Port     : %d\n", srcPort);
            printf("Destination Port: 50054\n\n");
            printf("Frame Number    : %d\n\n", currentFrameNum);

            printf("Payload\n--------------------------------------------------------------\n");
            if(payloadBits > 0) printf("%s\n\n", payloadString);
            else printf("None\n\n");

            printf("Padding\n--------------------------------------------------------------\n");
            if (paddingBits > 0) {
                for(b = 0; b < paddingBits; b++) printf("0");
                printf("\n\n");
            } else {
                printf("None\n\n");
            }

            printf("Error Code\n--------------------------------------------------------------\n");
            printf("00000000\n\n");

            printf("Complete Frame\n--------------------------------------------------------------\n");
            printf("%s\n%s\n%s\n%s\n%s\n%s\n%s", srcMacBin, destMacBin, ipBinSrc, ipBinDest, srcPortBin, destPortBin, payloadString);
            for(b = 0; b < paddingBits; b++) printf("0");
            printf("\n00000000\n\n");
            printf("Frame Created Successfully.\n");

            currentFrameNum++;
        }
    }

    printf("\n==============================================================\n");
    printf("COMPLETE FRAME STREAM\n");
    printf("==============================================================\n\n");

    for (pIdx = 0; pIdx < totalPackets; pIdx++) {
        int byteStart = pIdx * charSize;
        int bitsAvailableInPacket = charSize * 8;

        if (byteStart + charSize > totalLen) {
            bitsAvailableInPacket = (totalLen - byteStart) * 8;
        }

        for (fNumInPacket = 1; fNumInPacket <= 2; fNumInPacket++) {
            int payloadBits = frameBitSize;
            int paddingBits = 0;
            char payloadString[128];
            int bitOffsetStart = (fNumInPacket - 1) * frameBitSize;

            if (bitOffsetStart >= bitsAvailableInPacket) {
                payloadBits = 0;
                paddingBits = frameBitSize;
            } else if (bitOffsetStart + payloadBits > bitsAvailableInPacket) {
                payloadBits = bitsAvailableInPacket - bitOffsetStart;
                paddingBits = frameBitSize - payloadBits;
            }

            int b;
            for (b = 0; b < payloadBits; b++) {
                int overallBitPos = (byteStart * 8) + bitOffsetStart + b;
                int charPos = overallBitPos / 8;
                int bitShift = 7 - (overallBitPos % 8);
                int bitVal = (buffer[charPos] >> bitShift) & 1;
                payloadString[b] = bitVal ? '1' : '0';
            }
            payloadString[payloadBits] = '\0';

            printf("%s %s %s %s %s %s %s", srcMacBin, destMacBin, ipBinSrc, ipBinDest, srcPortBin, destPortBin, payloadString);
            for(b = 0; b < paddingBits; b++) printf("0");
            printf(" 00000000\n");
        }
    }
    printf("\n==============================================================\n");

    return totalFrames;
}


int printPhysicalLayer(int sIdx, int dIdx, const char *buffer, int totalLen, int srcPort, int packetBitSize, int frameBitSize) {
    int i;
    int charSize = packetBitSize / 8;
    if (charSize < 1) charSize = 2;
    int totalPackets = (totalLen + (charSize - 1)) / charSize;
    int fNumInPacket, pIdx, b;

    char srcMacBin[100], destMacBin[100];
    char srcPortBin[30], destPortBin[30];
    char ipBinSrc[50], ipBinDest[50], tempIp[9];

    macToBin(table[sIdx].mac, srcMacBin);
    macToBin(table[dIdx].mac, destMacBin);
    portToBin(srcPort, srcPortBin);
    portToBin(50054, destPortBin);

    ipBinSrc[0] = '\0';
    for(i = 0; i < 4; i++) {
        Bin(table[sIdx].ip[i], tempIp); strcat(ipBinSrc, tempIp); if(i < 3) strcat(ipBinSrc, " ");
    }
    ipBinDest[0] = '\0';
    for(i = 0; i < 4; i++) {
        Bin(table[dIdx].ip[i], tempIp); strcat(ipBinDest, tempIp); if(i < 3) strcat(ipBinDest, " ");
    }

    printf("\n==============================================================\n");
    printf("LAYER 5 : PHYSICAL LAYER\n");
    printf("==============================================================\n\n");
    printf("Complete Combined Transmission Stream:\n\n");

    for (pIdx = 0; pIdx < totalPackets; pIdx++) {
        int byteStart = pIdx * charSize;
        int bitsAvailableInPacket = charSize * 8;
        if (byteStart + charSize > totalLen) {
            bitsAvailableInPacket = (totalLen - byteStart) * 8;
        }

        for (fNumInPacket = 1; fNumInPacket <= 2; fNumInPacket++) {
            int payloadBits = frameBitSize;
            int paddingBits = 0;
            char payloadString[128];
            int bitOffsetStart = (fNumInPacket - 1) * frameBitSize;

            if (bitOffsetStart >= bitsAvailableInPacket) {
                payloadBits = 0;
                paddingBits = frameBitSize;
            } else if (bitOffsetStart + payloadBits > bitsAvailableInPacket) {
                payloadBits = bitsAvailableInPacket - bitOffsetStart;
                paddingBits = frameBitSize - payloadBits;
            }

            for (b = 0; b < payloadBits; b++) {
                int overallBitPos = (byteStart * 8) + bitOffsetStart + b;
                int charPos = overallBitPos / 8;
                int bitShift = 7 - (overallBitPos % 8);
                int bitVal = (buffer[charPos] >> bitShift) & 1;
                payloadString[b] = bitVal ? '1' : '0';
            }
            payloadString[payloadBits] = '\0';

            printf("%s %s %s %s %s %s %s", srcMacBin, destMacBin, ipBinSrc, ipBinDest, srcPortBin, destPortBin, payloadString);
            for(b = 0; b < paddingBits; b++) printf("0");
            printf(" 00000000\n");
        }
    }
    printf("\n==============================================================\n");
    printf("TRANSMISSION COMPLETED SUCCESSFULLY\n");
    printf("==============================================================\n");
    return 1;
}

int printTransmissionSummary(int sIdx, int dIdx, const char *buffer, int totalFrames, int srcPort) {
    printf("\n========================================================================\n");
    printf(" TRANSMISSION SUMMARY\n");
    printf("========================================================================\n");
    printf("  Total Frames Sent  : %d\n", totalFrames);
    printf("  Complete Message   : %s\n", buffer);
    printf("  Source Port        : %-14d | Destination Port    : 50054\n", srcPort);
    printf("  Source IP          : %d.%d.%d.%d  | Destination IP      : %d.%d.%d.%d\n",
           table[sIdx].ip[0], table[sIdx].ip[1], table[sIdx].ip[2], table[sIdx].ip[3],
           table[dIdx].ip[0], table[dIdx].ip[1], table[dIdx].ip[2], table[dIdx].ip[3]);
    printf("  Source MAC         : %s  | Destination MAC     : %s\n", table[sIdx].mac, table[dIdx].mac);
    printf("  Delivery Status    : Transmission Successful\n");
    printf("========================================================================\n");
    return 1;
}

void executeNetworkSimulation(int sIdx, int dIdx, const char *buffer, int totalLen, int srcPort, int packetBitSize, int frameBitSize) {
    int totalFrames;
    printApplicationLayer(buffer, totalLen);
    printTransportLayer(srcPort, buffer, totalLen);
    printNetworkLayer(sIdx, dIdx, buffer, totalLen, packetBitSize, srcPort);
    totalFrames = printDataLinkLabel(sIdx, dIdx, buffer, totalLen, srcPort, packetBitSize, frameBitSize);
    printPhysicalLayer(sIdx, dIdx, buffer, totalLen, srcPort, packetBitSize, frameBitSize);
    printTransmissionSummary(sIdx, dIdx, buffer, totalFrames, srcPort);
}

int main() {
    int choice, i, j, sIdx, dIdx, len;
    int packetBitSize, frameBitSize;
    char src[30], dest[30], fileName[30], buffer[100], delUrl[30];
    FILE *fp;

    srand((unsigned int)time(NULL));

    strcpy(table[0].url, "client.lan");
    table[0].ip[0] = 192; table[0].ip[1] = 168; table[0].ip[2] = 1; table[0].ip[3] = 10;
    strcpy(table[0].mac, "AA:BB:CC:DD");

    strcpy(table[1].url, "google.com");
    table[1].ip[0] = 142; table[1].ip[1] = 250; table[1].ip[2] = 190; table[1].ip[3] = 46;
    strcpy(table[1].mac, "11:22:33:44");

    entryCount = 2;

    while(1) {
        printf("\n=================== NETWORK TOOL MENU ===================\n");
        printf(" 1. Add Network Node\n");
        printf(" 2. Delete Network Node\n");
        printf(" 3. Generate & Stream Frame\n");
        printf(" 4. View Network Table\n");
        printf(" 5. Exit Program\n");
        printf("---------------------------------------------------------\n");
        printf("Enter your choice (1-5): ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        if (choice == 5) {
            printf("\nExiting program. Goodbye!\n");
            break;
        }

        if (choice == 1) {
            printf("\n--- ADD NEW NODE ARCHITECTURE ---\n");
            if (entryCount >= 20) {
                printf("\n[ERROR] Registry limits exceeded.\n");
                continue;
            }
            printf("Enter Domain URL : ");
            scanf("%29s", table[entryCount].url);
            printf("Enter IP Address (e.g., 192.168.1.1) : ");
            scanf("%d.%d.%d.%d", &table[entryCount].ip[0], &table[entryCount].ip[1], &table[entryCount].ip[2], &table[entryCount].ip[3]);
            printf("Enter MAC Address (XX:XX:XX:XX) : ");
            scanf("%19s", table[entryCount].mac);
            entryCount++;
            printf("\n>>> Node added successfully!\n");
        }
        else if (choice == 2) {
            printf("\n--- DELETE NODE INTERFACE ---\n");
            printf("Enter the URL to delete: ");
            scanf("%29s", delUrl);
            int foundIdx = -1;
            for(i = 0; i < entryCount; i++) {
                if(strcmp(table[i].url, delUrl) == 0) {
                    foundIdx = i;
                    break;
                }
            }
            if(foundIdx == -1) {
                printf("\n[ERROR] URL not found.\n");
            } else {
                for(j = foundIdx; j < entryCount - 1; j++) {
                    table[j] = table[j + 1];
                }
                entryCount--;
                printf("\n>>> Node successfully deleted.\n");
            }
        }
        else if (choice == 4) {
            displayTable();
        }
        else if (choice == 3) {
            printf("\n--- FRAME CONFIGURATION WINDOW ---\n");
            printf("Source URL : ");
            scanf("%29s", src);
            printf("Destination URL : ");
            scanf("%29s", dest);
            printf("Packet Bit Size (e.g. 16 bits = 2 chars) : ");
            scanf("%d", &packetBitSize);
            printf("Frame Bit Size (e.g. 10 bits) : ");
            scanf("%d", &frameBitSize);
            printf("Source File Name : ");
            scanf("%29s", fileName);

            fp = fopen(fileName, "r");
            if (!fp) {
                printf("\n[ERROR] File missing or inaccessible.\n");
                continue;
            }
            if (fgets(buffer, sizeof(buffer), fp) == NULL) {
                buffer[0] = '\0';
            }
            fclose(fp);

            len = (int)strlen(buffer);
            if(len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';
                len--;
            }

            sIdx = -1; dIdx = -1;
            for(i = 0; i < entryCount; i++) {
                if(strcmp(table[i].url, src) == 0) sIdx = i;
                if(strcmp(table[i].url, dest) == 0) dIdx = i;
            }

            if(sIdx == -1 || dIdx == -1) {
                printf("\n[ERROR] Routing conflict: Match paths not verified.\n");
                continue;
            }

            int randomSrcPort = (rand() % 49152) + 1024;
            executeNetworkSimulation(sIdx, dIdx, buffer, len, randomSrcPort, packetBitSize, frameBitSize);
        }
    }
    return 0;
}
