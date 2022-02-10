#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
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

    if (argc < 2) {
        raler(0, "Nombre d'arguments incorrects");
    }

    int raison;
    pid_t pid;
    pid_t pid_fils;
    int n = atoi(argv[1]);

    for (int k = 0; k < n; k++) {
        pid = fork();
        switch (pid) {
        case -1:
            raler(1, "fork");
        case 0:
            pid_fils = getpid();
            exit(pid_fils % 10);
        }
    }

    for (int k = 0; k < n; k++) {
        pid = wait(&raison);
        CHK(pid);
        printf("Le PID est : %ju\n", (intmax_t)pid);
        if (WIFEXITED(raison)) {
            printf("Code retour : %ju\n\n", (intmax_t)WEXITSTATUS(raison));
        }
    }
}