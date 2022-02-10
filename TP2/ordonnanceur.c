#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/types.h>
#include <sys/wait.h>
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

// Varibales globales utilisées pour les signaux
volatile sig_atomic_t signal_usr1 = 0;
volatile sig_atomic_t signal_usr2 = 0;
volatile sig_atomic_t signal_pere = 0;

// Fonctions d'affichage sur la sortie standard
void election(int process_id) {
    printf("SURP - process %d\n", process_id);
    fflush(stdout);
}
void terminaison_print(int process_id) {
    printf("TERM - process %d\n", process_id);
    fflush(stdout);
}
void eviction(int process_id) {
    printf("EVIP - process %d\n", process_id);
    fflush(stdout);
}

/**
 * @brief Fonction modifiant l'index du prochain processus à élire
 * @param tableau_pid tableau de tous les pid des fils
 * @param index index du tableau qui prendra l'index du procahin processus à
 * élire
 * @param nb_process nombre total de processus fils
 * */
void prochain_processus(pid_t *tableau_pid, int *index, int nb_process) {

    // Compteur permetant de ne pas boucler indéfiniment
    int cpt = 0;

    do {
        cpt++;
        *index = (*index + 1) % nb_process;
    } while (tableau_pid[*index] == 0 && cpt < nb_process);
}

/**
 * @brief Fonction permettant d'enregistrer la terminaison d'un sous-processus
 * @param tableau_pid tableau contenant tous les pid des processus fils
 * @param pid_traiter pid du processus fils fini
 * @param nb_process nombre total de processus fils
 * @return Renvoie l'index du processus fini et remlpace son pid par 0 dans le
 * tableau des pid ou -1 en cas d'échec
 * */
int enregistrer_terminaison(pid_t *tableau_pid, pid_t pid_traiter,
                            int nb_process) {
    for (int k = 0; k < nb_process; k++) {
        if (tableau_pid[k] == pid_traiter) {
            tableau_pid[k] = 0;
            return k;
        }
    }
    return -1;
}

/**
 * @brief Fonction executée par les processus fils
 * @param masque_fils masque des différents signaux pouvants être reçu par le
 * precessus fils
 * @param vide masque vide utilisé pour l'attente d'un signal
 * @param nb_quantums nombre de quantums restants devant etre réalisés par le
 * processus
 * @param pid_pere pid du père du processus fils
 * @param k numéro du processus fils
 * */
void process_fils(sigset_t *masque_fils, sigset_t *vide, int *nb_quantums,
                  pid_t pid_pere, int k) {
    while (*nb_quantums != 0) {

        // Attente du signal SIGSUR1 envoyée par le père
        while (!signal_usr1) {
            sigsuspend(vide);
            if (errno != EINTR) {
                raler(1, "sigsuspend");
            }
        }
        election(k);
        signal_usr1 = 0;

        // Le programme tourne tant que le père n'as pas envoyé le signal
        // SIGUSR2
        while (!signal_usr2) {
            // On masque les signaux
            CHK(sigprocmask(SIG_UNBLOCK, masque_fils, NULL));

            sleep(1);

            // On démasque les signaux
            CHK(sigprocmask(SIG_BLOCK, masque_fils, NULL));
        }
        signal_usr2 = 0;

        // Le programme renvoie son état et sa terminaison s'il a terminé
        if (*nb_quantums == 1) {
            return;
        } else {
            (*nb_quantums)--;
            CHK(kill(pid_pere, SIGUSR1));
        }
    }
}

/**
 * @brief Fonction executée lors de la reception d'un signal qui va modifier des
 * varaibles globales
 * @param signum numéro du signal reçu
 * */
void signal_handler(int signum) {
    switch (signum) {
    case SIGUSR1:
        signal_usr1 = 1;
        signal_pere = 2;
        break;

    case SIGUSR2:
        signal_usr2 = 1;
        break;

    case SIGCHLD:
        signal_pere = 3;
        break;

    case SIGALRM:
        signal_pere = 1;
        break;
    }
}

