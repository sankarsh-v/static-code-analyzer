int test_unsafe(char *buf, char *src, char *dest, int *num) {
    gets(buf);
    strcpy(dest, src);
    strcat(dest, src);
    sprintf(dest, "%s", src);
    scanf("%s", buf);

    fgets(buf, 2, src);
    strncpy(dest, src, 2);
    strncat(dest, src, 2);
    snprintf(dest, 2, "%s", src);
    scanf("%31s", buf);
    scanf("%d", num);

    return 0;
}
