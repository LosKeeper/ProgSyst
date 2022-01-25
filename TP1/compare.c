#include <fcntl.h>
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

int main(int argc, char **argv) {

    // Test du nombre d'arguments

    if (argc != 3) {
        fprintf(stderr, "Number of arguments incorect\n");
        return 1;
    }

    // Ouverture des fichiers

    int entree1 = open(argv[1], O_RDONLY);
    CHK(entree1);
    int entree2 = open(argv[2], O_RDONLY);
    CHK(entree2);

    char c1;
    char c2;
    __uintmax_t byte = 0;
    __uintmax_t line = 1;

    // Lecture et comparasion des caractères des fichiers

    while (read(entree1, &c1, 1)) {
        if (read(entree2, &c2, 1)) {
            byte++;
            if (c1 != c2) {
                fprintf(stderr, "%s %s differ: byte %ju, line %ju\n", argv[1],
                        argv[2], byte, line);
                return 1;
            } else if (c1 == '\n') {
                line++;
            }
        } else {
            if (!byte) {
                fprintf(stderr, "EOF on %s which is empty\n", argv[2]);
                CHK(close(entree1));
                CHK(close(entree2));
                return 1;
            }
            fprintf(stderr, "EOF on %s after byte %ju, line %ju\n", argv[2],
                    byte, line);
            CHK(close(entree1));
            CHK(close(entree2));
            return 1;
        }
    }

    // Cas du fichier 1 vide ou fini

    if (read(entree1, &c1, 1)) {
        if (!byte) {
            fprintf(stderr, "EOF on %s which is empty\n", argv[1]);
            CHK(close(entree1));
            CHK(close(entree2));
            return 1;
        }
        fprintf(stderr, "EOF on %s after byte %ju, line %ju\n", argv[1], byte,
                line);
        CHK(close(entree1));
        CHK(close(entree2));
        return 1;

        // Cas du fichier 2 vide ou fini

    } else if (read(entree2, &c2, 1)) {
        if (!byte) {
            fprintf(stderr, "EOF on %s which is empty\n", argv[2]);
            CHK(close(entree1));
            CHK(close(entree2));
            return 1;
        }
        fprintf(stderr, "EOF on %s after byte %ju, line %ju\n", argv[2], byte,
                line);
        CHK(close(entree1));
        CHK(close(entree2));
        return 1;
    }
    return 0;
}
