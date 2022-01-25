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

void sig_handler(int signum) { psignal(signum, NULL); }

int main(void) {
    pid_t pid = getpid();
    printf("UID = %u\n", (uint)pid);
    for (int k = 2; k < 32; k++) {
        if (k != SIGKILL && k != SIGSTOP && signal(k, sig_handler) == SIG_ERR) {
            raler(1, "signal");
        }
    }
    pause();
}