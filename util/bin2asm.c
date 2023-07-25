#include <stdio.h>

int main(int argc, char *argv[]) {
    unsigned char buf[4096];
    size_t amt;
    size_t i;
    int col = 0;
    char *name;
    if (argc != 2) {
        fprintf(stderr, "usage: %s NAME < DAT_FILE > ASM_FILE\n", argv[0]);
        for (i=0; i<argc; i++) {
            fprintf(stderr, " '%s'", argv[i]);
        }
        fprintf(stderr, "\n");
        return 1;
    }

    name = argv[1];
    printf("\
.globl %s_start\n\
	.section .rodata\n\
	.align 8\n\
%s_start:\n\
", name, name);

    while (! feof(stdin)) {
        amt = fread(buf, 1, sizeof(buf), stdin);
        for (i = 0; i < amt; i++) {
            if (col == 0) {
                printf(".byte ");
            }
            printf("0x%02x", buf[i]);
            col++;
            if (i == amt -1 && feof(stdin)) {
                continue;
            }
            if (col == 16) {
                printf("\n");
                col = 0;
            } else if (col % 4 == 0) {
                printf(", ");
            } else {
                printf(",");
            }
        }
    }
    if (col != 0) {
        printf("\n");
    }

    printf("\
.globl %s_end\n\
	.section .rodata\n\
	.align 8\n\
%s_end:\n\
", name, name);

    return 0;
}