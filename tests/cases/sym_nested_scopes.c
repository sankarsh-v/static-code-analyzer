int compute(int limit) {
    int total = 0;
    for (int i = 0; i < limit; i++) {
        int step = 2;
        {
            int inner = i * step;
            total += inner;
        }
    }
    return total;
}
