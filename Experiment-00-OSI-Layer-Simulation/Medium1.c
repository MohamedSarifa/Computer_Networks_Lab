int myAtoi(const char *s) {
    int i = 0;
    int sign = 1;
    long long num = 0;
    
    long long INT_MIN_VAL = -2147483648LL;
    long long INT_MAX_VAL = 2147483647LL;

    while (s[i] == ' ') {
        i++;
    }

    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }

    while (s[i] != '\0' && s[i] >= '0' && s[i] <= '9') {
        num = num * 10 + (s[i] - '0');
        
        if (num > INT_MAX_VAL + 1) {
            break;
        }
        i++;
    }

    long long result = sign * num;

    if (result < INT_MIN_VAL) {
        return (int)INT_MIN_VAL;
    }
    if (result > INT_MAX_VAL) {
        return (int)INT_MAX_VAL;
    }

    return (int)result;
}
