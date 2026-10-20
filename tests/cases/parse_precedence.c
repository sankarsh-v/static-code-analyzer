int test_precedence(int a, int b, int c, int d, int x, int y, int z) {
    int res1 = a + b * c;
    int res2 = a = b = c;
    int res3 = x && y || z;
    int res4 = a < b == c > d;
    return res1 + res2 + res3 + res4;
}
