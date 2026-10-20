int test_use_before_assign(void) {
    int uninit_var;
    int res = uninit_var + 1;
    uninit_var = 1;
    return res + uninit_var;
}

int test_init_at_decl(void) {
    int init_var = 100;
    return init_var + 1;
}

int test_assigned_before_read(void) {
    int assigned_var;
    assigned_var = 1;
    return assigned_var + 1;
}
