int test_switch_no_default(int val) {
    int res = 0;
    switch (val) {
        case 1:
            res = 1;
            break;
        case 2:
            res = 2;
            break;
    }
    return res;
}

int test_switch_with_default(int val) {
    int res = 0;
    switch (val) {
        case 1:
            res = 1;
            break;
        case 2:
            res = 2;
            break;
        default:
            res = 0;
            break;
    }
    return res;
}
