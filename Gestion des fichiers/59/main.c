#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <unistd.h>

typedef struct FICHIER {
    int desc;
    char *bufferR;
    char *bufferW;
    int right;
} FICHIER;

FICHIER my_open(char *path_name, char mode) {
    FICHIER fichier;
    switch (mode) {
    case 'r':
        fichier.desc = open(path_name, O_RDONLY);
        break;
    case 'w':
        fichier.desc = open(path_name, O_WRONLY);
        break;
    }
    return fichier;
}