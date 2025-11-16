#ifndef PROJET_H
#define PROJET_H

/*

Les bibliothèques nécessaires

Les prototypes de fonctions

Les définitions des structures

Les constantes utilisées


*/


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_CMDS 10
#define BUFSIZE 1024
#define TAILLE_MOT 64
#define DELIM " \t\r\n\a"
#define BUFFER_SIZE 4096

typedef struct command {
    char **args;         // Liste des arguments de la commande
    char *input_file;    // Fichier pour la redirection d'entrée ("<")
    char *output_file;   // Fichier pour la redirection de sortie (">")
    struct command *next; // Commande suivante (si "pipe" utilisé)
} command;

/* Structure pour représenter un processus */
typedef struct process {
    struct process *next;
    char **argv;
    pid_t pid;
    char completed;
    char stopped;
    int status;
} process;

/* Structure pour représenter un job */
typedef struct job {
    struct job *next;
    char *command;
    process *first_process;
    pid_t pgid;
    char notified;
    struct termios tmodes;
    int stdin, stdout, stderr;
} job;

/* Variables globales */
extern pid_t shell_pgid;
extern struct termios shell_tmodes;
extern int shell_terminal;
extern int shell_is_interactive;
extern job *first_job;

 

/* Fonctions du shell */
void init_shell();
job* find_job(pid_t pgid);
int job_is_stopped(job *j);
int job_is_completed(job *j);
char ***split_pipeline(char *line, int *num_cmds);


/* Fonctions utilitaires */
char *read_line();
char **split_line(char *line,char **input_file, char **output_file, int *append_mode,int *background);
int execute(char **args);
int launch(char **args, int infile, int outfile);
int execute_with_pipe(char *line);


/* Fonctions intégrées (built-in) */
int cd(char **args);
int help(char **args);
int my_exit(char **args);
int my_cp(char **args);
int num_builtins();

void copy_recursive(const char *src_dir, const char *dest_dir);
int copyfile(const char *source, const char *destination);
extern char *builtin_str[];
extern int (*builtin_func[]) (char **);

#endif /* PROJET_H */
