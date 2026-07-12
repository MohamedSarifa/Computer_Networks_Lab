#include <string.h>
#include <stdbool.h>

int countPalindromicSubsequence(char* s) {
    int n = strlen(s);
    int first[26];
    int last[26];
    for (int i = 0; i < 26; i++) {
        first[i] = -1;
        last[i] = -1;
    }

    for (int i = 0; i < n; i++) {
        int idx = s[i] - 'a';
        if (first[idx] == -1) {
            first[idx] = i;
        }
        last[idx] = i;
    }
    
    int total_unique_palindromes = 0;

    for (int i = 0; i < 26; i++) {
        if (first[i] != -1 && last[i] > first[i] + 1) {
       
            bool seen[26] = {false};
            int unique_count = 0;
            
            for (int j = first[i] + 1; j < last[i]; j++) {
                int middle_idx = s[j] - 'a';
                if (!seen[middle_idx]) {
                    seen[middle_idx] = true;
                    unique_count++;
                
                    if (unique_count == 26) {
                        break;
                    }
                }
            }
            total_unique_palindromes += unique_count;
        }
    }
    
    return total_unique_palindromes;
}
