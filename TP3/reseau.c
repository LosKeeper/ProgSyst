#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#define MAXSTA 10
#define PAYLOAD_SIZE 4

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

    // Test du nombre d'arguments
    if (argc != 2) {
        raler(0, "nombre d'arguments");
    }

    // Test nombre de stations
    int nb_sta = atoi(argv[1]);
    if (nb_sta < 1 || nb_sta > MAXSTA) {
        raler(0, "nombre de stations");
    }

    // Tableau de pipes
    int tab_pipe[MAXSTA + 1][2];

    // Génération des processus fils
    for (int k = 0; k < nb_sta; k++) {
        switch (fork()) {
        case -1:
            raler(1, "fork");

        case 0:
        }
    }
}