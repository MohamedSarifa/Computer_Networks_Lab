#include <stdio.h>
#include <string.h>

struct NetworkTable {
    char url[30];
    int ip[4];
    char mac[20];
    char ipStr[20];
};

struct NetworkTable table[20];
int entryCount = 0;

char srcURL[100], destURL[100], filename[100];
struct NetworkTable srcNode, destNode;
int srcPort = 5000, destPort = 8080;

char originalData[500];
char binaryBodyData[5000];
int payloadLength = 0;
char countBinStr[15];

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

void calculate14BitBinary(int num, char *str) {
    int temp = num;
    int i;
    str[14] = '\0';
    for (i = 13; i >= 0; i--) {
        if (temp % 2 == 1) {
            str[i] = '1';
        } else {
            str[i] = '0';
        }
        temp = temp / 2;
    }
}

void stringToBinary(const char *input, char *output) {
    int len = (int)strlen(input);
    int i;
    char byteStr[9];

    output[0] = '\0';
    for (i = 0; i < len; i++) {
        Bin((int)input[i], byteStr);
        strcat(output, byteStr);
        if (i < len - 1) {
            strcat(output, " ");
        }
    }
}

void initializeNetworkTable() {
    strcpy(table[0].url, "google.com");
    table[0].ip[0] = 142; table[0].ip[1] = 250; table[0].ip[2] = 190; table[0].ip[3] = 46;
    strcpy(table[0].ipStr, "142.250.190.46");
    strcpy(table[0].mac, "11:22:33:44");

    strcpy(table[1].url, "youtube.com");
    table[1].ip[0] = 142; table[1].ip[1] = 250; table[1].ip[2] = 191; table[1].ip[3] = 10;
    strcpy(table[1].ipStr, "142.250.191.10");
    strcpy(table[1].mac, "55:66:77:88");

    entryCount = 2;
}

void displayTable() {
    int i;
    printf("==================== NETWORK ROUTING TABLE ====================\n");
    printf("%-20s | %-15s | %-15s\n", "URL", "IP Address", "MAC Address");
    printf("------------------------------------------------------------\n");
    for (i = 0; i < entryCount; i++) {
        printf("%-20s | %-15s | %-15s\n", table[i].url, table[i].ipStr, table[i].mac);
    }
    printf("\n");
}

int findNode(const char* url, struct NetworkTable* resultNode) {
    int i;
    for (i = 0; i < entryCount; i++) {
        if (strcmp(table[i].url, url) == 0) {
            *resultNode = table[i];
            return 1;
        }
    }
    return 0;
}

void readInputFile() {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: Could not open file. Using standard fallback.\n");
        strcpy(originalData, "HELLO WORLD");
        return;
    }
    if (fgets(originalData, sizeof(originalData), fp) != NULL) {
        size_t len = strlen(originalData);
        if (len > 0 && originalData[len - 1] == '\n') {
            originalData[len - 1] = '\0';
        }
    }
    fclose(fp);
}

void processByteCount() {
    payloadLength = (int)strlen(originalData);
    calculate14BitBinary(payloadLength, countBinStr);
    stringToBinary(originalData, binaryBodyData);
}

