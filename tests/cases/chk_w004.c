int missing_return_if(int x) {
    if (x > 0) {
        return 1;
    }
}

int no_return_at_all(int a) {
    a++;
}

void void_procedure(int x) {
    x++;
}

int full_return(int x) {
    if (x > 0) {
        return 1;
    } else {
        return 0;
    }
}

int server_loop(void) {
    while (1) {
        ;
    }
}