int main(int argc, char **argv) {

    // Test nb arguments
    if (argc < 3) {
        raler(0, "arguments");
    }

    int nb_process_fini = 0;
    int nb_process = argc - 2;
    int duree_qtum = atoi(argv[1]);
    pid_t pid_pere = getpid();

    // Tests durée quantums
    if (duree_qtum < 1) {
        raler(0, "quantum");
    }

    // Tableau des pid de tous les processus fils
    pid_t *process_id = malloc(sizeof(pid_t) * nb_process);

    // Tableau du nombre de quantums par fils
    int *nb_quantums = malloc(sizeof(int) * nb_process);
    for (int i = 0; i < nb_process; i++) {
        nb_quantums[i] = atoi(argv[i + 2]);
    }

    // Test durée des processus
    for (int k = 0; k < nb_process; k++) {
        if (nb_quantums[k] < 1) {
            raler(0, "process");
        }
    }

    // Masques des signaux envoyés par le fils, par le père et masque vide
    sigset_t vide, masque_pere, masque_fils;
    CHK(sigemptyset(&vide));
    CHK(sigemptyset(&masque_pere));
    CHK(sigemptyset(&masque_fils));
    CHK(sigaddset(&masque_pere, SIGCHLD));
    CHK(sigaddset(&masque_pere, SIGUSR1));
    CHK(sigaddset(&masque_pere, SIGALRM));
    CHK(sigaddset(&masque_fils, SIGUSR1));
    CHK(sigaddset(&masque_fils, SIGUSR2));

    // Redirection des signaux
    struct sigaction usr1, usr2, chld, alarme;
    usr1.sa_handler = signal_handler;
    usr1.sa_flags = 0;
    CHK(sigemptyset(&usr1.sa_mask));
    usr2.sa_handler = signal_handler;
    usr2.sa_flags = 0;
    CHK(sigemptyset(&usr2.sa_mask));
    chld.sa_handler = signal_handler;
    chld.sa_flags = 0;
    CHK(sigemptyset(&chld.sa_mask));
    alarme.sa_handler = signal_handler;
    alarme.sa_flags = 0;
    CHK(sigemptyset(&alarme.sa_mask));
    CHK(sigaction(SIGUSR1, &usr1, NULL));
    CHK(sigaction(SIGUSR2, &usr2, NULL));
    CHK(sigaction(SIGCHLD, &chld, NULL));
    CHK(sigaction(SIGALRM, &alarme, NULL));

    // Génération de tous les processus fils
    for (int k = 0; k < nb_process; k++) {
        // Tableau de tous les pid des process fils
        process_id[k] = fork();

        switch (process_id[k]) {
        case -1:
            raler(1, "fork");

        case 0:
            // Sous-prgm processus fils
            process_fils(&masque_fils, &vide, &nb_quantums[k], pid_pere, k);

            // Libération de la mémoire occupée par les tableaux
            free(process_id);
            free(nb_quantums);

            exit(0);
        }
    }

    // Masquage des signaux envoyés par le fils
    CHK(sigprocmask(SIG_BLOCK, &masque_pere, &vide));

    // Démarage de la première alarme
    alarm(duree_qtum);

    // Itérateur du numéro du processus
    int k = 0;

    // Variables utilisées dans le switch
    int raison;
    pid_t pid_traiter;

    // Tant que tous les fils ne sont pas finis
    while (nb_process_fini < nb_process) {

        // Démarrage de l'alarme et élection du prochain procesus
        if (signal_pere == 0) {
            alarm(duree_qtum);
            CHK(kill(process_id[k], SIGUSR1));
        }

        // Attente d'un signal
        sigsuspend(&vide);
        if (errno != EINTR) {
            raler(1, "sigsuspend");
        }

        switch (signal_pere) {
        case 1: // Reception d'un SIGALRM

            // Envoie commande de terminaison au processus
            CHK(kill(process_id[k], SIGUSR2));
            break;

        case 2: // Reception d'un SIGUSR1

            eviction(k);

            // Recherche du prochain processus à élire
            prochain_processus(process_id, &k, nb_process);
            signal_pere = 0;
            break;

        case 3: // Reception d'un SIGCHLD

            eviction(k);

            // Récupération du pid du processus fini et enregistrement de sa
            // terminaison
            pid_traiter = wait(&raison);
            CHK(pid_traiter);
            nb_process_fini++;
            int index =
                enregistrer_terminaison(process_id, pid_traiter, nb_process);
            if (index == -1) {
                raler(0, "Le pid n'existe pas");
            }
            terminaison_print(index);

            // Recherche du prochain processus à élire
            prochain_processus(process_id, &k, nb_process);
            signal_pere = 0;
            break;
        }
    }

    // Libération de la mémoire occupée par les tableaux
    free(process_id);
    free(nb_quantums);

    return 0;
}