/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <wordexp.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

typedef struct jobs {
    pid_t pid;
    char *cmd;
    struct jobs *next;
} jobs;

/* Verbose mode is OFF by default */
int verbose = 0;

static jobs *background_jobs = NULL;

int parse_jokers(struct cmdline *pCmdline);

void exec_pipe(struct cmdline *pCmdline);

void exec_stdin(struct cmdline *pCmdline);

void exec_stdout(struct cmdline *pCmdline);

void exec_commands(struct cmdline *pCmdline);

void toggle_verbose();

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1

#include <libguile.h>

int question6_executer(char *line) {
    /* Question 6: Insert your code to execute the command line
     * identically to the standard execution scheme:
     * parsecmd, then fork+execvp, for a single command.
     * pipe and i/o redirection are not required.
     */
    struct cmdline *l;
    l = parsecmd(&line);
    exec_commands(l);
    // free(l);
    return 0;
}

SCM executer_wrapper(SCM x) {
    return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}

#endif

void terminate(char *line) {
#if USE_GNU_READLINE == 1
    /* rl_clear_history() does not exist yet in centOS 6 */
    clear_history();
#endif
    if (line)
        free(line);
    printf("thanks for using ensishell :)\n");
    exit(0);
}

void exec_background(pid_t pid, char *cmd) {
    int status;
    /* Execute process in background */
    waitpid(pid, &status, WNOHANG);

    /* Add process to the background jobs list*/
    jobs *new_job = malloc(sizeof(jobs));
    new_job->pid = pid;
    /* Initialize to the size of the command's name */
    new_job->cmd = (char *) malloc(sizeof(cmd) + 1);
    strcpy(new_job->cmd, cmd);
    new_job->next = background_jobs;
    background_jobs = new_job;
}

void exec_jobs() {
    /* Return nothing if no background job is running */
    if (background_jobs == NULL) {
        printf("no background job running\n");
        return;
    }

    int status;
    struct jobs *current = background_jobs;
    struct jobs *prev = background_jobs;
    do {
        status = kill(current->pid, 0);
        if (status == 0) {
            printf("[JOB] RUNNING pid: %d, cmd: %s\n", current->pid, current->cmd);
            prev = current;
            current = current->next;
        } else if (status == -1) {
            /* The process to remove is at the beginning of the list*/
            printf("[JOB] DONE pid: %d, cmd: %s\n", current->pid, current->cmd);
            if (background_jobs == current) {
                background_jobs = current->next;
                prev = background_jobs;
            } else {
                prev->next = prev->next->next;
            }
            free(current->cmd);
            free(current);
            current = current->next;
        } else {
            printf("\t[JOB STATUS] pid: %d, status: %d\n", current->pid, status);
        }
    } while (current != NULL);

    /* In case the jobs list is emptied when checking for dead processes */
    if (background_jobs == NULL) {
        printf("no background job running\n");
    }
}

void exec_pipe(struct cmdline *pCmdline) {
    /* Count the number of process/pipes to run */
    int pipe_counter = 0;
    while (pCmdline->seq[pipe_counter] != 0)
        pipe_counter++;

    /* Create a list of pipes long enough to run between the pipes */
    int **pipe_descriptor = malloc((pipe_counter - 1) * sizeof(int *));
    for (int i = 0; i < pipe_counter - 1; i++) {
        pipe_descriptor[i] = malloc(sizeof(int));
        /* Return if a fails */
        if (pipe(pipe_descriptor[i]) == -1) {
            perror("/!\\ Pipe failed /!\\\n");
            return;
        }
    }

    /* Fork for every pipe needed for the command */
    pid_t pipe_pid = 1;
    for (int i = 0; i < pipe_counter && pipe_pid; i++) {
        pipe_pid = fork();
        if (pipe_pid == 0) {
            /* Pipe stdin if the current pipe isn't the first */
            if (i != 0) { // Si on n'est pas le premier
                dup2(pipe_descriptor[i - 1][0], STDIN_FILENO);
            }
            /* Pipe stdout if the current pipe isn't the last*/
            if (i != pipe_counter - 1) {
                dup2(pipe_descriptor[i][1], STDOUT_FILENO);
            }

            /* Close all children pipes */
            for (int j = 0; j < pipe_counter - 1; j++) {
                close(pipe_descriptor[j][0]);
                close(pipe_descriptor[j][1]);
            }

            /* Exec the command and return an error if it fails*/
            int exec_status = execvp(*(pCmdline->seq[i]), pCmdline->seq[i]);
            if (exec_status == -1) {
                perror("/!\\ Execvp failed /!\\\n");
                return;
            }
        }
    }

    /* Close all parent pipes */
    for (int j = 0; j < pipe_counter - 1; j++) {
        close(pipe_descriptor[j][0]);
        close(pipe_descriptor[j][1]);
        free(pipe_descriptor[j]);
    }
    /* Free the whole thing once and for all*/
    free(pipe_descriptor);

    /* Wait for the children to finish before prompt */
    for (int i = 0; i < pipe_counter; i++) {
        wait(NULL);
    }

    /* Necessary exit since the piping is done from the child in the main function,
     * moving it to the parent process breaks things */
    exit(0);
}

void exec_stdin(struct cmdline *pCmdline) {
    int in_fd = open(pCmdline->in, O_RDONLY);
    if (in_fd < 0) {
        printf("/!\\ file %s does not exist or could not be opened /!\\\n", pCmdline->in);
        return;
    }
    close(0);
    dup(in_fd);
    close(in_fd);
}

