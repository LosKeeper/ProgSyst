#include <signal.h>
#include <stdarg.h>
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

pid_t pid_fils;
int signal_fin = 0;

void traite(int signum) {
    (void)signum;
    printf("Coucou\n");
}

void kill_fils(int signum) {
    signal_fin = 1;
    printf("Fin du programme\n");
    CHK(kill(pid_fils, SIGUSR1));
}

int main(void) {

    pid_fils = fork();
    int raison;
    switch (pid_fils) {
    case -1:
        raler(1, "fork");
    case 0:
        if (signal(SIGALRM, traite) == SIG_ERR) {
            raler(1, "signal");
        }
        while (signal_fin == 0) {
            unsigned int alarme_fils = alarm(1);
            pause();
        }
        printf("Message du père reçu\n");
        exit(0);
    default:
        if (signal(SIGALRM, kill_fils) == SIG_ERR) {
            raler(1, "signal");
        }
        unsigned int alarme_pere = alarm(10);
        CHK(wait(&raison));
    }
}