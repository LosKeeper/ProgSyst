#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

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

volatile sig_atomic_t signal_term = 0;
volatile sig_atomic_t signal_int = 0;

void signal_hander(int signum) {
    if (signum == SIGTERM)
        signal_term = 1;
    else if (signum == SIGINT)
        signal_int = 1;
}

int main(void) {
    int sortie;
    CHK(sortie = open("sortie.txt", O_WRONLY | O_CREAT | O_APPEND, 066));

    struct sigaction s;
    sigset_t signal_old, signal_new;
    struct timeval tv;

    uint32_t compteur = 0;
    int stop = 0;
    while (!stop) {
        // Masquage
        // sigemptyset(&signal_new);
        // sigaddset(&signal_new, SIGINT);
        // CHK(sigprocmask(SIG_BLOCK, &signal_new, &signal_old) == -1);

        // Critique
        compteur++;
        CHK(gettimeofday(&tv, NULL));
        if (signal_int) {
            write(sortie, compteur);
            signal_int = 0;
        }
        if (signal_term) {
            write(sortie, "fin");
            stop = 1;
        }

        // Demasquage
        // CHK(sigprocmask(SIG_SETMASK, &signal_old, NULL) == -1);
    }

    CHK(close(sortie));
}
