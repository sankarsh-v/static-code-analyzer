int test_empty_bodies(int a) {
    if (a > 0);
    if (a > 1) {}

    if (a > 2) {
        a++;
    } else;

    if (a > 0) {
        a++;
    } else {}

    while (a > 1);
    while (a > 2) {}

    for (int i = 0; i < 2; i++);
    for (int j = 0; j < 2; j++) {}

    if (a > 0) {
        a++;
    } else {
        a--;
    }

    while (a > 0) {
        a--;
    }

    for (int k = 0; k < 2; k++) {
        a += k;
    }

    if (a < 0) {
        for (;;);
    }

    return a;
}
