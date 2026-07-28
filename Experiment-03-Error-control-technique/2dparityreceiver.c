#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 64
#define BITS_PER_CHAR 8

int main() {
    FILE *inFile = fopen("matrix_output.txt", "r");
    if (!inFile) {
        printf("⚠️ Error: Could not open matrix_output.txt. Please run 2d_parity_sender first!\n");
        return 1;
    }

    int rows = 0, cols = 0;
    char line[128];
    while (fgets(line, sizeof(line), inFile)) {
        if (strstr(line, "Rows =")) {
            sscanf(line, "Rows = %d", &rows);
        } else if (strstr(line, "Cols =")) {
            sscanf(line, "Cols = %d", &cols);
        } else if (strstr(line, "Matrix:")) {
            break;
        }
    }
    if (rows <= 0 || cols <= 0) {
        printf("Error: Invalid matrix dimensions in matrix_output.txt.\n");
        fclose(inFile);
        return 1;
    }
    int matrix[MAX_CHARS + 1][BITS_PER_CHAR + 1];
    for (int i = 0; i <= rows; i++) {
        if (fgets(line, sizeof(line), inFile)) {
            line[strcspn(line, "\r\n")] = 0;
            for (int j = 0; j <= cols; j++) {
                matrix[i][j] = line[j] - '0';
            }
        }
    }
    fclose(inFile);

    printf("==========================================\n");
    printf("        2D PARITY RECEIVER & TEST         \n");
    printf("==========================================\n");
    printf("Received Matrix Dimensions: %d Rows x %d Columns (Data + Parity)\n\n", rows + 1, cols + 1);

    printf("Received Matrix:\n");
    for (int i = 0; i <= rows; i++) {
        for (int j = 0; j <= cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
    char simChoice = 'n';
    printf("\nDo you want to simulate a transmission error? (y/n): ");
    scanf(" %c", &simChoice);

    if (simChoice == 'y' || simChoice == 'Y') {
        int numFlips = 1;
        printf("Enter number of bits to flip (1 for Single-bit, >1 for Multiple): ");
        scanf("%d", &numFlips);

        for (int k = 1; k <= numFlips; k++) {
            int errRow = 0, errCol = 0;
            while (1) {
                printf("\n[Flip %d/%d] Enter row number to modify (Range: 1 to %d): ", k, numFlips, rows + 1);
                scanf("%d", &errRow);
                if (errRow >= 1 && errRow <= rows + 1) break;
                printf("Invalid row! Must be between 1 and %d.\n", rows + 1);
            }
            while (1) {
                printf("[Flip %d/%d] Enter column number to modify (Range: 1 to %d): ", k, numFlips, cols + 1);
                scanf("%d", &errCol);
                if (errCol >= 1 && errCol <= cols + 1) break;
                printf("Invalid column! Must be between 1 and %d.\n", cols + 1);
            }
            // Flip the bit
            matrix[errRow - 1][errCol - 1] = (matrix[errRow - 1][errCol - 1] == 0) ? 1 : 0;
            printf("*** Bit at Row %d, Column %d flipped ***\n", errRow, errCol);
        }
    }

    printf("\n==========================================\n");
    printf("           RECEIVER PROCESSING            \n");
    printf("==========================================\n");

    int recalculatedRowParity[MAX_CHARS + 1] = {0};
    int recalculatedColParity[BITS_PER_CHAR + 1] = {0};

    int rowErrors[MAX_CHARS + 1] = {0};
    int colErrors[BITS_PER_CHAR + 1] = {0};

    int failedRowIndex = -1, failedRowPointerCount = 0;
    int failedColIndex = -1, failedColPointerCount = 0;
    for (int i = 0; i < rows; i++) {
        int onesCount = 0;
        for (int j = 0; j < cols; j++) {
            if (matrix[i][j] == 1) onesCount++;
        }
        recalculatedRowParity[i] = (onesCount % 2 != 0) ? 1 : 0;
    }
    for (int j = 0; j < cols; j++) {
        int onesCount = 0;
        for (int i = 0; i < rows; i++) {
            if (matrix[i][j] == 1) onesCount++;
        }
        recalculatedColParity[j] = (onesCount % 2 != 0) ? 1 : 0;
    }
    int recalculatedCornerParity = 0;
    int colOnesCount = 0;
    for (int j = 0; j < cols; j++) {
        if (recalculatedColParity[j] == 1) {
            colOnesCount++;
        }
    }
    recalculatedCornerParity = (colOnesCount % 2 != 0) ? 1 : 0;

    printf("Matrix with Recalculated Parity Bits:\n");

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("[%d]\n", recalculatedRowParity[i]);
    }

    for (int j = 0; j < cols; j++) {
        printf("[%d] ", recalculatedColParity[j]);
    }
    printf("[%d] <-- Recalculated Column & Corner Parity\n\n", recalculatedCornerParity);

    printf("Row Parity check:\n");
    for (int i = 0; i < rows; i++)
    {
        if (recalculatedRowParity[i] != matrix[i][cols])
        {
            rowErrors[i] = 1;
            failedRowIndex = i;
            failedRowPointerCount++;
            printf("Row %d: FAIL\n", i + 1);
        }
        else
        {
            printf("Row %d: PASS\n", i + 1);
        }
    }

    printf("\nColumn Parity check:\n");
    for (int j = 0; j < cols; j++)
    {
        if (recalculatedColParity[j] != matrix[rows][j])
        {
            colErrors[j] = 1;
            failedColIndex = j;
            failedColPointerCount++;
            printf("Column %d: FAIL\n", j + 1);
        }
        else
        {
            printf("Column %d: PASS\n", j + 1);
        }
    }

    printf("\nCorner Parity check:\n");
    if (recalculatedCornerParity != matrix[rows][cols]) {
        printf("Corner Parity: FAIL\n");
    } else {
        printf("Corner Parity: PASS\n");
    }

    int canExtractData = 1;
    if (failedRowPointerCount == 0 && failedColPointerCount == 0) {
        printf("\n✅ Data received without error.\n");
    }
    else if (failedRowPointerCount == 1 && failedColPointerCount == 1) {
        printf("\nParity error isolated at Row %d, Column %d.\n", failedRowIndex + 1, failedColIndex + 1);
        matrix[failedRowIndex][failedColIndex] = (matrix[failedRowIndex][failedColIndex] == 0) ? 1 : 0;
        printf("🔧 Single-bit error corrected automatically at position [%d, %d]!\n", failedRowIndex + 1, failedColIndex + 1);

        printf("\nCorrected Data Matrix:\n");
        for (int i = 0; i <= rows; i++) {
            for (int j = 0; j <= cols; j++) {
                printf("%d ", matrix[i][j]);
            }
            printf("\n");
        }
    }
    else {
        printf("\n❌ Multiple errors detected.\n");
        printf("Cannot be corrected automatically.\n");
        canExtractData = 0;
    }
    if (canExtractData) {
        printf("\nRecovered Message: ");
        for (int i = 0; i < rows; i++) {
            unsigned char ch = 0;
            for (int j = 0; j < cols; j++) {
                ch = (ch << 1) | matrix[i][j];
            }
            printf("%c", ch);
        }
        printf("\n");
    }

    return 0;
}
