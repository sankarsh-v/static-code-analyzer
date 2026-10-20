int test_fall_through(int val) {
    int a = 0;
    switch (val) {
        case 1:
            a = 1;
        case 2:
            return 2;
        case 0:
        case -1:
            a = 0;
            break;
        default:
            a = 0;
            break;
    }

    switch (val) {
        case 0:
            a = 1;
            break;
        default:
            a = 2;
            break;
        case 1:
            a = 0;
    }

    return a;
}
