#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
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

void sousRepertoire(DIR *repertoire, )

    int main(int argc, char **argv) {
    // Récupération des dossiers dans lesquels chercher
    // char *path = getenv("PATH");
    // int ch = snprintf();

    // Ouverture du repertoire
    DIR *repertoire = opendir(argv[1]);
    if (repertoire == NULL) {
        raler(1, "repertoire");
    }
    struct dirent *infos;

    while ((infos = readdir(repertoire)) != 0) {
    }
}