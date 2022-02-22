#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

#define CHK(op)                                                                \
    do {                                                                       \
        if ((op) == -1)                                                        \
            raler(1, #op);                                                     \
    } while (0)

noreturn void raler(int syserr, const char *msg, ...) {
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    if (syserr == 1)
        perror("");

    exit(EXIT_FAILURE);
}

int main(void) {

    int tube1[2];
    CHK(pipe(tube1));

    switch (fork()) {
    case -1:
        raler(1, "fork");

    case 0:

    default:
    }

    int tube2[2];
    CHK(pipe(tube2));

    switch (fork()) {
    case -1:
        raler(1, "fork");

    case 0:

    default:
    }
}