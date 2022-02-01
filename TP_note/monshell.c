#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "chaine.h"

#define STR_LEN_DEF 1000

int exit_total = 0;

void cd_perso(char *chemin) {
    char *current_rep;
    /*if (chemin == NULL) {
        execlp("cd", getenv("HOME"));
    } else {
        execlp("cd", chemin);
    }*/
}

void umask_perso(int masque) {
    if (masque == 0) {
        mode_t cur_masque;
        cur_masque = umask(cur_masque);
        umask(cur_masque);
        PRINTOCT(cur_masque);
    } else {
        umask((mode_t)masque);
    }
}

void print_perso() { execl("/usr/bin/echo", "echo", "$?", NULL); }

void exit_perso(void) { exit_total = 1; }

int main(void) {
    // tant que la fermeture du programme n'est pas demandée
    while (exit_total != 1) {

        // lecture de la commande sur l'entree standard
        char buff[STR_LEN_DEF];
        int n;
        CHK(n = read(0, buff, STR_LEN_DEF));

        if (buff[0] == '\n' || buff[0] == '\0') {
            exit(0);
        }

        char **mots = decompose(buff, n);

        int raison;
        switch (fork()) {
        case -1:
            raler(1, "fork");

        case 0:
            // si commande cd
            if (!strcmp("cd", mots[0])) {
                if (mots[1]) {
                    cd_perso(mots[1]);
                } else {
                    cd_perso(NULL);
                }
                exit(0);

                // si commande umask
            } else if (!strcmp("umask", mots[0])) {
                if (mots[1]) {
                    umask_perso(CONVERT(mots[1]));
                } else {
                    umask_perso(0);
                }
                exit(0);

                // si commande print
            } else if (!strcmp("print", mots[0])) {
                print_perso();
                exit(0);

                // si commande exit
            } else if (!strcmp("exit", mots[0])) {
                exit_perso();
                exit(0);

                // sinon commande externe
            } else {
                execlp(mots[0], mots[0], mots[1], NULL);
                exit(0);
            }

        default:
            CHK(wait(&raison));
        }
        free_tab(mots);
    }
    return 0;
}
