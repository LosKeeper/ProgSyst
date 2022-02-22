#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <unistd.h>
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
        raler(0, "usage: %s <nb_sta>", argv[0]);
    }

    // Test nombre de stations
    int nb_sta = atoi(argv[1]);
    if (nb_sta < 1 || nb_sta > MAXSTA) {
        raler(0, "probleme nombre de stations");
    }

    // Tableau de tubes
    int tab_pipe[MAXSTA + 1][2];

    // Génération du tube commun
    CHK(pipe(tab_pipe[0]));

    // Génération des processus fils
    for (int k = 1; k < nb_sta + 1; k++) {

        // Génération des tubes père fils
        CHK(pipe(tab_pipe[k]));

        // Génération des processus fils
        switch (fork()) {
        case -1:
            raler(1, "fork");

        case 0:
            // Fermeture des tubes non nécessaires pour le fils
            CHK(close(tab_pipe[0][0]));
            for (int i = 1; i < k; i++) {
                CHK(close(tab_pipe[i][0]));
                CHK(close(tab_pipe[i][1]));
            }
            CHK(close(tab_pipe[k][1]));
            exit(0);
        }
    }

    // Fermeture des tubes non nécessaires pour le père
    CHK(close(tab_pipe[0][1]));
    for (int j = 1; j < nb_sta + 1; j++) {
        CHK(close(tab_pipe[j][0]));
    }

    printf("coucou\n");
}