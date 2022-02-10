#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/stat.h>
#include <sys/types.h>
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
    // Récupération données structure
    struct stat infos;
    CHK(stat(argv[1], &infos));

    // Vérification du type
    printf("TYPE :              ");
    switch (infos.st_mode & S_IFMT) {
    case S_IFBLK:
        printf("Mode Block\n");
        break;
    case S_IFCHR:
        printf("Mode Caractère\n");
        break;
    case S_IFDIR:
        printf("Fichier\n");
        break;
    case S_IFIFO:
        printf("FIFO/pipe\n");
        break;
    case S_IFLNK:
        printf("Lien Symbolique\n");
        break;
    case S_IFREG:
        printf("Fichier Régulier\n");
        break;
    case S_IFSOCK:
        printf("socket\n");
        break;
    default:
        printf("Inconnu\n");
        break;
    }

    // Vérification des permissions
    printf("Permissions :       %o\n", infos.st_mode & 0777);

    return 0;
}