#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CHK(op)  do { if ((op) == -1)   raler (1, #op); } while (0)
#define CHKP(op) do { if ((op) == NULL) raler (1, #op); } while (0)

#define PRINT(str, val) do { printf (str, val); fflush (stdout); } while (0)

noreturn void raler (int syserr, const char *msg, ...)
{
    va_list ap;

    va_start (ap, msg);
    vfprintf (stderr, msg, ap);
    fprintf (stderr, "\n");
    va_end (ap);

    if (syserr == 1)
        perror ("");

    exit (EXIT_FAILURE);
}

#define DEBUT_QT 0
#define FIN_QT   1

/**
 * Gestion des quanta : fonctions appelées par la réception de
 * SIGALRM dans le processus père (ordonnanceur) et de SIGUSR1 dans
 * les processus fils.
 */
volatile sig_atomic_t event_q;
void quantum_start (int signo)
{
    (void) signo;
    event_q = DEBUT_QT;
}

void quantum_end (int signo)
{
    (void) signo;
    event_q = FIN_QT;
}

/**
 * Acquittement qu'un processus a été évincé du processeur. Fonction
 * appelée par la réception de SIGUSR1 dans le processus père.
 */
volatile sig_atomic_t ack;
void acquittement (int signo)
{
    (void) signo;
    ack = 1;
}

/**
 * Notification de la fin d'un processus fils. Fonction appelée par la
 * réception de SIGCHLD dans le processus père. On peut recevoir
 * plusieurs SIGCHLD avant traitement.
 */
volatile sig_atomic_t fin_fils;
void process_term (int signo)
{
    (void) signo;
    fin_fils++;
}

/**
 * Modifie l'action associée à la réception d'un signal
 * positionne l'ancienne action dans le pointeur s2
 *
 * @param signal signal pour lequel on va modifier l'action associée
 * @param fct nouvelle fct handler pour le signal
 * @param s2 pointeur pour récupérer ancienne action du signal
 * @return void
 */
void
preparer_signal (int signal, void (*fct) (int signo), struct sigaction *s2)
{
    struct sigaction s1;
    s1.sa_flags = 0;

    s1.sa_handler = fct;
    CHK (sigemptyset (&s1.sa_mask));
    CHK (sigaction (signal, &s1, s2));

    return;
}

/**
 * Code exécuté par les processus fils : attente passive du CPU puis
 * travail (sleep (1)) jusqu'à éviction
 *
 * @param id ID du processus pour affichage
 * @param nb_quanta nbre de quanta que doit réaliser le processus
 * @return void
 */
noreturn void task (int id, int nb_quanta)
{
    pid_t parent = getppid ();

    sigset_t mask, vide;
    CHK (sigemptyset (&vide));
    CHK (sigemptyset (&mask));
    CHK (sigaddset (&mask, SIGUSR1));

    for (int i = 0 ; i < nb_quanta ; i++) {

        // attente passive du CPU
        CHK (sigprocmask (SIG_BLOCK, &mask, NULL));      // SIGUSR1 masqué

        while (event_q == FIN_QT) {
            sigsuspend (&vide);            // SIGUSR1 démasqué pdt attente
            if (errno != EINTR)
                raler (1, "sigsuspend");
        }

        CHK (sigprocmask (SIG_UNBLOCK, &mask, NULL)); // SIGUSR 1 démasqué

        // processus sur le CPU - pas de section critique ici
        PRINT ("SURP - process %d\n", id);
        while (event_q == DEBUT_QT)
            sleep (1);

        // on a reçu SIGUSR2 => éviction du CPU, ACK du retrait
        CHK (kill (parent, SIGUSR1));
    }

    exit (EXIT_SUCCESS);
}

/**
 * Création des procesus fils
 *
 * @param nbproc nombre de processus à créer
 * @param quanta tableau des quanta pour chaque processus
 * @return tableau des pid des processus créés
 */
pid_t *process_factory (size_t nbproc, int *quanta)
{
    pid_t *tab;
    CHKP (tab = calloc (nbproc, sizeof *tab));

    event_q = FIN_QT; // processus fils doivent attendre le go

    int q;
    for (size_t i = 0 ; i < nbproc ; i++) {
        switch (tab [i] = fork ()) {
        case -1:
            raler (1, "fork");

        case 0:
            q = quanta [i];
            free (tab);
            free (quanta);
            task (i, q);
        }
    }

    event_q = DEBUT_QT; // pour le processus père
    return tab;
}

/**
 * Sélection du prochain processus à placer sur le CPU. Retourne l'ID
 * (0, 1, ...)  du processus et pas son PID
 * Fonction appelée ssi des processus sont encore actifs
 *
 * @param tab tableau des pid des processus fils
 * @param nbproc nbre de processus fils
 * @param curproc indice du processus courant
 * @return indice du prochain processus à mettre sur CPU
 */
int select_process (pid_t *tab, int nbproc, int curproc)
{
    int procid = (curproc + 1) % nbproc;

    while (tab [procid] == -1) // algo tourniquet, on prend le suivant
        procid = (procid + 1) % nbproc;

    return procid;
}

/**
 * Gestion de la fin d'un processus fils par le processus
 * père. Positionne le pointeur ptermid avec l'ID du processus pour
 * lequel on a enregistré la terminaison.  La valeur de retour de la
 * fonction indique si le fils a terminé sans erreur.
 *
 * @param tab tableau des pid des processus fils
 * @param ptermid va stocker l'indice du processus qui a terminé
 * @return terminaison du fils => 0 OK, 1 => KO
 */
