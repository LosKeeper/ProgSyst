#include <fcntl.h>
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

    if (argc != 2) {
        raler(1, "Nombre d'arguments");
    }

    switch (fork()) {
    case -1:
        raler(1, "fork");

    case 0:
        close(1);
        int fd = open("toto", O_WRONLY | O_CREAT, 0666);
        CHK(fd);
        CHK(dup2(fd, 1));
        CHK(close(fd));
        execlp("ps", "eaux", NULL);
        raler(1, "execlp");
    }

    switch (fork()) {
    case -1:
        raler(1, "fork");

    case 0:
        close(0);
        int fd = open("toto", O_WRONLY | O_CREAT, 0666);
        CHK(fd);
        CHK(dup2(fd, 0));
        CHK(close(fd));
        execlp("grep", "^$1", NULL);
        raler(1, "execlp");
    }
}