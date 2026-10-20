int g_exempt_init = 1000;

int test_magic_numbers(int param, int *arr) {
    int local_init = 500;
    int x = 0;
    int y = 1;
    int z = 2;
    int n = -1;

    x = 42;
    if (param > 100) {
        y = -10;
    }
    arr[5] = 1;

    double f = 0.0;
    f = 3.14;

    arr[0] = 0;
    arr[1] = 1;
    arr[2] = 2;
    arr[param] = -1;

    if (f > 0.0) {
        return x + y + z + n + local_init + g_exempt_init;
    }
    return 0;
}
