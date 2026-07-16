#include <stdio.h>
#include <string.h>

struct NetworkTable {
    char url[30];
    int ip[4];
    char mac[20];
};

struct NetworkTable table[20];
int entryCount = 0;

char srcURL[100], destURL[100], filename[100];
struct NetworkTable srcNode, destNode;
int srcPort = 5000, destPort = 8080;

char originalData[500], stuffedData[1000];

// Converts an integer byte value into its 8-bit binary string representation
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

// Converts a 16-bit port number into two space-separated binary bytes
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

// Converts a hex MAC address string into space-separated binary chunks
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
}

void initializeNetworkTable() {
    strcpy(table[0].url, "google.com");
    table[0].ip[0] = 142; table[0].ip[1] = 250; table[0].ip[2] = 190; table[0].ip[3] = 46;
    strcpy(table[0].mac, "11:22:33:44");

    strcpy(table[1].url, "youtube.com");
    table[1].ip[0] = 142; table[1].ip[1] = 250; table[1].ip[2] = 191; table[1].ip[3] = 10;
    strcpy(table[1].mac, "55:66:77:88");

    entryCount = 2;
}

void getUserInput() {
    printf("Enter Source URL      : ");
    scanf("%s", srcURL);
    printf("Enter Destination URL : ");
    scanf("%s", destURL);
    printf("Enter Input File Name : ");
    scanf("%s", filename);
}

// Searches for a specific URL in the routing table
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
        strcpy(originalData, "HELLO ETX WORLD");
        return;
    }
    if (fgets(originalData, sizeof(originalData), fp) != NULL) {
        // Strip trailing newline characters if present
        size_t len = strlen(originalData);
        if (len > 0 && originalData[len - 1] == '\n') {
            originalData[len - 1] = '\0';
        }
    }
    fclose(fp);
}

void displayOriginalData() {
    printf("\nOriginal Data:\n%s\n", originalData);
}

void characterStuffing() {
    int i = 0;
    int j = 0;
    int len = (int)strlen(originalData);

    stuffedData[0] = '\0';

    while (i < len) {
        if (i <= len - 3 && originalData[i] == 'E' && originalData[i+1] == 'T' && originalData[i+2] == 'X') {
            stuffedData[j++] = 'D'; stuffedData[j++] = 'L'; stuffedData[j++] = 'E';
            stuffedData[j++] = ' ';
            stuffedData[j++] = 'E'; stuffedData[j++] = 'T'; stuffedData[j++] = 'X';
            i += 3;
        }
        else if (i <= len - 3 && originalData[i] == 'D' && originalData[i+1] == 'L' && originalData[i+2] == 'E') {
            stuffedData[j++] = 'D'; stuffedData[j++] = 'L'; stuffedData[j++] = 'E';
            stuffedData[j++] = ' ';
            stuffedData[j++] = 'D'; stuffedData[j++] = 'L'; stuffedData[j++] = 'E';
            i += 3;
        }
        else {
            stuffedData[j++] = originalData[i++];
        }
    }
    stuffedData[j] = '\0';
}

void displayStuffedData() {
    printf("\nStuffed Data:\n%s\n", stuffedData);
}

// Builds the human-readable formatted binary layout output
void buildLayoutOutput(FILE *stream) {
    char binaryString[200];
    char segment[20];
    int k;

    fprintf(stream, "================================================\n");
    fprintf(stream, "BISYNC FRAME LAYOUT\n");
    fprintf(stream, "================================================\n\n");

    fprintf(stream, "SYN\n00010110\n\n");
    fprintf(stream, "SYN\n00010110\n\n");
    fprintf(stream, "SOH\n00000001\n\n");

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

    fprintf(stream, "STX\n00000010\n\n");

    // Convert stuffed string characters to ASCII binary strings
    fprintf(stream, "DATA\n");
    int dataLen = (int)strlen(stuffedData);
    for(k = 0; k < dataLen; k++) {
        Bin((int)stuffedData[k], segment);
        fprintf(stream, "%s", segment);
        if(k < dataLen - 1) fprintf(stream, " ");
    }
    fprintf(stream, "\n\n");

    fprintf(stream, "ETX\n00000011\n\n");
    fprintf(stream, "CRC\n00000000\n");
}

// Saves a pure unbroken bitstream version into frame.txt
void saveRawFrameFile() {
    FILE *fp = fopen("frame.txt", "w");
    if (fp == NULL) return;

    char binaryString[200];
    char segment[20];
    int k;

    // SYN, SYN, SOH
    fprintf(fp, "000101100001011000000001");

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

    // STX
    fprintf(fp, "00000010");

    // DATA
    int dataLen = (int)strlen(stuffedData);
    for(k = 0; k < dataLen; k++) {
        Bin((int)stuffedData[k], segment);
        fprintf(fp, "%s", segment);
    }

    // ETX, CRC
    fprintf(fp, "0000001100000000\n");
    fclose(fp);
}

int main() {
    initializeNetworkTable();
    displayTable();
    printf("\n");

    getUserInput();

    // Step 6: Verify Source URL
    if (findNode(srcURL, &srcNode) == 0) {
        printf("\nInvalid Source URL\n");
        return 0;
    }

    // Step 7: Verify Destination URL
    if (findNode(destURL, &destNode) == 0) {
        printf("\nInvalid Destination URL\n");
        return 0;
    }

    readInputFile();
    displayOriginalData();
    characterStuffing();
    displayStuffedData();

    printf("\n");
    buildLayoutOutput(stdout); // Display completely frame

    // Save outputs to files
    saveRawFrameFile(); // Save protocol frame into frame.txt

    FILE *fpOut = fopen("output.txt", "w"); // Save formatted configuration layout into output.txt
    if (fpOut != NULL) {
        buildLayoutOutput(fpOut);
        fclose(fpOut);
    }

    printf("\nFrame Created Successfully.\n");
    return 0;
}
