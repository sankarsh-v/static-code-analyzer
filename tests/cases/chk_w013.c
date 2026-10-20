int test_division_by_zero(int x, int y) {
    int a = x / 0;
    int b = x % 0;
    x /= 0;
    x %= 0;

    int c = x / 1;
    int d = x % 2;
    int e = x / y;
    x /= 2;
    x %= 2;

    return a + b + c + d + e + x;
}
