#include <stdlib.h>
int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}
int subarrayBitwiseORs(int* arr, int arrSize) {
    int* global_results = (int*)malloc(arrSize * 31 * sizeof(int));
    int global_size = 0;
    int current_or[31];
    int current_size = 0;
    for (int i = 0; i < arrSize; i++) {
        int num = arr[i];
        int next_or[31];
        int next_size = 0;
        next_or[next_size++] = num;
        for (int j = 0; j < current_size; j++) {
            int new_val = current_or[j] | num;
            if (new_val != next_or[next_size - 1]) {
                next_or[next_size++] = new_val;
            }
        }
        current_size = 0;
        for (int j = 0; j < next_size; j++) {
            current_or[current_size++] = next_or[j];
            global_results[global_size++] = next_or[j];
        }
    }
    qsort(global_results, global_size, sizeof(int), compare);
    if (global_size == 0) {
        free(global_results);
        return 0;
    }
    int unique_count = 1;
    for (int i = 1; i < global_size; i++) {
        if (global_results[i] != global_results[i - 1]) {
            unique_count++;
        }
    }
    free(global_results);
    return unique_count;
}
