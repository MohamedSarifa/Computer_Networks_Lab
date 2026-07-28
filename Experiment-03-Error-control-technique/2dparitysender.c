#include <stdio.h>
#include <string.h>

#define MAX_CHARS 64
#define BITS_PER_CHAR 8

int main() {
    char inputStr[MAX_CHARS];

    printf("==========================================\n");
    printf("        2D PARITY SENDER (CHARACTER)       \n");
    printf("==========================================\n");

    printf("Enter string input (e.g., HELLO): ");
    if (fgets(inputStr, sizeof(inputStr), stdin) == NULL) {
        return 1;
    }

    int strLen = strlen(inputStr);
    while (strLen > 0 && (inputStr[strLen - 1] == '\n' || inputStr[strLen - 1] == '\r')) {
        inputStr[strLen - 1] = '\0';
        strLen--;
    }

    if (strLen == 0) {
        printf("⚠️ Error: Input string cannot be empty.\n");
        return 1;
    }

    int rows = strLen;
    int cols = BITS_PER_CHAR;

    int matrix[MAX_CHARS + 1][BITS_PER_CHAR + 1];

    for (int i = 0; i < rows; i++) {
        unsigned char ch = (unsigned char)inputStr[i];
        int rowOnes = 0;

        for (int j = 0; j < cols; j++) {
            int bit = (ch >> (7 - j)) & 1;
            matrix[i][j] = bit;
            if (bit == 1) {
                rowOnes++;
            }
        }

        matrix[i][cols] = (rowOnes % 2 != 0) ? 1 : 0;
    }

    for (int j = 0; j <= cols; j++) {
        int colOnes = 0;
        for (int i = 0; i < rows; i++) {
            if (matrix[i][j] == 1) {
                colOnes++;
            }
        }
        matrix[rows][j] = (colOnes % 2 != 0) ? 1 : 0;
    }

    printf("\nGenerated 2D Parity Matrix:\n\n");
    printf("Char | Data Bits (8-bit) | Row Parity\n");
    printf("-----|-------------------|-----------\n");

    for (int i = 0; i < rows; i++) {
        printf("  %c  | ", inputStr[i]);
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("|     %d\n", matrix[i][cols]);
    }

    printf("-----|-------------------|-----------\n");
    printf("CPar | ");
    for (int j = 0; j < cols; j++) {
        printf("%d ", matrix[rows][j]);
    }
    printf("|     %d (Corner Parity)\n\n", matrix[rows][cols]);

    FILE *outFile = fopen("matrix_output.txt", "w");
    if (!outFile) {
        printf("⚠️ Error opening output file.\n");
        return 1;
    }

    fprintf(outFile, "Rows = %d\n", rows);
    fprintf(outFile, "Cols = %d\n", cols);
    fprintf(outFile, "Matrix:\n");

    for (int i = 0; i <= rows; i++) {
        for (int j = 0; j <= cols; j++) {
            fprintf(outFile, "%d", matrix[i][j]);
        }
        fprintf(outFile, "\n");
    }

    fclose(outFile);
    printf(" Transmitted! 2D Parity Matrix saved to 'matrix_output.txt'.\n");

    return 0;
}
