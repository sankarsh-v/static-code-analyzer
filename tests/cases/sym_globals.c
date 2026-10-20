int g_counter = 0;
int g_max = 100;

void increment(void) {
    g_counter++;
}

int get_status(void) {
    if (g_counter >= g_max) {
        return 1;
    }
    return 0;
}
