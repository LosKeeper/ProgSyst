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
#define PATH 256
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
struct trame_t {
    int destination;
    int source;
    char payload[PAYLOAD_SIZE];
};

/**
 * @brief Fonction permettant de récupérer l'adresse de destination et le
 *message a transmettre à partir d'une trame
 * @param buffer correspondant à la trame brute
 * @return Une structure trame_t contenant le trame décomposée
 **/
struct trame_t read_trame(char *buffer) {

    struct trame_t trame;

    // Récupération de l'adresse de destination
    // Lecture du permier octet du buffer et copie dans la structure
    trame.destination = (int)buffer[0];

    // Récupération du payload
    // Lecture des 4 derniers octets du buffer et copie dans la structure
    for (int z = PAYLOAD_SIZE; z < TRAME_SIZE; z++) {
        trame.payload[z - PAYLOAD_SIZE] = buffer[z];
    }
    trame.payload[PAYLOAD_SIZE] = '\0';

    return trame;
}

/**
 * Il peut y avoir un blocage reseau lorsque chaque machine écrit sur le tube
 * commun il peut donc y avoir des conflits si plusieures machines écrivent en
 * meme temps sur le tube commun.
 **/
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

    // Tableau de tubes (pas de malloc donc il y aura surement des tubes non
    // utilisés)
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
            char str[PATH];
            int m = snprintf(str, PATH, "STA_%d", k);
            if (m < 0 || m >= PATH) {
                raler(0, "snprinf");
            }
            CHK(sta = open(str, O_RDONLY));

            // Tant que récpération d'un seul message complet (destination et
            // payload) qui correspond à une seule ligne
            int n;
            char buffer[TRAME_SIZE];
            struct trame_t trame;
            while ((n = read(sta, buffer, TRAME_SIZE)) > 0) {

                // Décomposition de la trame dans le structure et ajout de
                // numéro de l'émetteur
                trame = read_trame(buffer);
                trame.source = k;
                CHK(write(tab_pipe[0][1], &trame, sizeof(trame)));
            }
            CHK(close(sta));
            CHK(n);

            // Fermeture du tube commun
            CHK(close(tab_pipe[0][1]));

            // Lecture de la structure reçue
            while ((n = read(tab_pipe[k][0], &trame, sizeof(trame))) > 0) {
                printf("%d - %d - %d - %s\n", k, trame.source,
                       trame.destination, trame.payload);
            }
            CHK(n);

            // Fermeture du tube de lecture propre aàla station
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
    struct trame_t trame;
    int n;
    while ((n = read(tab_pipe[0][0], &trame, sizeof(trame))) > 0) {

        // Vérification de l'existance de la station de destination
        if (trame.destination <= nb_sta && trame.destination > 0) {

            // Ecriture sur le BON commutateur
            CHK(write(tab_pipe[trame.destination][1], &trame, sizeof(trame)));

        } else {

            // Diffusion sur toutes les stations
            for (int i = 1; i < nb_sta + 1 && i != trame.source; i++) {

                // Ecriture sur le commutateur
                CHK(write(tab_pipe[i][1], &trame, sizeof(trame)));
            }
        }
    }
    CHK(n);

    // Fermeture des tubes restants
    CHK(close(tab_pipe[0][0]));
    for (int i = 1; i < nb_sta + 1; i++) {
        CHK(close(tab_pipe[i][1]));
    }

    // Lecture code de retour des processus fils
    int raison[MAXSTA];
    for (int k = 0; k < nb_sta; k++) {
        CHK(wait(&raison[k]));
        if (!WIFEXITED(raison[k])) {
            raler(1, "erreur fermeture processus fils");
        }
    }

    exit(EXIT_SUCCESS);
}