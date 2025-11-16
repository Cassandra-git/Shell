#include "projet.h"
//Ce fichier contient les fonctions pour exécuter les commandes et gérer les processus.


/* Lire une ligne */
char *read_line() {
    size_t buffs = BUFSIZE; //Taille initiale du buffer déclaré dans projet.h
    int position = 0;
    char *buffer = malloc(sizeof(char) * buffs);
    int c;

    if (!buffer) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar(); //lectur d'un caractère
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        //Reallocation si a ligne depasse la taille initiale
        if (position >= buffs) {
            buffs += BUFSIZE;
            buffer = realloc(buffer, buffs);
            if (!buffer) {
                fprintf(stderr, "Allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/* Découper la ligne en arguments, enregistre si  on a des redirections et detecte si le processus est lancé en arrièr plan*/
char **split_line(char *line, char **input_file, char **output_file, int *append_mode,int *background){

    int bufsize = TAILLE_MOT, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token = strtok(line, DELIM);

    if (!tokens) {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (token != NULL) {
        if (strcmp(token, ">") == 0) {
            token = strtok(NULL, DELIM);
            *output_file = token;
            *append_mode = 0;
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, DELIM);
            *output_file = token;
            *append_mode = 1;
        } else if (strcmp(token, "<") == 0) {
            token = strtok(NULL, DELIM);
            *input_file = token;
        }else if (strcmp(token, "&") == 0) {
            *background = 1;
        } 
        else {
            tokens[position++] = token;
        }

        if (position >= bufsize) {
            bufsize += TAILLE_MOT;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                fprintf(stderr, "allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}


/* Exécuter une commande */
int execute(char **args) {
    if (args[0] == NULL)
        return 1;

    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    // Si ce n'est pas une commande interne, elle sera exécutée plus tard avec redirection
    return 1;
}

/* Lance une commande externe*/
int launch(char **args, int infile, int outfile) {
    pid_t pid;
    int status;

    pid = fork(); //Cree un processus
    if (pid == 0) {
        if (infile != STDIN_FILENO) {
            //Redirige entrée/sortie avec dup2
            dup2(infile, STDIN_FILENO);
            close(infile);
        }

        if (outfile != STDOUT_FILENO) {
            dup2(outfile, STDOUT_FILENO);
            close(outfile);
        }
        //execvp pour executer la commande
        if (execvp(args[0], args) == -1) {
            perror("Erreur d'exécution");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Erreur de fork");
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}


int execute_with_pipe(char *line) {
    int pipefd[2];
    pid_t pid1, pid2;

    // Séparer la ligne en deux commandes autour de "|"
    char *cmd1 = strtok(line, "|");
    char *cmd2 = strtok(NULL, "|");

    if (cmd2 == NULL) {
        fprintf(stderr, "Erreur : commande pipe incorrecte\n");
        return 1;
    }

    pipe(pipefd); // Création du pipe

    /*Le premir processus ecrit dans le pip et le second lit du pipe.Chaque commande est traitée par split_line puis execvp()*/
    pid1 = fork();
    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        char *input_file = NULL, *output_file = NULL;
        int append_mode = 0, background = 0;
        char **args = split_line(cmd1, &input_file, &output_file, &append_mode, &background);
        execvp(args[0], args);
        perror("Erreur execvp");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);

        char *input_file = NULL, *output_file = NULL;
        int append_mode = 0, background = 0;
        char **args = split_line(cmd2, &input_file, &output_file, &append_mode, &background);
        execvp(args[0], args);
        perror("Erreur execvp");
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 1;
}



// étape 1: copie un fichier
int copyfile(const char *source, const char *destination) {
    struct stat file_stat;
    
    // étape 2: récupération des permissions du fichier
    if (stat(source, &file_stat) == -1) {
        perror("Erreur lors de la récupération des permissions du fichier source");
        return 1;
    }
                
    // étape 1: ouverture du fichier source en lecture seule
    int idesc = open(source, O_RDONLY);
    if (idesc == -1) {   
        perror("Erreur d'ouverture du fichier source");
        return 1;
    }
     
    // étape1:création du fichier cible avec les meme permissions que le fichier source
    int odesc = open(destination, O_WRONLY | O_CREAT | O_TRUNC, file_stat.st_mode);
    if (odesc == -1) {
        perror("Erreur d'ouverture du fichier destination");
        close(idesc);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t rcnt;
        
    // étape1:boucle de lecture et écriture par blocs
    while ((rcnt = read(idesc, buffer, sizeof(buffer))) > 0) {
        ssize_t total_written = 0;
        while (total_written < rcnt) {
            ssize_t wcnt = write(odesc, buffer + total_written, rcnt - total_written);
            if (wcnt == -1) {
                perror("Erreur lors de l'écriture");
                close(idesc);
                close(odesc);
                return 1;
            }
            total_written += wcnt;
        }
    }

    if (rcnt == -1) {
        perror("Erreur lors de la lecture");
    }
        
    close(idesc);
    close(odesc);

    // etape2: appliquer les mêmes permissions que le fichier source
    if (chmod(destination, file_stat.st_mode) == -1) {
        perror("Erreur des permissions au fichier destination");
        return 1;
    }
        
    printf("Copie réussie : %s -> %s\n", source, destination);
    return (rcnt == -1) ? 1 : 1;
}

// étape3: fonction qui construit un chemin complet à partir d'un répertoire et d'un fichier
char *build_path(const char *dir, const char *filename) {
    size_t len_dir = strlen(dir);
    size_t len_file = strlen(filename);
    size_t need_slash = (len_dir > 0 && dir[len_dir - 1] != '/') ? 1 : 0;

    //allouer mémoire
    char *full_path = malloc(len_dir + len_file + need_slash + 1);
    if (!full_path) {
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }

    // construire le chemin
    strcpy(full_path, dir);
    if (need_slash) strcat(full_path, "/");
    strcat(full_path, filename);

    return full_path;
}

// étape4: fonction récursive pour copier un répertoire et son contenu
void copy_recursive(const char *src_dir, const char *dest_dir) {
    DIR *dir = opendir(src_dir);
    if (!dir) {
        perror("Impossible d'ouvrir le répertoire source");
        exit(EXIT_FAILURE);
    }

    struct stat dir_stat;
    if (stat(src_dir, &dir_stat) == -1) {
        perror("Erreur lors de l'accès aux informations du répertoire source");
        closedir(dir);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // etape3: construire les chemins source et destination
        char *src_path = build_path(src_dir, entry->d_name);
        char *dest_path = build_path(dest_dir, entry->d_name);

        struct stat entry_stat;
        if (stat(src_path, &entry_stat) == -1) {
            perror("Erreur lors de l'accès aux métadonnées");
            free(src_path);
            free(dest_path);
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode)) {
            // si c'est un répertoire:appel récursif
            copy_recursive(src_path, dest_path);
        } else {
            // sinon c'est un fichier on le copie
            copyfile(src_path, dest_path);
        }

        free(src_path);
        free(dest_path);
    }

    closedir(dir);
}
