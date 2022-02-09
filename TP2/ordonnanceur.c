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

void election(intmax_t process_id) {
    printf("SURP - process %ju\n", process_id);
    fflush(stdout);
}

void terminaison_print(intmax_t process_id) {
    printf("TERM - process %ju\n", process_id);
    fflush(stdout);
}

void eviction(intmax_t process_id) {
    printf("EVIP - process %ju\n", process_id);
    fflush(stdout);
}

void process_fils(sigset_t *masque_usr, sigset_t *vide, int *nb_quantums,
                  pid_t pid_pere) {
    (void)masque_usr;
    while (*nb_quantums != 0) {
        // Démasquer tous les signaux SIGUSR1 ET 2
        // !CHK(sigprocmask(SIG_SETMASK, vide, NULL));

        while (!signal_usr1) {
            sigsuspend(vide);
        }
        signal_usr1 = 0;

        while (!signal_usr2) {
            sleep(1);
        }
        signal_usr2 = 0;

        // Masquer tous les signaux envoyés par le pére
        // !CHK(sigprocmask(SIG_BLOCK, masque_usr, vide));

        (*nb_quantums)--;
        if (*nb_quantums == 0) {
            CHK(kill(pid_pere, SIGCHLD));
            exit(0);
        } else {
            CHK(kill(pid_pere, SIGUSR1));
        }
    }
}

void signal_handler(int signum) {
    switch (signum) {
    case SIGUSR1:
        signal_usr1 = 1;
        break;

    case SIGUSR2:
        signal_usr2 = 1;
        break;

    case SIGCHLD:
        signal_child = 1;
        break;

    case SIGALRM:
        signal_alarme = 1;
        break;
    }
}

int main(int argc, char **argv) {

    int nb_process_fini = 0;

    // Test nb arguments
    if (argc < 3) {
        raler(0, "arguments");
    }

    intmax_t nb_process = (intmax_t)argc - 2;
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
    for (intmax_t k = 0; k < nb_process; k++) {
        if (nb_quantums[k] < 1) {
            raler(0, "process");
        }
    }

    // Masques pere et fils (par héritage)
    sigset_t vide, masque_signaux_fils, masque_alarme, masque_fils;
    CHK(sigemptyset(&vide));
    CHK(sigemptyset(&masque_signaux_fils));
    CHK(sigaddset(&masque_signaux_fils, SIGCHLD));
    CHK(sigaddset(&masque_signaux_fils, SIGUSR1));
    CHK(sigemptyset(&masque_alarme));
    CHK(sigaddset(&masque_alarme, SIGALRM));
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

    for (intmax_t k = 0; k < nb_process; k++) {
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

    // Tant que tous les fils ne sont pas finis
    while (nb_process_fini < nb_process) {
        for (intmax_t k = 0; k < nb_process; k++) {
            // Masquage des signaux envoyés par le fils
            CHK(sigprocmask(SIG_BLOCK, &masque_signaux_fils, &vide));

            // Si processus k fini alors on le passe
            if (process_id[k] == -1) {
                goto process_suivant;
            }

            // Démasquer alarme
            CHK(sigprocmask(SIG_SETMASK, &vide, NULL));

            // Démarrage de l'alarme sur la duré d'un quantum
            alarm(duree_qtum);

            // Lancement du processus k
            CHK(kill(process_id[k], SIGUSR1));
            election(k);

            // Attente SIGALARM
            while (!signal_alarme) {
                sigsuspend(&vide);
            }
            signal_alarme = 0;

            // Masquer alarme
            CHK(sigprocmask(SIG_BLOCK, &masque_alarme, &vide));

            // Démasquage des signaux envoyés par le fils
            CHK(sigprocmask(SIG_SETMASK, &vide, NULL));

            // Envoie commande de terminaison au processus
            CHK(kill(process_id[k], SIGUSR2));
            eviction(k);

            // Attente SIGCHLD ou SIGUSR1
            sigsuspend(&vide);

            // Masquage des signaux envoyés par le fils
            CHK(sigprocmask(SIG_BLOCK, &masque_signaux_fils, NULL));

            // Si SIGCHLD -> process k a ne plus considérer
            if (signal_child == 1) {
                signal_child = 0;
                nb_process_fini++;
                process_id[k] = -1;
                int raison;
                CHK(wait(&raison));
                terminaison_print(k);

            } else if (signal_usr1 == 1) { // Si SIGUSR1 alors process non fini
                signal_usr1 = 0;
            }

        process_suivant:;
        }
    }
    free(process_id);
    free(nb_quantums);
    return 0;
}