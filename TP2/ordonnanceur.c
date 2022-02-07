#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/types.h>
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

int stop = 0;

void election(intmax_t process_id) {
    printf("SURP - process %ju\n", process_id);
}

void terminaison(intmax_t process_id) {
    printf("TERM - process %ju\n", process_id);
}

void eviction(intmax_t process_id) {
    printf("EVIP - process %ju\n", process_id);
}

void process(int temps) { (void)temps; }

int main(int argc, char **argv) {

    int fin = 0;

    // Test nb arguments
    if (argc < 3) {
        raler(0, "arguments");
    }

    intmax_t nb_process = (intmax_t)argc - 2;
    intmax_t duree_qtum = (intmax_t)atoi(argv[1]);

    // Tests durée quantums et process
    if (duree_qtum < 1) {
        raler(0, "quantum");
    }
    for (intmax_t k = 2; k < nb_process + 2; k++) {
        if (atoi(argv[k]) < 1) {
            raler(0, "process");
        }
    }

    // Tableau des pid de tous les processus fils
    pid_t *process_id = malloc(sizeof(pid_t) * nb_process);

    // Masques pere et fils (par héritage)

    for (intmax_t k = 0; k < nb_process; k++) {
        process_id[k] = fork();

        switch (process_id[k]) {
        case -1:
            raler(1, "fork");

        case 0:
            pause();
            process(atoi(argv[k + 2]));
            exit(0);
        }
    }

    // Tant que tous les fils ne sont pas finis
    while (fin == 0) {
        for (intmax_t k = 0; k < nb_process; k++) {
            // Mettre prgm k
            // Attente SIGALARM OU SIGCHLD
            // Si SIGCHLD -> process k a ne plus considérer
        }
    }

    return 0;
}