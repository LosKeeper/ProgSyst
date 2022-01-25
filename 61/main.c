#include <errno.h>
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

int main(void) {

    pid_t pid;
    pid_t pid_fils;
    int raison;
    pid = fork();

    switch (pid) {
    case -1:
        raler(1, "fork");

    case 0:
        pid = getppid();
        pid_fils = getpid();
        printf("Le processus fils a pour PID : %jd\n", (intmax_t)pid_fils);
        printf("Le processus pere a pour PID : %jd\n", (intmax_t)pid);
        exit(pid_fils % 10);

    default:
        printf("Le processus fils a pour PID : %jd\n", (intmax_t)pid);
        pid = wait(&raison);
        CHK(pid);
        if (WIFEXITED(raison)) {
            printf("Code retour : %ju\n", (intmax_t)WEXITSTATUS(raison));
        }
        break;
    }
}