int count = 0;

int test_shadow_param(int count) {
    int val = 1;
    if (val > 0) {
        int val = 2;
        count += val;
    }
    return count + val;
}

int test_no_shadow(int a) {
    int b = 1;
    if (a > 0) {
        int c = 2;
        b += c;
    }
    return a + b;
}
