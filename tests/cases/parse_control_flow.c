int test(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        if (i % 2 == 0)
            if (i > 5) sum += i;
            else sum += 1;
    }
    while (sum > 100) {
        sum -= 10;
    }
    do {
        sum++;
    } while (sum < 150);
    return sum;
}