void printDDCMPLayout(FILE *stream) {
    char binaryString[100];
    char segment[20];
    int k;

    fprintf(stream, "==================================================\n");
    fprintf(stream, "            DDCMP BYTE COUNT FRAME LAYOUT\n");
    fprintf(stream, "==================================================\n\n");

    fprintf(stream, "SYN\n00010110\n\n");
    fprintf(stream, "SYN\n00010110\n\n");

    fprintf(stream, "CLASS\n00000001\n\n");

    fprintf(stream, "COUNT\n%s\n\n", countBinStr);

    fprintf(stream, "HEADER\n\n");

    fprintf(stream, "Source MAC\n");
    macToBin(srcNode.mac, binaryString);
    fprintf(stream, "%s\n\n", binaryString);

    fprintf(stream, "Destination MAC\n");
    macToBin(destNode.mac, binaryString);
    fprintf(stream, "%s\n\n", binaryString);

    fprintf(stream, "Source IP\n");
    binaryString[0] = '\0';
    for (k = 0; k < 4; k++) {
        Bin(srcNode.ip[k], segment);
        strcat(binaryString, segment);
        if (k < 3) strcat(binaryString, " ");
    }
    fprintf(stream, "%s\n\n", binaryString);

    fprintf(stream, "Destination IP\n");
    binaryString[0] = '\0';
    for (k = 0; k < 4; k++) {
        Bin(destNode.ip[k], segment);
        strcat(binaryString, segment);
        if (k < 3) strcat(binaryString, " ");
    }
    fprintf(stream, "%s\n\n", binaryString);

    fprintf(stream, "Source Port\n");
    portToBin(srcPort, binaryString);
    fprintf(stream, "%s\n\n", binaryString);

    fprintf(stream, "Destination Port\n");
    portToBin(destPort, binaryString);
    fprintf(stream, "%s\n\n", binaryString);

    fprintf(stream, "BODY\n\n");
    fprintf(stream, "%s\n\n", binaryBodyData);

    fprintf(stream, "CRC\n00000000\n");
}

void saveRawFrameFile() {
    FILE *fp = fopen("frame.txt", "w");
    if (fp == NULL) return;

    char binaryString[100];
    char segment[20];
    int k;

    // SYN, SYN, CLASS, COUNT
    fprintf(fp, "000101100001011000000001%s", countBinStr);

    // HEADER fields
    macToBin(srcNode.mac, binaryString);
    for(k=0; binaryString[k]; k++) if(binaryString[k]!=' ') fputc(binaryString[k], fp);

    macToBin(destNode.mac, binaryString);
    for(k=0; binaryString[k]; k++) if(binaryString[k]!=' ') fputc(binaryString[k], fp);

    for (k = 0; k < 4; k++) { Bin(srcNode.ip[k], segment); fprintf(fp, "%s", segment); }
    for (k = 0; k < 4; k++) { Bin(destNode.ip[k], segment); fprintf(fp, "%s", segment); }

    portToBin(srcPort, binaryString);
    for(k=0; binaryString[k]; k++) if(binaryString[k]!=' ') fputc(binaryString[k], fp);

    portToBin(destPort, binaryString);
    for(k=0; binaryString[k]; k++) if(binaryString[k]!=' ') fputc(binaryString[k], fp);

    // BODY (Remove spacing from binaryBodyData string)
    for(k=0; binaryBodyData[k]; k++) {
        if(binaryBodyData[k] != ' ') {
            fputc(binaryBodyData[k], fp);
        }
    }

    // CRC
    fprintf(fp, "00000000\n");
    fclose(fp);
}

int main() {
    // Step 1 & 2: Display Route Table
    initializeNetworkTable();
    displayTable();

    // Step 3, 4, 5: Gather input criteria
    printf("Enter Source URL      : ");
    scanf("%s", srcURL);
    printf("Enter Destination URL : ");
    scanf("%s", destURL);
    printf("Enter Input File Name : ");
    scanf("%s", filename);

    // Step 6: Targeted Search with validation stops
    if (findNode(srcURL, &srcNode) == 0) {
        printf("\nInvalid Source URL\n");
        return 0;
    }
    if (findNode(destURL, &destNode) == 0) {
        printf("\nInvalid Destination URL\n");
        return 0;
    }

    // Step 7 & 8: Process Message Data
    readInputFile();
    printf("\nOriginal Data:\n%s\n\n", originalData);

    // Step 9, 10, 11, 12: Binary conversions
    processByteCount();

    // Step 13 & 14: Construct and Display Complete Frame
    printDDCMPLayout(stdout);

    // Step 15: Save clean protocol bitstream fields to frame.txt
    saveRawFrameFile();

    // Step 16: Save formatted summary structure layout to output.txt
    FILE *fpOut = fopen("output.txt", "w");
    if (fpOut != NULL) {
        printDDCMPLayout(fpOut);
        fclose(fpOut);
    }

    // Step 17 & 18: Success notification and safe Stop
    printf("\nFrame Created Successfully.\n");
    return 0;
}
