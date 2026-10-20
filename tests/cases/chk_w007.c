int test_deep_nesting(int a, int b, int c, int d) {
    int res = 0;
    if (a > 0) {
        while (b > 0) {
            for (int i = 0; i < 2; i++) {
                if (c > 0) {
                    res += d;
                }
            }
        }
    }
    return res;
}

int test_shallow_nesting(int a, int b, int c) {
    int res = 0;
    if (a > 0) {
        while (b > 0) {
            if (c > 0) {
                res += a + b + c;
            }
        }
    }
    return res;
}
