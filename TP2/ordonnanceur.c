#include <stdarg.h>
#include <stdint.h>
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

int terminaison = 0;

int main(int argc, char **argv) {
    if (argc < 3) {
        raler(0, "arguments");
    }
    uintmax_t duree_qtum = (uintmax_t)atoi(argv[1]);
    uintmax_t nb_process = (uintmax_t)argc - 2;

    for (uintmax_t k = 0; k < nb_process; k++) {
        int raison;
        switch (fork()) {
        case -1:
            raler(1, "fork");

        case 0:
            while (!terminaison) {
                sleep(1);
            }
            exit(0);
        }
    }
}