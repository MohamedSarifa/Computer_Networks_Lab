#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char raw_stream[5000];

char syn1_bin[9], syn2_bin[9], soh_bin[9];
char srcMac_bin[40], destMac_bin[40];
char srcIp_bin[40], destIp_bin[40];
char srcPort_bin[20], destPort_bin[20];
char stx_bin[9], etx_bin[9], crc_bin[9];

char receivedStuffedData[1000];
char deStuffedData[500];

char decodedSrcMac[30], decodedDestMac[30];
char decodedSrcIp[30], decodedDestIp[30];
int decodedSrcPort = 0, decodedDestPort = 0;

// Helper to trim newline characters
void trimNewline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

// Helper to copy a specific segment of bits and null-terminate it
void sliceBits(const char *src, int start, int length, char *dest) {
    strncpy(dest, src + start, length);
    dest[length] = '\0';
}

// Converts an 8-bit binary string into a single character string representation
char binToChar(const char *bin8) {
    char val = 0;
    for (int i = 0; i < 8; i++) {
        val = (val << 1) | (bin8[i] - '0');
    }
    return val;
}

// Helper to convert any standard character string into an 8-bit binary space-separated block layout
void textToBin(const char *text, char *binOut) {
    binOut[0] = '\0';
    char byteBin[10];
    int len = (int)strlen(text);
    for (int i = 0; i < len; i++) {
        unsigned char c = text[i];
        for (int j = 7; j >= 0; j--) {
            byteBin[7 - j] = (c & (1 << j)) ? '1' : '0';
        }
        byteBin[8] = '\0';
        strcat(binOut, byteBin);
        if (i < len - 1) {
            strcat(binOut, " ");
        }
    }
}

// Decodes standard grouped binary representations to MAC layout
void decodeMac(const char *binStream, char *macOut) {
    macOut[0] = '\0';
    for (int i = 0; i < 4; i++) { // 32 bits total = 4 bytes
        char sub[9];
        sliceBits(binStream, i * 8, 8, sub);
        unsigned char byteVal = binToChar(sub);
        char segment[5];
        sprintf(segment, "%02X", byteVal);
        strcat(macOut, segment);
        if (i < 3) strcat(macOut, ":");
    }
}

// Decodes binary representations into standard dotted IP layout
void decodeIp(const char *binStream, char *ipOut) {
    int octets[4];
    for (int i = 0; i < 4; i++) {
        char sub[9];
        sliceBits(binStream, i * 8, 8, sub);
        octets[i] = (unsigned char)binToChar(sub);
    }
    sprintf(ipOut, "%d.%d.%d.%d", octets[0], octets[1], octets[2], octets[3]);
}

// Decodes a 16-bit binary block to a Port short integer
int decodePort(const char *binStream) {
    int port = 0;
    for (int i = 0; i < 16; i++) {
        port = (port << 1) | (binStream[i] - '0');
    }
    return port;
}

