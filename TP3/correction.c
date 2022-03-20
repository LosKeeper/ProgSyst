/**
 * Les tubes ont une capacité de stockage limitée. Un interblocage
 * peut se produire car les stations ont un comportement séquentiel :
 * elles doivent d'abord envoyer toutes leurs trames (donc écrire sur
 * le tube commun) et seulement ensuite récupérer les trames
 * transmises par le commutateur (donc lire sur leurs tubes
 * personnels). Si une station a toujours des trames à transmettre
 * alors que le commutateur a rempli le tube personnel de cette
 * station, ce dernier est bloqué en écriture et donc ne peut plus
 * lire (i.e. vider) le tube commun. Si dans le même temps le tube
 * commun est rempli (puisque le commutateur ne le vide plus), la
 * station en question sera également bloquée en écriture sur le tube
 * commun, ce qui amène à un interblocage : le commutateur attend que
 * la station vide son tube pour y écrire, et la station attend que le
 * commutateur vide le tube commun pour y écrire. Il est possible de
 * résoudre ce problème avec la primitive « poll » qui permet
 * d'attendre des événements sur un descripteur.
 */

#include <stdio.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CHK(op) do { if ((op) == -1) raler (1, #op); } while (0)

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

#define MAXSTA 10       // nbr max de STA fixé dans le sujet
#define PATH 256        // taille du nom max pour fichier STA_X
#define PAYLOAD_SIZE 4  // taille payload fixé dans le sujet

// une entrée dans les fichiers STA_X
struct file_entry {
    int dst;
    char payload [PAYLOAD_SIZE];
};

// une trame envoyée sur un tube
struct trame {
    int src;
    int dst;
    char payload [PAYLOAD_SIZE];
};

/**
 * Fonction exécutée par les processus fils : lecture d'un fichier
 * STA_X (si existant) et envoie des trames correspondantes puis
 * attente de trames sur tube dédié
 *
 * @param addr adresse de la station courante
 * @param port_tx descripteur pour joindre le commutateur
 * @param port_rx descripteur pour recevoir depuis le commutateur
 * @return void
 */
noreturn void station (int addr, int port_tx, int port_rx)
{
    // transmission des messages
    char filename [PATH];
    int n = snprintf (filename, PATH, "STA_%d", addr);
    if (n < 0 || n >= PATH)
        raler (0, "snprintf fils %d", addr);

    errno = 0;
    int d;
    if ((d = open (filename, O_RDONLY)) == -1) {
        if (errno != ENOENT)                   // si le fichier n'existe pas
            raler (1, "open fils %d", addr);   // la station n'a rien à
    }                                          // transmettre

    struct trame t;

    // si fichier existe, on doit transmettre des trames
    if (errno != ENOENT) {
        t.src = addr;
        struct file_entry e;

        while (read (d, &e, sizeof e) > 0) {
            t.dst = e.dst;
            memcpy (t.payload, e.payload, PAYLOAD_SIZE);
            CHK (write (port_tx, &t, sizeof t));
        }

        CHK (close (d));
    }

    // plus de trames à transmettre, on ferme
    CHK (close (port_tx));

    // réception des messages
    while (read (port_rx, &t, sizeof t) > 0) {
        char payload [PAYLOAD_SIZE + 1] = {0};
        memcpy (payload, t.payload, PAYLOAD_SIZE);
        printf ("%d - %d - %d - %s\n", addr, t.src, t.dst, payload);
    }

    CHK (close (port_rx));

    exit (EXIT_SUCCESS);
}

/**
 * Création des processus fils (les stations)
 *
 * @param nb_sta nombre de processus à créer
 * @param tubes descripteurs des différents tubes :
 * tubes [0] => tube commun
 * tubes [x] => tube pour la station X
 * @result void
 */
void process_factory (int nb_sta, int tubes [MAXSTA+1][2])
{
    // 1 tube commun pour STA vers COMUT
    CHK (pipe (tubes[0]));

    for (int i = 1 ; i < nb_sta + 1; i++) {

        // 1 tube par STA pour COMUT vers STA
        CHK (pipe (tubes [i]));

        switch (fork ()) {
        case -1:
            raler (1 , "fork");

        case 0:
            for (int j = 1 ; j < i ; j++)
                CHK (close (tubes [j][1])); // côté wr des tubes précédents

            CHK (close (tubes [0][0])); // côté rd tube commun
            CHK (close (tubes [i][1])); // côté wr tube dédié

            station (i, tubes [0][1], tubes [i][0]);

        default:
            CHK (close (tubes [i][0])); // côté rd tube dédié
        }
    }

    CHK (close (tubes [0][1])); // côté wr tube commun
    return;
}

/**
 * Attente de la terminaison de tous les processus fils (les stations)
 *
 * @param nb_sta nombre de processus fils total
 * @return 1 si au moins un fils s'est mal terminé, 0 sinon
 */
int attendre_fils (int nb_sta)
{
    int r_val = 0, raison;

    for (int i = 0 ; i < nb_sta ; i++) {
        CHK (wait (&raison));
        if (! WIFEXITED (raison) || WEXITSTATUS (raison) != 0) {
            fprintf (stderr, "fils mal terminé");
            r_val = 1;
        }
    }

    return r_val;
}

/**
 * Transmssion broadcast d'une trame depuis le commutateur
 *
 * @param t trame à transmettre
 * @param tubes descripteurs des différents tubes :
 * tubes [0] => tube commun (à ne pas utiliser ici)
 * tubes [X] => tube pour joindre la station X
 * @param nb_sta nombre de stations présentes
 * @return void
 */
void broadcast (struct trame t, int tubes[MAXSTA][2], int nb_sta)
{
    for (int i = 1 ; i <= nb_sta ; i++) {
        if (i != t.src)
            CHK (write (tubes [i][1], &t, sizeof t));
    }
}

int main (int argc, char *argv [])
{
    if (argc != 2)
        raler (0, "usage: %s nb_sta", argv [0]);

    int nb_sta;
    if (sscanf (argv [1], "%d", &nb_sta) != 1 || nb_sta < 2 || nb_sta > MAXSTA)
        raler (0, "nb_sta dans [2, %d]", MAXSTA);

    int tubes [MAXSTA+1][2]; // + 1 pour le tube commun
    process_factory (nb_sta, tubes);

    struct trame t;
    while (read (tubes [0][0], &t, sizeof t) > 0) {
        if (t.dst == 0 || t.dst > nb_sta)
            broadcast (t, tubes, nb_sta);
        else
            CHK (write (tubes [t.dst][1], &t, sizeof t));
    }

    // fermeture des tubes vers le stations
    // => provoque la fin des processus fils
    for (int i = 1 ; i < nb_sta + 1 ; i++)
        CHK (close (tubes [i][1]));

    CHK (close (tubes [0][0]));

    return attendre_fils (nb_sta);
}
