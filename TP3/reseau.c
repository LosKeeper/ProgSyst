#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXSTA 10
#define PAYLOAD_SIZE 4
#define TRAME_SIZE 2 * PAYLOAD_SIZE

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
// Structure utilisée pour décomposer une trame
typedef struct trame_t {
    int destination;
    char payload[PAYLOAD_SIZE];
} trame_t;

/**
 * @brief Fonction permettant de récupérer l'adresse de destination et le
 *message a transmettre à partir d'une trame
 * @param buffer correspondant à la trame brute
 * @return Une structure trame_t contenant le trame décomposée
 **/
trame_t read_trame(char *buffer) {

    trame_t trame;

    // Récupération de l'adresse de destination
    // Lecture du permier octet
    trame.destination = (int)buffer[0];

    // Récupération du payload
    // Lecture des 4 derniers octets
    for (int z = PAYLOAD_SIZE; z < TRAME_SIZE; z++) {
        trame.payload[z - PAYLOAD_SIZE] = buffer[z];
    }
    trame.payload[PAYLOAD_SIZE] = '\0';

    return trame;
}

int main(int argc, char **argv) {

    // Test du nombre d'arguments
    if (argc != 2) {
        raler(0, "usage: %s <nb_sta>", argv[0]);
    }

    // Test nombre de stations
    int nb_sta = atoi(argv[1]);
    if (nb_sta < 1 || nb_sta > MAXSTA) {
        raler(0, "probleme nombre de stations");
    }

    // Tableau de tubes
    int tab_pipe[MAXSTA + 1][2];

    // Génération du tube commun
    CHK(pipe(tab_pipe[0]));

    // Génération des tubes père fils
    for (int k = 1; k < nb_sta + 1; k++) {
        CHK(pipe(tab_pipe[k]));
    }

    // Génération des processus fils
    for (int k = 1; k < nb_sta + 1; k++) {

        // Génération des processus fils
        switch (fork()) {
        case -1:
            raler(1, "fork");

        case 0:
            // Fermeture des tubes non nécessaires pour le fils
            CHK(close(tab_pipe[0][0]));
            CHK(close(tab_pipe[k][1]));
            for (int i = 1; i < k; i++) {
                CHK(close(tab_pipe[i][0]));
                CHK(close(tab_pipe[i][1]));
            }
            for (int j = k + 1; j < nb_sta + 1; j++) {
                CHK(close(tab_pipe[j][0]));
                CHK(close(tab_pipe[j][1]));
            }

            // Ouverture de son fichier STA_x
            int sta;
            char str[6];
            sprintf(str, "STA_%d", k);
            CHK(sta = open(str, O_RDONLY));

            // Tant que récpération d'un seul message complet (destination et
            // payload) qui correspond à une seule ligne
            char buffer[TRAME_SIZE];
            while (read(sta, buffer, TRAME_SIZE) > 0) {

                // Ecriture sur le commutateur
                CHK(write(tab_pipe[0][1], buffer, TRAME_SIZE));
                CHK(write(tab_pipe[0][1], &str[4], 1));
            }
            CHK(close(tab_pipe[0][1]));

            //! Attente de message
            // sleep(1);

            // Lecture de la trame reçue
            while (read(tab_pipe[k][0], buffer, TRAME_SIZE)) {
                trame_t trame;
                trame = read_trame(buffer);
                char source;
                CHK(read(tab_pipe[k][0], &source, 1));
                printf("%d - %c - %d - %s\n", k, source, trame.destination,
                       trame.payload);
                fflush(stdout);
            }
            CHK(close(tab_pipe[k][0]));

            exit(EXIT_SUCCESS);
        }
    }

    // Fermeture des tubes non nécessaires pour le père
    CHK(close(tab_pipe[0][1]));
    for (int j = 1; j < nb_sta + 1; j++) {
        CHK(close(tab_pipe[j][0]));
    }

    // Lecture du commutateur
    char buffer[TRAME_SIZE];
    while (read(tab_pipe[0][0], buffer, TRAME_SIZE) > 0) {

        // Decomposition trame
        trame_t trame;
        trame = read_trame(buffer);

        // Récupération de l'adresse de l'emetteur
        char source;
        CHK(read(tab_pipe[0][0], &source, 1));

        // Vérification de l'existance de la station de destination
        if (trame.destination < nb_sta + 1) {

            // Ecriture sur le BON commutateur
            CHK(write(tab_pipe[trame.destination][1], buffer, TRAME_SIZE));

            // Ecriture de l'emetteur du message dans le pipe
            CHK(write(tab_pipe[trame.destination][1], &source, 1));

        } else {

            // Diffusion sur toutes les stations
            for (int i = 1; i < (int)source; i++) {

                // Ecriture sur le commutateur
                CHK(write(tab_pipe[i][1], buffer, TRAME_SIZE));

                // Ecriture de l'emetteur du message dans le pipe
                CHK(write(tab_pipe[i][1], &source, 1));
            }

            for (int i = (int)source; i < nb_sta + 1; i++) {

                // Ecriture sur le commutateur
                CHK(write(tab_pipe[i][1], buffer, TRAME_SIZE));

                // Ecriture de l'emetteur du message dans le pipe
                CHK(write(tab_pipe[i][1], &source, 1));
            }
        }
    }

    // Fermeture des tubes restants
    CHK(close(tab_pipe[0][0]));
    for (int i = 1; i < nb_sta + 1; i++) {
        CHK(close(tab_pipe[i][1]));
    }

    // Lecture code de retour des processus fils
    int *raison = malloc(sizeof(int) * nb_sta);
    for (int k = 0; k < nb_sta; k++) {
        CHK(wait(&raison[k]));
        if (!WIFEXITED(raison[k])) {
            raler(1, "erreur fermeture processus fils");
        }
    }

    free(raison);
    return 0;
}