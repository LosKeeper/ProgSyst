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

int main(int arg, char **argv) {

    // Ouverture du repertoire
    DIR *repertoire = opendir(argv[1]);
    if (repertoire == NULL) {
        raler(1, "repertoire");
    }
    struct dirent *infos;

    // Lecture des noms du repertoire
    while ((infos = readdir(repertoire)) != NULL) {
        if ((strcmp(infos->d_name, ".") != 0) &&
            (strcmp(infos->d_name, "..") != 0)) {
            printf("%s\n", infos->d_name);
        }
        errno = 0;
    }

    if (errno != 0) {
        raler(1, "repertoire");
    }
    // Fermeture du fichier
    CHK(closedir(repertoire));

    return 0;
}