// Reads the pure raw string from frame.txt and slices it contextually
void readAndParseRawStream() {
    FILE *fp = fopen("frame.txt", "r");
    if (fp == NULL) {
        printf("Error: Could not open input file 'frame.txt'\n");
        exit(1);
    }
    if (fgets(raw_stream, sizeof(raw_stream), fp) == NULL) {
        printf("Error: 'frame.txt' is empty\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    trimNewline(raw_stream);
    size_t len = strlen(raw_stream);

    // Slice based on exact bit offsets
    sliceBits(raw_stream, 0, 8, syn1_bin);
    sliceBits(raw_stream, 8, 8, syn2_bin);
    sliceBits(raw_stream, 16, 8, soh_bin);

    // Header (Starts at index 24, length 224 bits)
    sliceBits(raw_stream, 24, 32, srcMac_bin);
    sliceBits(raw_stream, 56, 32, destMac_bin);
    sliceBits(raw_stream, 88, 32, srcIp_bin);
    sliceBits(raw_stream, 120, 32, destIp_bin);
    sliceBits(raw_stream, 152, 16, srcPort_bin);
    sliceBits(raw_stream, 168, 16, destPort_bin);

    sliceBits(raw_stream, 184, 8, stx_bin);

    // Calculate Data payload slice sizing dynamically
    int dataStart = 192;
    int dataLength = (int)len - dataStart - 16; // Leaving trailing ETX (8 bits) and CRC (8 bits)

    char rawDataBits[4000];
    sliceBits(raw_stream, dataStart, dataLength, rawDataBits);

    sliceBits(raw_stream, dataStart + dataLength, 8, etx_bin);
    sliceBits(raw_stream, dataStart + dataLength + 8, 8, crc_bin);

    // Translate the binary sequence data bits back into text characters
    int characterIndex = 0;
    for (int i = 0; i < dataLength; i += 8) {
        char singleByte[9];
        sliceBits(rawDataBits, i, 8, singleByte);
        receivedStuffedData[characterIndex++] = binToChar(singleByte);
    }
    receivedStuffedData[characterIndex] = '\0';
}

void characterDeStuffing() {
    int i = 0;
    int j = 0;
    int len = (int)strlen(receivedStuffedData);

    while (i < len) {
        // Look for string sequences matching stuffed representations
        if (i <= len - 7 && strncmp(&receivedStuffedData[i], "DLE ETX", 7) == 0) {
            deStuffedData[j++] = 'E'; deStuffedData[j++] = 'T'; deStuffedData[j++] = 'X';
            i += 7;
        }
        else if (i <= len - 7 && strncmp(&receivedStuffedData[i], "DLE DLE", 7) == 0) {
            deStuffedData[j++] = 'D'; deStuffedData[j++] = 'L'; deStuffedData[j++] = 'E';
            i += 7;
        }
        else {
            deStuffedData[j++] = receivedStuffedData[i++];
        }
    }
    deStuffedData[j] = '\0';
}

void printReceiverLayout(FILE *stream) {
    char stuffedBinary[5000];
    char originalBinary[5000];

    textToBin(receivedStuffedData, stuffedBinary);
    textToBin(deStuffedData, originalBinary);

    fprintf(stream, "====================================================\n");
    fprintf(stream, "        BISYNC RECEIVER OUTPUT PROCESSING           \n");
    fprintf(stream, "====================================================\n\n");

    fprintf(stream, "--- Decoded Header Fields ---\n");
    fprintf(stream, "a) Source MAC Address      : %s\n", decodedSrcMac);
    fprintf(stream, "b) Destination MAC Address : %s\n", decodedDestMac);
    fprintf(stream, "c) Source IP Address       : %s\n", decodedSrcIp);
    fprintf(stream, "d) Destination IP Address  : %s\n", decodedDestIp);
    fprintf(stream, "e) Source Port Number      : %d\n", decodedSrcPort);
    fprintf(stream, "f) Destination Port Number : %d\n\n", decodedDestPort);

    fprintf(stream, "--- Data Processing Fields ---\n");
    fprintf(stream, "Received Stuffed Data (Text):\n%s\n\n", receivedStuffedData);
    fprintf(stream, "Received Stuffed Data (Binary):\n%s\n\n", stuffedBinary);

    fprintf(stream, "De-stuffed Original Message (Text):\n%s\n\n", deStuffedData);
    fprintf(stream, "De-stuffed Original Message (Binary):\n%s\n\n", originalBinary);

    fprintf(stream, "--- Complete Binary Frame Configuration Structure ---\n");
    fprintf(stream, "SYN : %s\n", syn1_bin);
    fprintf(stream, "SYN : %s\n", syn2_bin);
    fprintf(stream, "SOH : %s\n", soh_bin);
    fprintf(stream, "HEADER (Src MAC)  : %s\n", srcMac_bin);
    fprintf(stream, "HEADER (Dest MAC) : %s\n", destMac_bin);
    fprintf(stream, "HEADER (Src IP)   : %s\n", srcIp_bin);
    fprintf(stream, "HEADER (Dest IP)  : %s\n", destIp_bin);
    fprintf(stream, "HEADER (Src Port) : %s\n", srcPort_bin);
    fprintf(stream, "HEADER (Dest Port): %s\n", destPort_bin);
    fprintf(stream, "STX : %s\n", stx_bin);
    fprintf(stream, "DATA: %s\n", stuffedBinary);
    fprintf(stream, "ETX : %s\n", etx_bin);
    fprintf(stream, "CRC : %s\n\n", crc_bin);

    fprintf(stream, "--- Frame Parity Verification ---\n");
    if (strcmp(crc_bin, "00000000") == 0) {
        fprintf(stream, "Result: Frame Received Successfully.\n");
    } else {
        fprintf(stream, "Result: Frame Corrupted.\n");
    }
}

int main() {
    readAndParseRawStream();

    // Process Header Conversions
    decodeMac(srcMac_bin, decodedSrcMac);
    decodeMac(destMac_bin, decodedDestMac);
    decodeIp(srcIp_bin, decodedSrcIp);
    decodeIp(destIp_bin, decodedDestIp);
    decodedSrcPort = decodePort(srcPort_bin);
    decodedDestPort = decodePort(destPort_bin);

    characterDeStuffing();

    // Output directly to Terminal console layout
    printReceiverLayout(stdout);

    // Mirror identical output to target file text format
    FILE *fpOut = fopen("receiver_output.txt", "w");
    if (fpOut != NULL) {
        printReceiverLayout(fpOut);
        fclose(fpOut);
    }

    return 0;
}
