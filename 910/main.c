#include <stdarg.h>
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

void envoyer(pid_t recepteur, int octet) {}

void preparer_recepetion(void) { pause(); }

int recevoir(pid_t emetteur) {}

int main(int argc, char **argv) {
    int raison;
    pid_t pere = fork();
    switch (pere) {
    case -1:
        raler(1, "fork");

    case 0:

    default:
        wait(&raison);
    }
}