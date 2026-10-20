int classify(int val) {
    int status = 0;
    switch (val) {
        case 1:
            status = 10;
            break;
        case 2:
            status = 20;
            break;
        default:
            status = -1;
            break;
    }
    int result = (status > 0) ? status : 0;
    return result;
}
