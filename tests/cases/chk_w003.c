int test_ret(int x) {
    return x;
    x++;
}

int test_ifelse(int cond) {
    if (cond) {
        return 1;
    } else {
        return 2;
    }
    int dead = 10;
    return dead;
}

int test_reachable(int cond) {
    int y = 0;
    if (cond) {
        y = 1;
    } else {
        return 0;
    }
    y++;
    return y;
}
