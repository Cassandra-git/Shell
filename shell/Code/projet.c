#include "projet.h"



// Fichier principal

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;
job *first_job = NULL;

/* Initialisation du shell */
void init_shell() {
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp())) {
            kill(-shell_pgid, SIGTTIN);
        }

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        tcsetpgrp(shell_terminal, shell_pgid);
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

/* Fonction principale */
int main() {
    char *line;
    char **args;
    int status = 1;
    char cwd[1024];
    char *input_file = NULL;
    char *output_file = NULL;
    int append_mode = 0;
    int background = 0;

    init_shell();

    do {
        if (getcwd(cwd, sizeof(cwd)) != NULL) { // Affiche le prompt
            printf("CLS-prompt-%s$> ", cwd);
        } else {
            perror("getcwd error");
        }

        input_file = NULL;
        output_file = NULL;
        append_mode = 0;
        background = 0;

        line = read_line();
        if (strchr(line, '|') != NULL) { // Detecte le pipe et appelle execute_pipe
        execute_with_pipe(line);
        free(line);
        continue;
}
	
        args = split_line(line, &input_file, &output_file, &append_mode, &background);

        // Gestion des redirections 
        int infile = STDIN_FILENO;
        int outfile = STDOUT_FILENO;

        if (input_file != NULL) {
            infile = open(input_file, O_RDONLY);
            if (infile < 0) {
                perror("Erreur fichier entrée");
                free(line);
                free(args);
                continue;
            }
        }

        if (output_file != NULL) {
            int flags = O_WRONLY | O_CREAT | (append_mode ? O_APPEND : O_TRUNC);
            outfile = open(output_file, flags, 0644);
            if (outfile < 0) {
                perror("Erreur fichier sortie");
                free(line);
                free(args);
                continue;
            }
        }

        //Si commande interne l'execute sinon utilise lauch pour créer un processus

        if (args[0] != NULL) {
            int is_builtin = 0;
            for (int i = 0; i < num_builtins(); i++) {
                if (strcmp(args[0], builtin_str[i]) == 0) {
                    status = (*builtin_func[i])(args);
                    is_builtin = 1;
                    break;
                }
            }

            if (!is_builtin) {
                if (background) {
                    if (fork() == 0) {
                        launch(args, infile, outfile);
                        exit(0);
                    }
                } else {
                    status = launch(args, infile, outfile);
                }
            }
        }

        // Fermer les fichiers s'ils ont été ouverts
        if (infile != STDIN_FILENO) close(infile);
        if (outfile != STDOUT_FILENO) close(outfile);

        free(line);
        free(args);
    } while (status);

    return 0;
}


