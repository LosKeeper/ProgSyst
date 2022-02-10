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

volatile sig_atomic_t signal_usr1 = 0;

volatile sig_atomic_t signal_usr2 = 0;

volatile sig_atomic_t signal_child = 0;

volatile sig_atomic_t signal_alarme = 0;

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

void process_fils(sigset_t *masque_usr, sigset_t *vide, int *nb_quantums,
                  pid_t pid_pere) {
    (void)masque_usr;
    while (*nb_quantums != 0) {
        // *Masquer tous les signaux envoyés par le pére
        // !CHK(sigprocmask(SIG_BLOCK, masque_usr, vide));

        while (!signal_usr1) {
            sigsuspend(vide);
        }
        signal_usr1 = 0;

        while (!signal_usr2) {
            sleep(1);
        }
        signal_usr2 = 0;

        if (*nb_quantums == 1) {
            exit(0);
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
        signal_child = 1;
        signal_pere = 3;
        break;

    case SIGALRM:
        signal_alarme = 1;
        signal_pere = 1;
        break;
    }
}

int main(int argc, char **argv) {

    int nb_process_fini = 0;

    // Test nb arguments
    if (argc < 3) {
        raler(0, "arguments");
    }

    int nb_process = (int)argc - 2;
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

    // Masques pere et fils (par héritage)
    sigset_t vide, masque_signaux_fils, masque_fils;
    CHK(sigemptyset(&vide));
    CHK(sigemptyset(&masque_signaux_fils));
    CHK(sigaddset(&masque_signaux_fils, SIGCHLD));
    CHK(sigaddset(&masque_signaux_fils, SIGUSR1));
    CHK(sigaddset(&masque_signaux_fils, SIGALRM));
    CHK(sigemptyset(&masque_signaux_fils));
    CHK(sigaddset(&masque_fils, SIGUSR1));
    CHK(sigaddset(&masque_fils, SIGUSR2));

    // Redirection des signaux
    struct sigaction usr1, usr2, chld, alarme;
    usr1.sa_handler = signal_handler;
    usr2.sa_handler = signal_handler;
    chld.sa_handler = signal_handler;
    alarme.sa_handler = signal_handler;
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
            process_fils(&masque_fils, &vide, &nb_quantums[k], pid_pere);
        }
    }

    // Masquage des signaux envoyés par le fils
    CHK(sigprocmask(SIG_BLOCK, &masque_signaux_fils, &vide));

    // Démarage de la première alarme
    alarm(duree_qtum);

    // Itérateur du numéro du processus
    int k = 0;

    int cpt = 0;
    int raison;
    pid_t pid_traiter;

    // Tant que tous les fils ne sont pas finis
    while (nb_process_fini < nb_process) {

        if (signal_pere == 0) {
            alarm(duree_qtum);
            // Lancement du processus suivant
            CHK(kill(process_id[k], SIGUSR1));
            election(k);
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

        /*
                for (int k = 0; k < nb_process; k++) {
                    // *Faire switch sur les différents signaux ossibles
           soit SIGUSR1
                    // *soit SIGCHLD soit SIGALRM

                    // Masquage des signaux envoyés par le fils
                    CHK(sigprocmask(SIG_BLOCK, &masque_signaux_fils,
           &vide));

                    // Si processus k fini alors on le passe
                    if (process_id[k] == 0) {
                        goto process_suivant;
                    }

                    // Démasquer alarme
                    CHK(sigprocmask(SIG_SETMASK, &vide, NULL));

                    // On s'assure que l'ancienne alarme soir terminée
                    while (alarm(0) != 0) {
                    }

                    // Démarrage de l'alarme sur la duré d'un quantum
                    alarm(duree_qtum);

                    // Lancement du processus k
                    CHK(kill(process_id[k], SIGUSR1));
                    election(k);

                    // Attente SIGALARM
                    while (!signal_alarme) {
                        sigsuspend(&vide); // !Masque et démasque tout seul
                    }
                    signal_alarme = 0;

                    // Masquer alarme
                    CHK(sigprocmask(SIG_BLOCK, &masque_alarme, &vide));

                    // Démasquage des signaux envoyés par le fils
                    CHK(sigprocmask(SIG_SETMASK, &vide, NULL));

                    // Envoie commande de terminaison au processus
                    CHK(kill(process_id[k], SIGUSR2));

                    // Attente SIGCHLD ou SIGUSR1
                    sigsuspend(&vide);

                    // Masquage des signaux envoyés par le fils et de
           l'alarme CHK(sigprocmask(SIG_BLOCK, &masque_signaux_fils,
           &masque_alarme));

                    // Si SIGCHLD -> process k a ne plus considérer
                    if (signal_child == 1) {
                        eviction(k);
                        signal_child = 0;
                        nb_process_fini++;
                        int raison;
                        CHK(wait(&raison));
                        enregistrer_terminaison(process_id, raison,
           nb_process); terminaison_print(k);

                    } else if (signal_usr1 == 1) { // Si SIGUSR1 alors
           process non fini eviction(k); signal_usr1 = 0;
                    }

                process_suivant:;
                }*/
    }
    free(process_id);
    free(nb_quantums);
    return 0;
}