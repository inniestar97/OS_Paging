#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *x;
    x = (char *) malloc(sizeof(char) * 3);
    x[1] = 'a';
    x[2] = 'A';

    int i = 0;
    for (i = 0; i < 8; i++) {
        printf("%d", (x[0] & (0x80 >> i)) >> (7 - i));
    }
    printf("\n");

    for (i = 0; i < 8; i++) {
        printf("%d", (x[1] & (0x80 >> i)) >> (7 - i));
    }
    printf("\n");

    for (i = 0; i < 8; i++) {
        printf("%d", (x[2] & (0x80 >> i)) >> (7 - i));
    }
    printf("\n");

    return 0;
}