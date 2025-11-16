#include "projet.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

//Ce fichier contient les fonctions des commandes intégrées (cd, help, my_exit,cp).

// Tableau de chaines de caractères avec les noms des commandes internes 
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "cp"
};

//Tableau de pointeurs vers des fonctions
int (*builtin_func[]) (char **) = {
    &cd,
    &help,
    &my_exit,
    &my_cp
};

//Fonction qui retourne le nombre de commandes intégrées
int num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/* Commande cd */
int cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "argument manquant à \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) { 
            perror("Erreur ");
        }
    }
    return 1;
}

/* Commande help */
int help(char **args) {
    int i;
    printf("Cassandra's, Sira's, and Laeticia's shell\n");
    printf("Tapez les noms des programmes et les arguments, puis appuyez sur la touche « Entrée ».\n");
    printf("Les commandes suivants sont disponibles :\n");

    for (i = 0; i < num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }
    printf("Utilise la commande \"man\" pour des informations sur les autres commandes.\n");
    return 1;
}

/* Commande exit */

int my_exit(char **args) {
    int exit_code = 0;  

    if (args[1] != NULL) {  
        exit_code = atoi(args[1]);  //conversion d l'argument en entier
    }

    printf("Fermeture du shell avec le code %d...\n", exit_code);

    free(args);
    

    exit(exit_code); //quite le programm avec le code donné
    return 0;  
}

/* Commande cp */
int my_cp(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: cp <source> <destination>\n");
        return 1;
    }

    struct stat src_stat;
    if (stat(args[1], &src_stat) == -1) {
        perror("Erreur d'accès au fichier/répertoire source");
        return 1;
    }

    if (S_ISDIR(src_stat.st_mode)) {
        copy_recursive(args[1], args[2]);
    } else {
        return copyfile(args[1], args[2]);
    }

    return 1;
}
