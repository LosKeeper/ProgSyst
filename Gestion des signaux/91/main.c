#include <signal.h>
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

volatile sig_atomic_t compteur = 0;

void incrementer(int signum) {
    (void)signum;
    compteur++;
    fprintf(stdout, "Compteur = %u\n", compteur);
}

int main(void) {

    if (signal(SIGINT, incrementer) == SIG_ERR) {
        raler(1, "signal");
    }

    while (compteur < 5) {
        pause();
    }
}