#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUFFER_SIZE 512

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

void copier(int fdsrc, int fddst) {
    char *buffer[BUFFER_SIZE];
    ssize_t n;
    while ((n = read(fdsrc, buffer, BUFFER_SIZE)) > 0) {
        CHK(write(fddst, buffer, n));
    }
}

int main(void) {

    int tube[2];
    CHK(pipe(tube));
    switch (fork()) {

    case -1:
        raler(1, "fork");

    case 0:
        close(tube[1]);
        copier(tube[0], 1);
        close(tube[0]);
        exit(0);

    default:
        close(tube[0]);
        copier(0, tube[1]);
        close(tube[1]);
        CHK(wait(NULL));
        exit(0);
    }
}
