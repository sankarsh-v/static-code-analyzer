int g_unused = 100;

int test_unused(int param_unused) {
    int unused_var;
    int assigned_only;
    assigned_only = 1;
    int used_var = 1;
    return used_var;
}
