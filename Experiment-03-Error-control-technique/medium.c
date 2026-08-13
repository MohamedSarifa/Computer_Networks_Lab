#include <stdio.h>
int totalHammingDistance(int* nums, int numsSize) {
    int total_distance = 0;
    for (int i = 0; i < 32; i++) {
        int ones_count = 0;
        for (int j = 0; j < numsSize; j++) {
            ones_count += (nums[j] >> i) & 1;
        }
        int zeros_count = numsSize - ones_count;
        total_distance += ones_count * zeros_count;
    }
    return total_distance;
}