void exec_stdout(struct cmdline *pCmdline) {
    int out_fd = creat(pCmdline->out, 0644);
    if (out_fd < 0) {
        printf("/!\\ file %s could not be opened /!\\\n", pCmdline->out);
        return;
    }
    close(1);
    dup(out_fd);
    close(out_fd);
}

void exec_commands(struct cmdline *pCmdline) {
    /* We start by extending the jokers */
    int joker_status = parse_jokers(pCmdline);
    if(joker_status){
        /* extension has failed, so we exit */
        perror("Extentsion has failed");
        exit(0);
    }
    
    /* pCmdline->seq[0] is the first command and pCmdline->seq[1] is the second if a pipe is used */
    pid_t pid;
    int status = 0;

    /* Special commands */
    switch (pid = fork()) {
        case -1:
            printf("/!\\ FORK FAILED /!\\\n");
            break;
        case 0:
            /* Child section */
            if (pCmdline->in) {
                /* stdin redirection */
                exec_stdin(pCmdline);
            }
            if (pCmdline->out) {
                /* stdout redirection */
                exec_stdout(pCmdline);
            }
            if (pCmdline->seq[1]) {
                /* pipe */
                exec_pipe(pCmdline);
            }

            /* Regular commands */
            status = execvp(*(pCmdline->seq[0]), *(pCmdline->seq));
            if (status == -1) {
                printf("/!\\ command not found: %s /!\\\n", *(pCmdline->seq[0]));
                exit(0);
            }
            break;
        default:
            /* Parent section */
            if (!pCmdline->bg) {
                waitpid(pid, &status, 0);
            } else {
                /* Ignore dead children */
                /* Temporary workaround before implementing SIGCHLD handler */
                signal(SIGCHLD, SIG_IGN);
                exec_background(pid, *pCmdline->seq[0]);
            }
            break;
    }
}

void toggle_verbose() {
    verbose = !verbose;
    if (verbose) {
        printf("/!\\ VERBOSE /!\\\n");
    } else {
        printf("/!\\ QUIET /!\\\n");
    }
}

int parse_jokers(struct cmdline *pCmdline){
/* Modify in place the pCmdline with extended calls. */
    /* Expand the all the command lines for the program to run.  */
    for(int i =0; pCmdline->seq[i] != NULL; i++){
        wordexp_t result;
        /* I think we have to call it once before using WRDE_APPEND */
        switch (wordexp (pCmdline->seq[i][0], &result, 0)){
                case 0:			/* Successful.  */
                    break;
                case WRDE_NOSPACE:
                    /* If the error was WRDE_NOSPACE,
                    then perhaps part of the result was allocated.  */
                    wordfree (&result);
                    perror("Extension has failed");
                    return -1;
                default:                    /* Some other error.  */
                    return -1;
        }
        for(int j=1; pCmdline->seq[i][j] != NULL; j++){
              /* Expand the string for the program to run.  */
            if (wordexp (pCmdline->seq[i][j], &result, WRDE_APPEND))
            {
                /* there should be errors handling */
                wordfree (&result);
                return -1;
            }
        }
        // pCmdline->seq[i] = result.we_wordc;
        /* We need to copy the results into the command line */
        char **p = malloc(sizeof(char *)*(result.we_wordc+1));
        int j;
        for(j = 0; j < result.we_wordc; j++){
            /* We need to allocate for the size of the word + 1 
            for the null terminating char */  
            char *word = malloc((strlen(result.we_wordv[j])+1)*(sizeof(char)));
            strncpy(word, result.we_wordv[j], strlen(result.we_wordv[j])+1);
            p[j] = word;
        }
        /* We need to NULL terminate */
        p[j] = NULL;
        pCmdline->seq[i] = p;
    }
    return 0;
}


int main() {
    printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
    scm_init_guile();
    /* register "executer" function in scheme */
    scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

    while (1) {
        struct cmdline *l;
        char *line = 0;
        int i, j;
        char *prompt = "ensishell>";

        /* Readline use some internal memory structure that
           can not be cleaned at the end of the program. Thus
           one memory leak per command seems unavoidable yet */
        line = readline(prompt);
        if (line == 0 || !strncmp(line, "exit", 4)) {
            terminate(line);
        }

#if USE_GNU_READLINE == 1
        add_history(line);
#endif


#if USE_GUILE == 1
        /* The line is a scheme command */
        if (line[0] == '(') {
            char catchligne[strlen(line) + 256];
            sprintf(catchligne,
                    "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))",
                    line);
            scm_eval_string(scm_from_locale_string(catchligne));
            continue;
        }
#endif

        /* parsecmd free line and set it up to 0 */
        l = parsecmd(&line);

        /* If input stream closed, normal termination */
        if (!l) {

            terminate(0);
        }

        /* Execute commands */
        if (l->seq[0]) {
            if (strcmp(*l->seq[0], "jobs") == 0) {
                exec_jobs();
            } else if (strcmp(*l->seq[0], "v") == 0) {
                toggle_verbose();
            } else {
                exec_commands(l);
            }
        }

        if (l->err) {
            /* Syntax error, read another command */
            printf("error: %s\n", l->err);
            continue;
        }

        if (verbose) {
            if (l->in) printf("in: %s\n", l->in);
            if (l->out) printf("out: %s\n", l->out);
            if (l->bg) printf("background (&)\n");

            /* Display each command of the pipe */
            for (i = 0; l->seq[i] != 0; i++) {
                char **cmd = l->seq[i];
                printf("seq[%d]: ", i);
                for (j = 0; cmd[j] != 0; j++) {
                    printf("'%s' ", cmd[j]);
                }
                printf("\n");
            }
        }
    }
}
