#include "chaine.h"

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

/** @brief Calcule la taille d'une ligne
 *
 *  Calcule la taille de la première ligne (nombre d'octets jusqu'au
 *  premier '\n') comprise dans la chaine de caractères buf de taille
 *  len lue depuis le descripteur desc. Si buf contient plusieures
 *  lignes, alors la tête de lecture sur desc est repositionnée juste
 *  après cette première ligne.
 *
 *  @param desc descripteur depuis lequel les octets dans buf sont issus.
 *  @param buf  chaine de caractères contenant une ou plusieurs lignes.
 *  @param len  longueur de la chaine -1 car dernière place pour '\0'.
 *  @return     longueur de la première ligne '\n' inclu.
 */
ssize_t taille_ligne (int desc, char *buf, ssize_t len)
{
    int i = 0;
    while (i < len && buf [i] != '\n')
        i++;

    ssize_t r_val = len;
    if (i != len - 1 && i != len) {
        CHK (lseek (desc, i + 1 - len, SEEK_CUR));
        r_val = i + 1;
    }

    return r_val;
}

/** @brief Remplace de multiples occurences de ' ' par une seule
 *
 *  Remplace les occurences multiples de ' ' par une seule et ajuste
 *  la taille de la chaîne en conséquence. Si la chaîne termine par ' '
 *  ou " \n" elle supprime le dernier ' '
 *
 *  @param buf la chaine de caractères à modifier
 *  @param len la longueur de la chaine.
 *  @return la nouvelle taille après modification
 */
ssize_t translate (char *buf, ssize_t len)
{
    int space = 0;
    for (int i = 0 ; i < len ; i++) {
        if (buf [i] == ' ') {
            if (space == 0)
                space = 1;
            else {
                for (int j = i ; j > 0 && j < len ; j++)
                    buf [j - 1] = buf [j];
                len--;
                i--;
            }
        } else
            space = 0;
    }

    return len;
}

/** @brief Découpe une chaine de caractères en mots.
 *
 *  La chaine de caractères doit se terminer par le caractère
 *  '\n'. Attention, elle ne comporte pas de '\0'. Place des pointeurs
 *  sur chaque mot dans un tableau alloué dynamiquement et terminé par
 *  NULL.
 *
 *  @param buf la chaine de caractères à découper, ne contient qu'une commande.
 *  @param len la longueur de la chaine -1 car dernière place pour \0
 *  @return une pointeur sur un tableau qui contient tous les mots.
 */
char **decompose (char *buf, ssize_t len)
{
    // supression des occurences multiples de ' '
    len = translate (buf, len);

    // gestion lignes multiples dans buf
    len = taille_ligne (0, buf, len);

    // compte le nbr de mots pour éviter d'appeler plusieurs fois realloc
    int nb_mots = 0, space = 0;
    for (int i = 0 ; i < len ; i++) {
        // les mots sont délimités par ' ' ou '\n'
        if (buf [i] == '\n' && space == 0) {
            nb_mots++;
        } else if (buf [i] == ' ') {
            nb_mots++;
            space = 1;
        } else {
            space = 0;
        }
    }

    char **tab_cmd;
    // alloue une case suppl. pour marquer la fin du tableau par NULL
    // utile pour la fonction free_tab
    CHKP (tab_cmd = malloc ((nb_mots + 1) * sizeof *tab_cmd));
    tab_cmd [nb_mots] = NULL;

    buf [len] = '\0';             // pour strtok
    if (buf [len - 1] == '\n')
        buf [len - 1] = '\0';

    for (int i = 0 ; i < nb_mots ; i++) {
        char *mot = strtok (buf, " ");
        // alloue une case suppl. pour le \0 final
        CHKP (tab_cmd [i] = malloc (strlen (mot) + 1));
        strcpy (tab_cmd [i], mot);
        buf = NULL;                // obligatoire pour futurs appels à strtok
    }

    return tab_cmd;
}

/** @brief libère la mémoire d'un tableau de mots
 *
 * Le tableau doit être terminé par NULL.
 *
 * @param tab_cmd un tableau de pointeurs sur des mots
 * @return Void.
 */
void free_tab (char **tab_cmd)
{
    int i = 0;
    while (tab_cmd [i] != NULL)
        free (tab_cmd [i++]);

    free (tab_cmd);

    return;
}
