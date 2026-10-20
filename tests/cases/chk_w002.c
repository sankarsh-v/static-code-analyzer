int test_conditions(int a, int b) {
    if (a = 1) {
        b++;
    }
    while (b = 1) {
        a++;
    }
    for (int i = 0; i = 1; i++) {
        a++;
    }

    if (a == 1) {
        b++;
    }
    if ((a = 1)) {
        b++;
    }
    if ((a = 1) != 0) {
        b++;
    }
    return a + b;
}
