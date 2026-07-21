#include <stdio.h>
#include <string.h>

#define MAX_BITS 30000
#define MAX_FRAMES 500
#define MAX_STR 128
#define HEADER_BITS 8

static char frameStreams[MAX_FRAMES][MAX_BITS];
static char line[MAX_BITS];
static char workData[MAX_BITS];

void computeCRC(const char *inputBits, const char *generator, char *remainder) {
    int genLen = strlen(generator);
    int inputLen = strlen(inputBits);

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
    FILE *inFile = fopen("output.txt", "r");
    if (!inFile) {
        printf(" Error: Could not open output.txt\n");
        return 1;
    }

    int expectedFrames = 0;
    char generator[MAX_STR] = "";

    while (fgets(line, sizeof(line), inFile)) {
        char *pGen = strstr(line, "Generator =");
        if (pGen != NULL) {
            pGen += 11;
            while (*pGen == ' ' || *pGen == '\t') pGen++;
            int idx = 0;
            while (pGen[idx] != '\0' && pGen[idx] != '\n' && pGen[idx] != '\r' && pGen[idx] != ' ') {
                generator[idx] = pGen[idx];
                idx++;
            }
            generator[idx] = '\0';
        }

        char *pNum = strstr(line, "frames =");
        if (!pNum) pNum = strstr(line, "Frames =");

        if (pNum != NULL) {
            char *eq = strchr(pNum, '=');
            if (eq != NULL) {
                eq++;
                while (*eq == ' ' || *eq == '\t') eq++;
                expectedFrames = 0;
                while (*eq >= '0' && *eq <= '9') {
                    expectedFrames = expectedFrames * 10 + (*eq - '0');
                    eq++;
                }
            }
            break;
        }
    }

    int genDegree = strlen(generator) - 1;
    int totalFramesRead = 0;

    while (fgets(line, sizeof(line), inFile) && totalFramesRead < MAX_FRAMES) {
        char *delim = strstr(line, "||");
        if (!delim) continue;

        delim += 2;
        while (*delim == ' ' || *delim == '\t') delim++;

        int len = strlen(delim);
        while (len > 0 && (delim[len - 1] == '\n' || delim[len - 1] == '\r')) {
            delim[len - 1] = '\0';
            len--;
        }

        strcpy(frameStreams[totalFramesRead], delim);
        totalFramesRead++;
    }
    fclose(inFile);

    if (totalFramesRead == 0) {
        printf("⚠️ Error: No frames found in output.txt.\n");
        return 1;
    }

    char simChoice = 'n';
    printf("Do you want to simulate a transmission error? (y/n): ");
    scanf(" %c", &simChoice);

    if (simChoice == 'y' || simChoice == 'Y') {
        int targetFrame = 0;
        int bitPos = 0;

        while (1) {
            printf("Enter frame number to modify (Range: 1 to %d): ", totalFramesRead);
            scanf("%d", &targetFrame);

            if (targetFrame >= 1 && targetFrame <= totalFramesRead) {
                break;
            }
            printf(" Invalid frame! Please choose a number between 1 and %d.\n", totalFramesRead);
        }

        int frameIdx = targetFrame - 1;
        int frameLen = strlen(frameStreams[frameIdx]);
        int dataStartBit = HEADER_BITS + 1;
        int crcStartBit = frameLen - genDegree + 1;

        printf("\nFrame #%d Layout Breakdown:\n", targetFrame);
        printf("  -> Total Bits  : %d\n", frameLen);
        printf("  -> Header Range: Bits 1 to %d\n", HEADER_BITS);
        printf("  -> Data Range  : Bits %d to %d\n", dataStartBit, crcStartBit - 1);
        printf("  -> CRC Range   : Bits %d to %d\n", crcStartBit, frameLen);

        while (1) {
            printf("Enter bit position to flip (Range: %d to %d): ", dataStartBit,crcStartBit);
            scanf("%d", &bitPos);

            if (bitPos >=dataStartBit  && bitPos <crcStartBit) {
                break;
            }
            printf(" Invalid bit position! Choose a position between 1 and %d.\n", frameLen);
        }

        const char *regionName = "HEADER";
        if (bitPos >= crcStartBit) {
            regionName = "CRC";
        } else if (bitPos >= dataStartBit) {
            regionName = "DATA";
        }

        int bitIdx = bitPos - 1;
        frameStreams[frameIdx][bitIdx] = (frameStreams[frameIdx][bitIdx] == '0') ? '1' : '0';

        printf("\n*** Transmission Error Simulated ***\n");
        printf("Frame #%d : Bit %d flipped (%s region).\n", targetFrame, bitPos, regionName);
    }

    printf("\n");
    int i,d;
    for ( i = 0; i < totalFramesRead; i++) {
        char crcCheckRemainder[MAX_STR] = {0};
        computeCRC(frameStreams[i], generator, crcCheckRemainder);

        int errorDetected = 0;
        for (d = 0; d < genDegree; d++) {
            if (crcCheckRemainder[d] != '0') {
                errorDetected = 1;
                break;
            }
        }

        printf("Frame #%02d [CRC: %s]\n", i + 1, errorDetected ? "FAIL" : "PASS");
    }

    return 0;
}
