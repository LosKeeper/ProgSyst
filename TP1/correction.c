#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SIZE 1024

#define MIN(a, b) (((a) > (b)) ? (b) : (a))
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

// gestion des lectures partielles pour remplir au max le buffer
ssize_t remplir_buf(int desc, char *buf, ssize_t taille) {
    ssize_t n = 0, tmp = 1;
    while (n != taille && tmp > 0) {
        CHK(tmp = read(desc, buf + n, SIZE - n));
        n += tmp;
    }

    return n;
}

// retourne 2 si les deux fichiers sont vides
//          1 si slt l'un des deux est vide
//          0 sinon
int test_vide(int desc1, const char *f1, int desc2, const char *f2) {
    struct stat stbuf1, stbuf2;
    CHK(fstat(desc1, &stbuf1));
    CHK(fstat(desc2, &stbuf2));

    int r_val = 0;
    int size;

    // les deux fichiers sont vides donc identiques
    if (stbuf1.st_size == 0 && stbuf2.st_size == 0)
        r_val = 2;

    // si un et slt un fichier est vide
    else if ((size = MIN(stbuf1.st_size, stbuf2.st_size)) == 0) {
        fprintf(stderr, "EOF on %s which is empty\n",
                size == stbuf1.st_size ? f1 : f2);
        r_val = 1;
    }

    return r_val;
}

ssize_t cmp(char *buf1, char *buf2, ssize_t n, size_t *nblignes) {
    ssize_t i = 0;

    while (i < n && buf1[i] == buf2[i]) {
        if (buf1[i] == '\n')
            (*nblignes)++;
        i++;
    }

    return i != n ? i : -1;
}

int main(int argc, char *argv[]) {
    // toujours tester le nb d'arguments
    if (argc != 3)
        raler(0, "usage: %s <fichier1> <fichier2>", argv[0]);

    // ouverture des fichiers en lecture seule
    int desc1, desc2;
    CHK(desc1 = open(argv[1], O_RDONLY));
    CHK(desc2 = open(argv[2], O_RDONLY));

    // gestion fichier vide
    int fin; // condition arrêt boucle principale
    fin = test_vide(desc1, argv[1], desc2, argv[2]);

    int res = fin % 2;   // code de retour du programme
    ssize_t pos = 0;     // octets déjà comparés
    size_t nblignes = 1; // nb de lignes déjà traitées

    while (fin == 0) {
        char buf1[SIZE], buf2[SIZE];
        ssize_t ind;

        // remplir les buffers au max, même en cas de lectures partielles
        ssize_t n1 = remplir_buf(desc1, buf1, SIZE);
        ssize_t n2 = remplir_buf(desc2, buf2, SIZE);

        // fichiers identiques, on a terminé
        if (n1 == 0 && n2 == 0)
            fin = 1;

        // comparaison des octets lus
        else if ((ind = cmp(buf1, buf2, MIN(n1, n2), &nblignes)) != -1) {
            fprintf(stderr, "%s %s differ: byte %jd, line %ju\n", argv[1],
                    argv[2], (intmax_t)pos + ind + 1, (uintmax_t)nblignes);
            res = fin = 1;
        }
        // traitement des tailles différentes
        else if (n1 != n2) {
            char *fichier = n1 < n2 ? argv[1] : argv[2];
            fprintf(stderr, "EOF on %s after byte %jd, line %ju\n", fichier,
                    (intmax_t)pos + MIN(n1, n2), (uintmax_t)nblignes);
            res = fin = 1;
        } else // n1 == n2 donc on continue
            pos += n1;
    }

    CHK(close(desc1));
    CHK(close(desc2));

    return res;
}