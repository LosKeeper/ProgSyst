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
            CHK(sigprocmask(SIG_UNBLOCK, masque_fils, NULL));
            sleep(1);
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

    // Masques des signaux envoyés par le fils et masque vide
    sigset_t vide, masque_signaux_fils, masque_fils;
    CHK(sigemptyset(&vide));
    CHK(sigemptyset(&masque_signaux_fils));
    CHK(sigemptyset(&masque_fils));
    CHK(sigaddset(&masque_signaux_fils, SIGCHLD));
    CHK(sigaddset(&masque_signaux_fils, SIGUSR1));
    CHK(sigaddset(&masque_signaux_fils, SIGALRM));
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

    for (int k = 0; k < nb_process; k++) {
        // Liste de tous les pid des process fils
        process_id[k] = fork();

        switch (process_id[k]) {
        case -1:
            raler(1, "fork");

        case 0:
            // Sous-prgm processus fils
            process_fils(&masque_fils, &vide, &nb_quantums[k], pid_pere, k);
            free(process_id);
            free(nb_quantums);
            exit(0);
        }
    }

    // Masquage des signaux envoyés par le fils
    CHK(sigprocmask(SIG_BLOCK, &masque_signaux_fils, &vide));

    // Démarage de la première alarme
    alarm(duree_qtum);

    // Itérateur du numéro du processus
    int k = 0;

    // Variables utilisées dans le switch
    int cpt = 0;
    int raison;
    pid_t pid_traiter;

    // Tant que tous les fils ne sont pas finis
    while (nb_process_fini < nb_process) {

        if (signal_pere == 0) {
            alarm(duree_qtum);
            // Lancement du processus suivant
            CHK(kill(process_id[k], SIGUSR1));
        }

        // Attente d'un signal
        sigsuspend(&vide);
        if (errno != EINTR) {
            raler(1, "sigsuspend");
        }

        switch (signal_pere) {
        case 1:
            // Envoie commande de terminaison au processus
            CHK(kill(process_id[k], SIGUSR2));
            break;

        case 2:
            eviction(k);
            cpt = 0;
            do {
                cpt++;
                k = (k + 1) % nb_process;
            } while (process_id[k] == 0 && cpt < nb_process);
            signal_pere = 0;
            break;

        case 3:
            eviction(k);
            nb_process_fini++;
            pid_traiter = wait(&raison);
            CHK(pid_traiter);
            int index =
                enregistrer_terminaison(process_id, pid_traiter, nb_process);
            if (index == -1) {
                raler(0, "pid does not exist");
            }
            terminaison_print(index);
            cpt = 0;
            do {
                cpt++;
                k = (k + 1) % nb_process;
            } while (process_id[k] == 0 && cpt < nb_process);
            signal_pere = 0;
            break;
        }
    }
    free(process_id);
    free(nb_quantums);
    return 0;
}