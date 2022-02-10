#include "chaine.h"

#define SIZE 1024

#define ERROR(op)                                                              \
    do {                                                                       \
        if ((op) == -1) {                                                      \
            perror(#op);                                                       \
            int_env_var = 1;                                                   \
        } else                                                                 \
            int_env_var = 0;                                                   \
    } while (0)

static int int_env_var = 0;

int internal_cmd(char **tab_cmd) {
    int r_val = 0;

    if (0) {
    } else if (strcmp(tab_cmd[0], "cd") == 0) { // CD
        char *path = tab_cmd[1];
        if (path == NULL) {
            path = getenv("HOME");
            if (path == NULL) {
                fprintf(stderr, "cd: variable HOME non définie\n");
                int_env_var = 1;
            }
        }

        if (path != NULL) {
            ERROR(chdir(path));
        }

        r_val = 1;
    } else if (strcmp(tab_cmd[0], "umask") == 0) { // UMASK
        // umask ne peut pas échouer
        if (tab_cmd[1] == NULL) {
            mode_t current = umask(0777);
            umask(current);
            PRINTOCT(current);
        } else {
            umask(CONVERT(tab_cmd[1]));
        }
        int_env_var = 0;
        r_val = 1;
    } else if (strcmp(tab_cmd[0], "print") == 0) { // PRINT
        printf("%d\n", int_env_var);
        int_env_var = 0;
        r_val = 1;
    } else if (strcmp(tab_cmd[0], "exit") == 0) { // EXIT
        free_tab(tab_cmd);
        exit(EXIT_SUCCESS);
    }

    return r_val;
}

void attendre_fils(void) {
    int raison;

    CHK(wait(&raison));
    if (!(WIFEXITED(raison) && WEXITSTATUS(raison) == 0)) {
        int_env_var = 1;
    } else {
        int_env_var = 0;
    }

    return;
}

int main(void) {
    int end = 0;

    while (end == 0) {
        ssize_t n;
        char buf[SIZE + 1];

        CHK(n = read(0, buf, SIZE));
        if (n == 0) { // si redirection stdin
            end = 1;
            continue;
        } else if (buf[0] == '\n' || buf[0] == ' ')
            continue;

        char **tab_cmd = decompose(buf, n);

        if (internal_cmd(tab_cmd) == 1) { // commande interne
        } else {                          // commande externe
            switch (fork()) {
            case -1:
                perror("fork");
                int_env_var = 1;
                break;

            case 0:
                execvp(tab_cmd[0], tab_cmd);
                perror("execvp");
                free_tab(tab_cmd);
                exit(EXIT_FAILURE);

            default:
                attendre_fils(); // attendre terminaison
                                 // avant de lire la cmd suivante
            }
        }

        free_tab(tab_cmd); // pas de fuite de mémoire
    }

    return 0;
}