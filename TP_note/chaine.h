#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <stdarg.h>

#define CHK(op)  do { if ((op) == -1)   raler (1, #op);} while (0)
#define CHKP(op) do { if ((op) == NULL) raler (1, #op);} while (0)

/** @brief convertit une chaine "0xxx" en entier
 */
#define CONVERT(str) strtol (str, NULL, 8)

/** @brief affiche un entier en chaine "0xxx"
 */
#define PRINTOCT(mode) printf ("0%03lo\n", (uintmax_t) mode);

/** Fonctions pour manipuler le buffer
 */
noreturn void raler (int, const char *, ...);
ssize_t taille_ligne (int, char *, ssize_t);
ssize_t translate (char *, ssize_t);
char **decompose (char *, ssize_t);
void free_tab (char **);
