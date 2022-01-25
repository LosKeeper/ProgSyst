#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
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

    if (argc != 3) {
        raler(1, "nombre d'arguments");
    }

    int raison;
    intmax_t n = atoi(argv[1]);
    intmax_t m = atoi(argv[2]);

    if ((n <= 0) | (m <= 1)) {
        raler(1, "arguments incorrects");
    }

    uintmax_t *tab = malloc(n * sizeof(uintmax_t));
    if (!tab) {
        raler(1, "malloc");
    }
    for (intmax_t k = 0; k < n; k++) {
        tab[k] = (uintmax_t)(m * rand() / RAND_MAX);
    }

    for (intmax_t j = 0; j < n; j++) {
        switch (fork()) {
        case -1:
            raler(1, "fork");

        case 0:
            sleep(tab[j]);
            free(tab);
            exit(tab[j]);
        }
    }

    free(tab);

    for (intmax_t k = 0; k < n; k++) {
        CHK(wait(&raison));
        if (WIFEXITED(raison)) {
            printf("Code retour : %ju\n", (intmax_t)WEXITSTATUS(raison));
        }
    }

    return 0;
}