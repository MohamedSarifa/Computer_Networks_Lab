#include <stdio.h>
#include <string.h>

#define MAX_BITS 100

int main() {
    FILE *inFile = fopen("channel.txt", "r");
    if (!inFile) {
        printf("⚠️ Error: Could not open channel.txt. Please run the sender first!\n");
        return 1;
    }

    int m = 0, r = 0;
    char codeStr[MAX_BITS] = "";
    char line[MAX_BITS];

    while (fgets(line, sizeof(line), inFile)) {
        if (strstr(line, "m =")) {
            sscanf(line, "m = %d", &m);
        } else if (strstr(line, "r =")) {
            sscanf(line, "r = %d", &r);
        } else if (strstr(line, "Code =")) {
            sscanf(line, "Code = %s", codeStr);
        }
    }
    fclose(inFile);

    int n = m + r;
    int recvBits[MAX_BITS];

    int strLen = strlen(codeStr);
    for (int i = 0; i < strLen; i++) {
        recvBits[strLen - i] = codeStr[i] - '0';
    }

    printf("==========================================\n");
    printf("   HAMMING CODE RECEIVER & ERROR TEST     \n");
    printf("==========================================\n");

    char simChoice = 'n';
    printf("Do you want to simulate a transmission error? (y/n): ");
    scanf(" %c", &simChoice);

    if (simChoice == 'y' || simChoice == 'Y') {
        int errorPos = 0;

        while (1) {
            printf("Enter bit position to flip (Range 1 to %d, from Right-to-Left): ", n);
            scanf("%d", &errorPos);

            if (errorPos >= 1 && errorPos <= n) {
                break;
            }
            printf(" Invalid selection! Bit position must be between 1 and %d.\n", n);
        }

        recvBits[errorPos] = (recvBits[errorPos] == 0) ? 1 : 0;
        printf("\n*** Transmission Error Simulated at Position %d ***\n", errorPos);
    }

    printf("\n==========================================\n");
    printf("            RECEIVER PROCESSING           \n");
    printf("==========================================\n");

    printf("Received Hamming Code : ");
    for (int i = n; i >= 1; i--) {
        printf("%d", recvBits[i]);
    }
    printf("\n");

    int syndrome = 0;
    for (int i = 0; i < r; i++) {
        int pPos = (1 << i);
        int parityCount = 0;

        for (int j = 1; j <= n; j++) {
            if (j & pPos) {
                parityCount += recvBits[j];
            }
        }

        if (parityCount % 2 != 0) {
            syndrome += pPos;
        }
    }

    if (syndrome == 0) {
        printf("\nStatus: No Error Detected.\n");
    } else {
        printf("\n Status: Error Detected at Bit Position %d (from Right)\n", syndrome);

        recvBits[syndrome] = (recvBits[syndrome] == 0) ? 1 : 0;
        printf("Corrected Hamming Code: ");
        for (int i = n; i >= 1; i--) {
            printf("%d", recvBits[i]);
        }
        printf("\n");
    }

    printf("\nExtracted Original Data: ");
    for (int i = n; i >= 1; i--) {
        if ((i & (i - 1)) != 0) {
            printf("%d", recvBits[i]);
        }
    }
    printf("\n");

    return 0;
}

[24bcs145@mepcolinux EX3]$cat hamsender.c
#include <stdio.h>
#include <string.h>

#define MAX_BITS 100

int main() {
    char dataStr[MAX_BITS];
    int dataBits[MAX_BITS];
    int codeBits[MAX_BITS];
    int m = 0, r = 0, n = 0;

    printf("==========================================\n");
    printf("         HAMMING CODE (SENDER)            \n");
    printf("==========================================\n");

    printf("Enter binary data bits (e.g., 1011): ");
    scanf("%s", dataStr);

    m = strlen(dataStr);
    for (int i = 0; i < m; i++) {
        dataBits[i] = dataStr[i] - '0';
    }

    while ((1 << r) < (m + r + 1)) {
        r++;
    }

    n = m + r;

    int dataIdx = m - 1;
    for (int i = 1; i <= n; i++) {
        if ((i & (i - 1)) == 0) {
            codeBits[i] = 0;
        } else {
            codeBits[i] = dataBits[dataIdx--];
        }
    }

    for (int i = 0; i < r; i++) {
        int pPos = (1 << i);
        int parityCount = 0;

        for (int j = 1; j <= n; j++) {
            if (j & pPos) {
                if (j != pPos) {
                    parityCount += codeBits[j];
                }
            }
        }
        codeBits[pPos] = (parityCount % 2 == 0) ? 0 : 1;
    }

    printf("\nData length (m) : %d\n", m);
    printf("Parity bits (r) : %d\n", r);
    printf("Total Length (n): %d\n", n);
    printf("Generated Code  : ");
    for (int i = n; i >= 1; i--) {
        printf("%d", codeBits[i]);
    }
    printf("\n");

    FILE *outFile = fopen("channel.txt", "w");
    if (!outFile) {
        printf("⚠️ Error opening file channel.txt for writing.\n");
        return 1;
    }

    fprintf(outFile, "m = %d\n", m);
    fprintf(outFile, "r = %d\n", r);
    fprintf(outFile, "Code = ");
    for (int i = n; i >= 1; i--) {
        fprintf(outFile, "%d", codeBits[i]);
    }
    fprintf(outFile, "\n");
    fclose(outFile);

    printf("\nTransmission sent! Saved output to 'channel.txt'.\n");
    return 0;
}
