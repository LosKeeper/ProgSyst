#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <unistd.h>

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

int main(int argc, char **argv) {

    if (argc != 2) {
        raler(0, "nombre d'arguments");
    }

    int tube1[2];
    CHK(pipe(tube1));

    switch (fork()) {
    case -1:
        raler(1, "fork");

    case 0:
        CHK(close(tube1[0]));
        CHK(dup2(1, tube1[1]));
        CHK(close(tube1[1]));
        execlp("ps", "eaux", NULL);
    }

    int tube2[2];
    CHK(pipe(tube2));

    switch (fork()) {
    case -1:
        raler(1, "fork");

    case 0:
        dup2(0, tube1[0]);
        dup2(1, tube2[1]);
        execlp("grep", argv[1]);

    default:
    }
}
