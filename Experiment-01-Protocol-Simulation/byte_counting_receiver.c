#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char raw_stream[6000];

// Bit buffers for extraction
char syn1_bin[9], syn2_bin[9], class_bin[9], count_bin[15];
char srcMac_bin[33], destMac_bin[33];
char srcIp_bin[33], destIp_bin[33];
char srcPort_bin[17], destPort_bin[17];
char raw_binary_body[4000];
char crc_bin[9];

// Readable variables
char decodedSrcMac[30], decodedDestMac[30];
char decodedSrcIp[30], decodedDestIp[30];
int decodedSrcPort = 0, decodedDestPort = 0;
char receivedMessageText[1000];

int decodedCount = 0;
int actualByteCount = 0;

void trimNewline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

// Safely isolates structural slices out of the continuous bitstream
void sliceBits(const char *src, int start, int length, char *dest) {
    strncpy(dest, src + start, length);
    dest[length] = '\0';
}

// Converts an 8-bit binary segment into its character equivalence
char binToChar(const char *bin8) {
    char val = 0;
    for (int i = 0; i < 8 && bin8[i] != '\0'; i++) {
        val = (val << 1) | (bin8[i] - '0');
    }
    return val;
}

// Converts a raw binary sequence into a standard integer evaluation
int binToInt(const char *bin, int bits) {
    int val = 0;
    for (int i = 0; i < bits && bin[i] != '\0'; i++) {
        val = (val << 1) | (bin[i] - '0');
    }
    return val;
}

// Formats a raw 32-bit sequence into your sender's custom MAC string representation
void decodeMac(const char *bin32, char *macOut) {
    macOut[0] = '\0';
    for (int i = 0; i < 4; i++) {
        char sub[9];
        sliceBits(bin32, i * 8, 8, sub);
        unsigned char byteVal = binToChar(sub);
        char segment[5];
        sprintf(segment, "%02X", byteVal);
        strcat(macOut, segment);
        if (i < 3) strcat(macOut, ":");
    }
}

// Decodes a raw 32-bit sequence into a standard dotted decimal IP format
void decodeIp(const char *bin32, char *ipOut) {
    int octets[4];
    for (int i = 0; i < 4; i++) {
        char sub[9];
        sliceBits(bin32, i * 8, 8, sub);
        octets[i] = (unsigned char)binToChar(sub);
    }
    sprintf(ipOut, "%d.%d.%d.%d", octets[0], octets[1], octets[2], octets[3]);
}

// Decodes a raw 16-bit sequence into a numeric Port value
int decodePort(const char *bin16) {
    return binToInt(bin16, 16);
}

// Adds visual spaces into binary outputs to match your sender's display style
void formatWithSpaces(const char *src, char *dest, int groupSize) {
    int len = (int)strlen(src);
    int dIdx = 0;
    for (int i = 0; i < len; i++) {
        dest[dIdx++] = src[i];
        if ((i + 1) % groupSize == 0 && (i + 1) < len) {
            dest[dIdx++] = ' ';
        }
    }
    dest[dIdx] = '\0';
}

