int x = 10;

int main(void) {
    int x = 20;
    x = x + 1;
    {
        int x = 30;
        x = x * 2;
    }
    return x;
}
