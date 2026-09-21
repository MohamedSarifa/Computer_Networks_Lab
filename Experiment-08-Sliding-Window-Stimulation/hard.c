#include <stdlib.h>
#include <string.h>

int compare_ints(const void* a, const void* b) {
    int arg1 = *(const int*)a;
    int arg2 = *(const int*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

int find_index(int* arr, int size, int val) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == val) return mid;
        if (arr[mid] < val) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

int insert_index(int* arr, int size, int val) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] < val) low = mid + 1;
        else high = mid - 1;
    }
    return low;
}

double* medianSlidingWindow(int* nums, int numsSize, int k, int* returnSize) {
    *returnSize = numsSize - k + 1;
    double* result = (double*)malloc(sizeof(double) * (*returnSize));
    int* window = (int*)malloc(sizeof(int) * k);
    
    for (int i = 0; i < k; i++) {
        window[i] = nums[i];
    }
    qsort(window, k, sizeof(int), compare_ints);
    
    if (k % 2 == 1) {
        result[0] = (double)window[k / 2];
    } else {
        result[0] = ((double)window[k / 2 - 1] + (double)window[k / 2]) / 2.0;
    }
    
    for (int i = k; i < numsSize; i++) {
        int out_val = nums[i - k];
        int in_val = nums[i];
        
        int del_idx = find_index(window, k, out_val);
        memmove(&window[del_idx], &window[del_idx + 1], (k - del_idx - 1) * sizeof(int));
        
        int ins_idx = insert_index(window, k - 1, in_val);
        memmove(&window[ins_idx + 1], &window[ins_idx], (k - ins_idx - 1) * sizeof(int));
        window[ins_idx] = in_val;
        
        if (k % 2 == 1) {
            result[i - k + 1] = (double)window[k / 2];
        } else {
            result[i - k + 1] = ((double)window[k / 2 - 1] + (double)window[k / 2]) / 2.0;
        }
    }
    
    free(window);
    return result;
}
