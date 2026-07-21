#include <stdio.h>
#include <string.h>

int count_ones(const char* binary_str)
{
    int count = 0;
    int i;
    for (i = 0; binary_str[i] != '\0'; i++)
    {
        if (binary_str[i] == '1')
        {
            count++;
        }
    }
    return count;
}

void char_to_7bit_binary(char c, char* output)
{
    int i;

    for (i = 6; i >= 0; i--)
    {
        output[6 - i] = ((c >> i) & 1) ? '1' : '0';
    }
    output[7] = '\0';
}

int main()
{
    char text[50];
    char binary_blocks[50][9];
    int total_chars;
    int j;
    char choice;
    printf("Enter a text string (e.g., Hello):\n");
    scanf("%49[^\n]", text);

    total_chars = strlen(text);

    printf("\nGenerating Odd Parity:\n");
    printf("-----------------------------------------\n");
    printf("Idx\tChar\t7-Bit\tParity\t8-Bit Result\n");
    printf("-----------------------------------------\n");

    for (j = 0; j < total_chars; j++)
    {
        char binary7[8];
        char parity_bit;

        char_to_7bit_binary(text[j], binary7);
        strcpy(binary_blocks[j], binary7);

        parity_bit = (count_ones(binary7) % 2 != 0) ? '0' : '1';
        binary_blocks[j][7] = parity_bit;
        binary_blocks[j][8] = '\0';

        printf("[%d]\t'%c'\t%s\t%c\t%s\n", j, text[j], binary7, parity_bit, binary_blocks[j]);
    }
    printf("-----------------------------------------\n");

    printf("\nWhether you like to change a bit? (y/n): ");
    scanf(" %c", &choice);

    if (choice == 'y' || choice == 'Y')
    {
        int char_idx, bit_pos;

        printf("Enter the character index (0 to %d): ", total_chars - 1);
        scanf("%d", &char_idx);

        if (char_idx >= 0 && char_idx < total_chars)
        {
            printf("Which bit position (1 to 8) do you want to change in %s? ", binary_blocks[char_idx]);
            scanf("%d", &bit_pos);

            if (bit_pos >= 1 && bit_pos <= 8)
            {
                int target_index = bit_pos - 1;
                binary_blocks[char_idx][target_index] = (binary_blocks[char_idx][target_index] == '1') ? '0' : '1';

                printf("\nModified 8-bit string for index [%d]: %s\n", char_idx, binary_blocks[char_idx]);
            }
            else
            {
                printf("Invalid bit position! Exiting.\n");
                return 1;
            }
        }
        else
        {
            printf("Invalid character index! Exiting.\n");
            return 1;
        }
    }
    else
    {
        printf("No changes made. Exiting program.\n");
        return 0;
    }

    printf("\nChecking Parity of all blocks:\n");
    printf("-----------------------------------------\n");
    printf("Idx\t8-Bit String\tStatus\n");
    printf("-----------------------------------------\n");
    for (j = 0; j < total_chars; j++)
    {
        printf("[%d]\t%s\t", j, binary_blocks[j]);
        if (count_ones(binary_blocks[j]) % 2 != 0)
        {
            printf("Success: Odd Parity maintained\n");
        }
        else
        {
            printf("Error: NOT Odd Parity! (Bit error detected)\n");
        }
    }
    printf("-----------------------------------------\n");

    return 0;
}