int term_process (pid_t *tab, int *ptermid)
{
    int raison, r_val = 0;
    pid_t pterm;

    CHK (pterm = wait (&raison));
    if (!WIFEXITED (raison) || WEXITSTATUS (raison) != 0) {
        fprintf (stderr, "erreur terminaison fils");
        r_val = 1;
    }

    // on cherche l'indice du process qui a terminé avec son pid
    *ptermid = 0;
    while (tab [*ptermid] != pterm)
        (*ptermid)++;
    tab [*ptermid] = -1;

    return r_val;
}

// si plusieurs signaux sont en attente car masqués, POSIX indique que
// l'ordre de délivrance lors du démasquage est non défini. Il faut
// donc utiliser des états dans le cas où SIGCHLD et SIGUSR1 pour un
// même processus ne sont pas délivrés dans le bon ordre
#define ETAT_RIEN         0 // processus pas encore traité
#define ETAT_SURP         1 // processus sur CPU
#define ETAT_ATTENTE_ACK  2 // demande d'évicton, attente ACK

int main (int argc, char *argv [])
{
    if (argc < 3)
        raler (0, "usage: %s dur_qtum nb_qtum_p1 [nb_qtum_p2] ...", argv [0]);

    int r_val   = 0;          // valeur de retour du pg
    int nbproc  = argc - 2;   // nb de processus fils
    int actifs  = argc - 2;   // nb de processus encore actifs

    int duree_qtum;
    if (sscanf (argv [1], "%d", &duree_qtum) != 1 || duree_qtum < 1)
        raler (0, "durée d'un quantum > 0 sec.");

    int *quanta;
    CHKP (quanta = calloc (nbproc, sizeof *quanta));
    for (int i = 2 ; i < argc ; i++) {
        if (sscanf (argv [i], "%d", &quanta [i-2]) != 1 || quanta [i-2] < 1)
            raler (0, "durée d'un processus > 0 quantum");
    }

    // init. des actions pour les signaux des proc. fils
    struct sigaction sigusr2;
    preparer_signal (SIGUSR1, quantum_start, NULL);
    preparer_signal (SIGUSR2, quantum_end,   &sigusr2);

    // création des processus fils
    pid_t *tab = process_factory (nbproc, quanta);

    // init. des actions pour les signaux du proc. principal
    // on remet action précédente pour SIGUSR2
    preparer_signal (SIGUSR2, sigusr2.sa_handler, NULL);
    preparer_signal (SIGUSR1, acquittement, NULL);
    preparer_signal (SIGCHLD, process_term, NULL);
    preparer_signal (SIGALRM, quantum_end,  NULL);

    // init. des masques de signaux pour les sections critiques
    sigset_t vide, mask_event;
    CHK (sigemptyset (&vide));
    CHK (sigemptyset (&mask_event));
    CHK (sigaddset (&mask_event, SIGALRM));
    CHK (sigaddset (&mask_event, SIGCHLD));
    CHK (sigaddset (&mask_event, SIGUSR1));

    int pcur = -1; // indice du processus courant

    ack = 1;       // phase d'init pour lancer l'ordonnancement

    int state = ETAT_RIEN;
    while (actifs > 0) {
        int pterm = -1; // indice du processus qui a terminé

        // si rien à faire, attente passive
        CHK (sigprocmask (SIG_BLOCK, &mask_event, NULL));
        while (event_q == DEBUT_QT && fin_fils == 0 && ack == 0) {
            sigsuspend (&vide);
            if (errno != EINTR)
                raler (1, "sigsuspend");
        }
        CHK (sigprocmask (SIG_UNBLOCK, &mask_event, NULL));

        // Gestion des événements

        /* réception SIGCHLD => fin d'un fils -----------------------------*/
        if (fin_fils > 0) {
            actifs--;       // 1 processus actif de moins
            fin_fils--;     // d'autre fils peuvent avoir terminé
                            // on les traite à la prochaine itération

            // enregistrement terminaison
            r_val = term_process (tab, &pterm);

            if (pterm == pcur) { // si processus courant qui a terminé

                if (state == ETAT_SURP) { // il a été relancé pour rien

                    alarm (0);              // annule l'émission de SIGALRM
                    event_q = DEBUT_QT;     // si SIGALRM reçu entre temps
                                            // on annule son traitement
                    state = ETAT_RIEN;

                // reception de SIGCHLD avant SIGUSR1
                } else if (state == ETAT_ATTENTE_ACK) {
                    PRINT ("EVIP - process %d\n", pcur);
                    state = ETAT_RIEN;
                }

                if (actifs > 0)      // s'il reste des procressus actifs
                    ack = 1;         // en placer 1 nouveau sur le CPU
            }

            PRINT ("TERM - process %d\n", pterm);
        }

        /* réception SIGALRM => fin quantum -------------------------------*/
        if (event_q == FIN_QT) {
            event_q = DEBUT_QT;
            CHK (kill (tab [pcur], SIGUSR2));
            state = ETAT_ATTENTE_ACK;
        }

        /* réception SIGUSR1 => ACK éviction ------------------------------*/
        if (ack == 1) {
            ack = 0;
            if (state == ETAT_ATTENTE_ACK) {
                PRINT ("EVIP - process %d\n", pcur);
                state = ETAT_RIEN;
            }

            // élection d'un nouveau processus
            if (actifs > 0) {
                pcur = select_process (tab, nbproc, pcur);
                CHK (kill (tab [pcur], SIGUSR1));
                alarm (duree_qtum);
                state = ETAT_SURP;
            }
        }
    }

    free (tab);
    free (quanta);
    return r_val;
}
