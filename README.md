# Programmation Système
## [Cours]((https://pdagog.gitlab.io/ens/cours-ps.pdf))
Utilisation de primitives systèmes en C

- Gestion des fichiers
- Gestion des phériphériques
- Gestion des processus
- Gestion du temps
- Gestion des tubes
- Gestion des signaux

## [Exercices](https://pdagog.gitlab.io/ens/exercices.pdf)

Pour l'ensemble des programmes, on utilisera u ensmeble de macros contenues dans le fichier `macro.h` qui nous permettrons de vérifier les valeurs de retour des primitives systèmes.

- Gestion des fichiers :
    - Exercice 5.7 : copie l'entrée standard sur la sortie standard avec fonctions bibliothèques et primitives systèmes et comparaison du temps nécessaire
    - Exercice 5.8 : affiche le type de fichier et ses permissions
    - Exercice 5.9 :  création librarie standard entrée sortie avec des primitives systèmes
    - Exercice 5.10 : affiche les objets d'un répertoire
    - Exercice 5.14 : cherche la localisation d'une commande

- Gestion des processus :
    - Exercice 6.1 : génération d'un processus fils et affichage des différents pid
    - Exercice 6.2 : lance n processus fils et affiche leur pid
    - Exercice 6.4 : lecture de caractères avec processus père et fils
    - Exercice 6.5 : utilisation de exec avec fork
    - Exercice 6.8 : algorithme de tri de complexité nulle (sous process et sleep)
    - Exercice 6.9 : comandes avec redirection

- Gestion des signaux :
    - Exercice 9.1 : compteur de `SIGINT`
    - Exercice 9.3 : attends un signal et affiche sa signification
    - Exercice 9.5 : signaux père et fils
    - Exercice 9.7 : ecriture de la date dans un fichier lors de la reception de signaux
    - Exercice 9.10 : simulation coupleur série avec signaux

- TP : 
    - [TP n°1](https://moodle.unistra.fr/pluginfile.php/1010991/mod_resource/content/3/tp1.pdf) : fonction comparant 2 fichiers
    - [TP n°2](https://moodle.unistra.fr/pluginfile.php/1087930/mod_resource/content/3/tp2.pdf) : simulation de l'ordonnancement des processus sur un monoprocessrur
    - TP noté : réalisation d'un shell