void parseRawBitStream() {
    FILE *fp = fopen("frame.txt", "r");
    if (fp == NULL) {
        printf("Error: Could not open source bitstream file 'frame.txt'\n");
        exit(1);
    }
    if (fgets(raw_stream, sizeof(raw_stream), fp) == NULL) {
        printf("Error: 'frame.txt' file is empty.\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    trimNewline(raw_stream);
    int streamLen = (int)strlen(raw_stream);

    // Step 3: Read fields sequentially using strict bit alignments matching the sender
    sliceBits(raw_stream, 0, 8, syn1_bin);
    sliceBits(raw_stream, 8, 8, syn2_bin);
    sliceBits(raw_stream, 16, 8, class_bin);
    sliceBits(raw_stream, 24, 14, count_bin); // 14-bit COUNT field

    // Decode Count instantly to accurately track following lengths
    decodedCount = binToInt(count_bin, 14);

    // Header layout field Extractions (Starts at bit offset 38)
    sliceBits(raw_stream, 38, 32, srcMac_bin);
    sliceBits(raw_stream, 70, 32, destMac_bin);
    sliceBits(raw_stream, 102, 32, srcIp_bin);
    sliceBits(raw_stream, 134, 32, destIp_bin);
    sliceBits(raw_stream, 166, 16, srcPort_bin);
    sliceBits(raw_stream, 182, 16, destPort_bin);

    // Calculate dynamic body segment space allocation
    int bodyStartOffset = 198;
    int bodyBitLength = streamLen - bodyStartOffset - 8; // Preserving trailing 8-bit CRC

    if (bodyBitLength < 0) bodyBitLength = 0;

    sliceBits(raw_stream, bodyStartOffset, bodyBitLength, raw_binary_body);
    sliceBits(raw_stream, bodyStartOffset + bodyBitLength, 8, crc_bin);

    actualByteCount = bodyBitLength / 8;
}

void processMessagePayload() {
    int msgLen = 0;
    // Step 7 & 8: Read exactly COUNT bytes from the BODY and convert to ASCII
    for (int i = 0; i < decodedCount && i < actualByteCount; i++) {
        char byteBin[9];
        sliceBits(raw_binary_body, i * 8, 8, byteBin);
        receivedMessageText[msgLen++] = binToChar(byteBin);
    }
    receivedMessageText[msgLen] = '\0';
}

void printReceiverLayout(FILE *stream) {
    char spacedMacSrc[100], spacedMacDest[100];
    char spacedIpSrc[100], spacedIpDest[100];
    char spacedPortSrc[50], spacedPortDest[50];
    char spacedBody[5000];

    formatWithSpaces(srcMac_bin, spacedMacSrc, 8);
    formatWithSpaces(destMac_bin, spacedMacDest, 8);
    formatWithSpaces(srcIp_bin, spacedIpSrc, 8);
    formatWithSpaces(destIp_bin, spacedIpDest, 8);
    formatWithSpaces(srcPort_bin, spacedPortSrc, 8);
    formatWithSpaces(destPort_bin, spacedPortDest, 8);
    formatWithSpaces(raw_binary_body, spacedBody, 8);

    fprintf(stream, "==================================================\n");
    fprintf(stream, "            DDCMP RECEIVER OUTPUT PROCESSING      \n");
    fprintf(stream, "==================================================\n\n");

    // Steps 4 & 5: Decode and display headers
    fprintf(stream, "--- Decoded Header Fields ---\n");
    fprintf(stream, "Source MAC           : %s\n", decodedSrcMac);
    fprintf(stream, "Destination MAC      : %s\n", decodedDestMac);
    fprintf(stream, "Source IP            : %s\n", decodedSrcIp);
    fprintf(stream, "Destination IP       : %s\n", decodedDestIp);
    fprintf(stream, "Source Port          : %d\n", decodedSrcPort);
    fprintf(stream, "Destination Port     : %d\n", decodedDestPort);
    fprintf(stream, "Decoded COUNT Field  : %d (decimal)\n\n", decodedCount);

    // Step 9: Display the received message
    fprintf(stream, "--- Message Text Payload ---\n");
    fprintf(stream, "Received Message     : %s\n\n", receivedMessageText);

    // Steps 10, 11, 12, 13: Verifications
    fprintf(stream, "--- Verification Status Metrics ---\n");
    fprintf(stream, "Received Byte Count  : %d\n", decodedCount);
    fprintf(stream, "Actual Body Bytes    : %d\n", actualByteCount);

    if (decodedCount == actualByteCount) {
        fprintf(stream, "Byte Count Validation: Frame Valid\n");
    } else {
        fprintf(stream, "Byte Count Validation: Frame Corrupted\n");
    }

    if (strcmp(crc_bin, "00000000") == 0) {
        fprintf(stream, "CRC Verification     : CRC Valid\n\n");
    } else {
        fprintf(stream, "CRC Verification     : CRC Error\n\n");
    }

    // Step 3 output rendering layout
    fprintf(stream, "--- Raw Frame Structural Binary Layout ---\n");
    fprintf(stream, "SYN   : %s\n", syn1_bin);
    fprintf(stream, "SYN   : %s\n", syn2_bin);
    fprintf(stream, "CLASS : %s\n", class_bin);
    fprintf(stream, "COUNT : %s\n", count_bin);
    fprintf(stream, "BODY  : %s\n", spacedBody);
    fprintf(stream, "CRC   : %s\n", crc_bin);
}

int main() {
    // Step 1, 2, 3: Open frame.txt and process raw bits sequentially
    parseRawBitStream();

    // Step 4: Decode all extracted header sequences
    decodeMac(srcMac_bin, decodedSrcMac);
    decodeMac(destMac_bin, decodedDestMac);
    decodeIp(srcIp_bin, decodedSrcIp);
    decodeIp(destIp_bin, decodedDestIp);
    decodedSrcPort = decodePort(srcPort_bin);
    decodedDestPort = decodePort(destPort_bin);

    // Step 7 & 8: Process binary blocks directly into ASCII values
    processMessagePayload();

    // Step 5 through 13: Print fully validated structure summary to console
    printf("\n");
    printReceiverLayout(stdout);

    // Step 14: Save the completed receiver log layout into receiver_output.txt
    FILE *fpOut = fopen("receiver_output.txt", "w");
    if (fpOut != NULL) {
        printReceiverLayout(fpOut);
        fclose(fpOut);
    }

    return 0;
}
