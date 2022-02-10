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

    if (argc != 2) {
        raler(1, "nombre d'arguments");
    }

    struct timeval timev;
    struct timezone timez;
    time_t t1;
    time_t t2;
    int raison;

    CHK(gettimeofday(&timev, &timez));
    t1 = timev.tv_usec;

    fprintf(stdout, "Heure : %ju secondes, %ju microsecondes\n",
            (uintmax_t)timev.tv_sec, (uintmax_t)timev.tv_usec);

    switch (fork()) {
    case -1:
        raler(1, "fork");

    case 0:
        execl("/bin/ls", "ls", "-l", argv[1], NULL);
        raler(1, "execl");

    default:
        CHK(wait(&raison));
        if (!WIFEXITED(raison)) {
            raler(1, "wait");
        }

        CHK(gettimeofday(&timev, &timez));
        t2 = timev.tv_usec;

        fprintf(stdout, "Heure : %ju secondes, %ju microsecondes\n",
                (uintmax_t)timev.tv_sec, (uintmax_t)timev.tv_usec);
        fprintf(stdout, "Temps écoulé : %ju microsecondes\n",
                (uintmax_t)(t2 - t1));
    }
}