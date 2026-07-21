#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BITS 20000
#define MAX_STR 128

struct RouteEntry {
    char url[MAX_STR];
    char mac[MAX_STR];
    char ip[MAX_STR];
    int port;
};

struct RouteEntry routingTable[2] = {
    {"google.com",      "00:1A:2B:3C", "192.168.1.10", 36602},
    {"youtube.com", "00:AA:BB:CC", "10.0.0.5",     48686}
};

static char binMessage[MAX_BITS];
static char transportSegment[MAX_BITS];
static char packetPayloads[200][MAX_BITS];
static int  packetActualPayloadLengths[200];
static int  totalPacketsGenerated = 0;
static char globalFrames[500][MAX_BITS];
static int  totalFramesGenerated = 0;

void textToBinStr(const char *input, char *output) {
    output[0] = '\0';
    int i,b;
    for (i = 0; input[i] != '\0'; i++) {
        for ( b = 7; b >= 0; b--) {
            strcat(output, ((input[i] >> b) & 1) ? "1" : "0");
        }
    }
}

void portToBinStr(int port, char *output) {
    output[0] = '\0';
    int b;
    for (b = 15; b >= 0; b--) {
        strcat(output, ((port >> b) & 1) ? "1" : "0");
    }
}

void fieldToBinStr(const char *type, const char *value, char *output) {
    output[0] = '\0';
    if (strcmp(type, "MAC") == 0) {
        unsigned char bytes[4];
        unsigned int m[4];
        int i,b;
        sscanf(value, "%x:%x:%x:%x", &m[0], &m[1], &m[2], &m[3]);
        for (i = 0; i < 4; i++) bytes[i] = (unsigned char)m[i];
        for (i = 0; i < 4; i++) {
            for (b = 7; b >= 0; b--) strcat(output, ((bytes[i] >> b) & 1) ? "1" : "0");
        }
    } else if (strcmp(type, "IP") == 0) {
        unsigned char bytes[4];
        unsigned int ip[4];
        int j,b;
        sscanf(value, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
        for (j = 0; j < 4; j++) bytes[j] = (unsigned char)ip[j];
        int k;
        for (k = 0; k < 4; k++) {
            for (b = 7; b >= 0; b--) strcat(output, ((bytes[k] >> b) & 1) ? "1" : "0");
        }
    }
}

void parsePolyToBin(const char *polyStr, char *binPoly) {
    if (strstr(polyStr, "x^3+x+1") != NULL || strstr(polyStr, "x3+x+1") != NULL) {
        strcpy(binPoly, "1011");
    } else if (strstr(polyStr, "x^4+x+1") != NULL) {
        strcpy(binPoly, "10011");
    } else if (strstr(polyStr, "10") != NULL) {
        strcpy(binPoly, polyStr);
    } else {
        strcpy(binPoly, "1011");
    }
}

void computeCRC(const char *inputBits, const char *generator, char *remainder) {
    int genLen = strlen(generator);
    int inputLen = strlen(inputBits);
    char workData[MAX_BITS];
    strcpy(workData, inputBits);
    int i,j;
    for (i = 0; i <= inputLen - genLen; i++) {
        if (workData[i] == '1') {
            for (j = 0; j < genLen; j++) {
                workData[i + j] = (workData[i + j] == generator[j]) ? '0' : '1';
            }
        }
    }
    strncpy(remainder, workData + (inputLen - genLen + 1), genLen - 1);
    remainder[genLen - 1] = '\0';
}

int main() {
    printf("========================================================================\n");
    printf("                        SENDER INITIALIZATION & CONFIG                   \n");
    printf("========================================================================\n");
    int i;
    for (i = 0; i < 2; i++) {
        printf("URL: %-15s | MAC: %s | IP: %-12s | Port: %d\n",
               routingTable[i].url, routingTable[i].mac, routingTable[i].ip, routingTable[i].port);
    }
    printf("========================================================================\n\n");

    char srcUrl[MAX_STR], destUrl[MAX_STR], filename[MAX_STR], polyInput[MAX_STR], generator[MAX_STR];
    int packetPayloadSize = 16, framePayloadSize = 16;

    printf("--- Input Operational Directives ---\n");
    printf("Enter Source URL            : "); fgets(srcUrl, MAX_STR, stdin); srcUrl[strcspn(srcUrl, "\n\r")] = 0;
    printf("Enter Destination URL       : "); fgets(destUrl, MAX_STR, stdin); destUrl[strcspn(destUrl, "\n\r")] = 0;
    printf("Enter Input File Name       : "); fgets(filename, MAX_STR, stdin); filename[strcspn(filename, "\n\r")] = 0;
    printf("Enter Packet Payload Size   : "); scanf("%d", &packetPayloadSize);
    printf("Enter Frame Payload Size    : "); scanf("%d", &framePayloadSize);
    while (getchar() != '\n');
    printf("Enter Generator Polynomial  : "); fgets(polyInput, MAX_STR, stdin); polyInput[strcspn(polyInput, "\n\r")] = 0;

    parsePolyToBin(polyInput, generator);
    int genDegree = strlen(generator) - 1;

    struct RouteEntry *srcRoute = &routingTable[0];
    struct RouteEntry *destRoute = &routingTable[1];

    FILE *inFile = fopen(filename, "r");
    char rawMessage[500] = "NET";
    if (inFile != NULL) {
        if(fgets(rawMessage, sizeof(rawMessage), inFile) != NULL) rawMessage[strcspn(rawMessage, "\n\r")] = 0;
        fclose(inFile);
    }

    textToBinStr(rawMessage, binMessage);
    int textLen = strlen(rawMessage);
    int textBits = textLen * 8;
    printf("\n----------------------------------------------------------\n");
    printf("STEP 1: APPLICATION LAYER [SENDER]\n");
    printf("----------------------------------------------------------\n");
    printf("  Original Message Text   : %s\n", rawMessage);
    printf("  Message Metrics         : Total Characters = %d | Stream Size = %d bits\n", textLen, textBits);
    printf("  Generated App Bitstream : %s\n", binMessage);

    char binSrcPort[17], binDestPort[17];
    portToBinStr(srcRoute->port, binSrcPort);
    portToBinStr(destRoute->port, binDestPort);

    transportSegment[0] = '\0';
    strcat(transportSegment, binSrcPort);
    strcat(transportSegment, binDestPort);
    strcat(transportSegment, binMessage);

    printf("\n----------------------------------------------------------\n");
    printf("STEP 2: TRANSPORT LAYER [SENDER]\n");
    printf("----------------------------------------------------------\n");
    printf("  TCP Header Entry -> Source Port      : %s (%d)\n", binSrcPort, srcRoute->port);
    printf("                   -> Destination Port : %s (%d)\n", binDestPort, destRoute->port);
    printf("  TCP Segment Header Size              : 32 bits total\n");
    printf("  Full Outbound Transport Segment      : %s\n", transportSegment);

    char binSrcIp[33], binDestIp[33];
    fieldToBinStr("IP", srcRoute->ip, binSrcIp);
    fieldToBinStr("IP", destRoute->ip, binDestIp);

    printf("\n----------------------------------------------------------\n");
    printf("STEP 3: NETWORK LAYER [SENDER - PACKETIZATION]\n");
    printf("----------------------------------------------------------\n");

    int segmentLen = strlen(transportSegment);
    int segmentOffset = 0;
    totalPacketsGenerated = 0;

    while (segmentOffset < segmentLen) {
        int remainingBits = segmentLen - segmentOffset;
        int currentChunkSize = (remainingBits < packetPayloadSize) ? remainingBits : packetPayloadSize;

        strncpy(packetPayloads[totalPacketsGenerated], transportSegment + segmentOffset, currentChunkSize);
        packetPayloads[totalPacketsGenerated][currentChunkSize] = '\0';
        packetActualPayloadLengths[totalPacketsGenerated] = currentChunkSize;

        printf("  Packet #%d Layout Configuration:\n", totalPacketsGenerated + 1);
        printf("     -> Requested Target Payload Size : %d bits\n", packetPayloadSize);
        printf("     -> ACTUAL Raw Segment Extracted  : %d bits\n", currentChunkSize);
        printf("     -> IP Base Envelope Header Size  : 64 bits (Src IP + Dest IP)\n");
        printf("     -> Total Envelope Packet Size    : %d bits\n", currentChunkSize + 64);
        printf("     -> IP Header Stream [Src/Dest]   : %s%s\n", binSrcIp, binDestIp);
        printf("     -> Pure Data Payload Bitstream   : %s\n", packetPayloads[totalPacketsGenerated]);
        printf("  --------------------------------------------------------\n");

        segmentOffset += currentChunkSize;
        totalPacketsGenerated++;
    }

    char binSrcMac[33], binDestMac[33];
    fieldToBinStr("MAC", srcRoute->mac, binSrcMac);
    fieldToBinStr("MAC", destRoute->mac, binDestMac);

    printf("\n----------------------------------------------------------\n");
    printf("STEP 4: DATA LINK LAYER [SENDER - FRAMING]\n");
    printf("----------------------------------------------------------\n");
    printf("  Generator Choice : %s | Modulo-2 Pattern Divisor: %s (Degree: %d)\n", polyInput, generator, genDegree);
    printf("----------------------------------------------------------\n");

    totalFramesGenerated = 0;
    int p;
    for (p = 0; p < totalPacketsGenerated; p++) {
        int pPayloadLen = strlen(packetPayloads[p]);
        int payloadOffset = 0;
        int frameNumberInPacket = 1;

        while (payloadOffset < pPayloadLen) {
            int remainingPayloadBits = pPayloadLen - payloadOffset;
            int actualBits = (remainingPayloadBits < framePayloadSize) ? remainingPayloadBits : framePayloadSize;

            char framePayload[MAX_BITS];
            strncpy(framePayload, packetPayloads[p] + payloadOffset, actualBits);
            framePayload[actualBits] = '\0';

            char paddingStr[MAX_BITS] = "";
            int paddingBits = 0;
            int pad;
            if (actualBits < framePayloadSize) {
                paddingBits = framePayloadSize - actualBits;
                for (pad = 0; pad < paddingBits; pad++) strcat(paddingStr, "0");
            }

            char frameHeaderAndPayload[MAX_BITS] = "";
            strcat(frameHeaderAndPayload, binSrcMac);
            strcat(frameHeaderAndPayload, binDestMac);
            strcat(frameHeaderAndPayload, framePayload);
            strcat(frameHeaderAndPayload, paddingStr);

            char crcMathDividend[MAX_BITS];
            strcpy(crcMathDividend, frameHeaderAndPayload);
            int dg;
            for (dg = 0; dg < genDegree; dg++) strcat(crcMathDividend, "0");

            char crcRemainder[128];
            computeCRC(crcMathDividend, generator, crcRemainder);

            globalFrames[totalFramesGenerated][0] = '\0';
            strcat(globalFrames[totalFramesGenerated], frameHeaderAndPayload);
            strcat(globalFrames[totalFramesGenerated], crcRemainder);

            int frameHeaderSize = 64;
            int completeFrameSize = frameHeaderSize + actualBits + paddingBits + genDegree;

            printf("  Global Frame #%d (Parent Packet #%d, Local Sequence #%d):\n",
                   totalFramesGenerated + 1, p + 1, frameNumberInPacket);
            printf("    [Header Specs] Src MAC: %s | Dest MAC: %s\n", binSrcMac, binDestMac);
            printf("    [Header Size ] %d bits  | Payload Data Size: %d bits\n", frameHeaderSize, actualBits);
            printf("    [Tail Adjust ] Padding: %d bits  | Concise CRC Output: %s (%d bits)\n", paddingBits, crcRemainder, genDegree);
            printf("    [Total Frame ] Aggregated Frame Footprint: %d bits\n", completeFrameSize);
            printf("    [Full Output ] %s\n", globalFrames[totalFramesGenerated]);
            printf("  --------------------------------------------------------\n");

            payloadOffset += actualBits;
            frameNumberInPacket++;
            totalFramesGenerated++;
        }
    }

    printf("\n----------------------------------------------------------\n");
    printf("STEP 5: PHYSICAL LAYER [TRANSMISSION MEDIUM]\n");
    printf("----------------------------------------------------------\n");

    FILE *outFile = fopen("output.txt", "w");
    if (outFile != NULL) {
        fprintf(outFile, "Frame payload = %d\n", framePayloadSize);
        fprintf(outFile, "Packet payload = %d\n", packetPayloadSize);
        fprintf(outFile, "Generator = %s\n", generator);
        fprintf(outFile, "Number of frames = %d\n", totalFramesGenerated);

        const char *preambleSFD = "10101011";
        int i;
        for (i = 0; i < totalFramesGenerated; i++) {
            printf("  Physically Streaming Outbound Frame #%02d -> %s || %s\n",
                   i + 1, preambleSFD, globalFrames[i]);
            fprintf(outFile, "%s || %s\n", preambleSFD, globalFrames[i]);
        }
        fclose(outFile);
        printf("\nTransmission Pipeline Completed Successfully.\n");
        printf("Metadata header parameters and transmission matrices stored inside output.txt\n");
    } else {
        printf("Error opening output.txt for writing!\n");
    }
    printf("========================================================================\n");

    return 0;
}
