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

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

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
    printf("Not implemented yet: can not execute %s\n", line);

    /* Remove this line when using parsecmd as it will free it */
    free(line);

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


typedef struct jobs {
    pid_t pid;
    char *cmd;
    struct jobs *next;
} jobs;

/* Verbose mode is OFF by default */
int verbose = 0;

static jobs *background_jobs = NULL;

void exec_pipe(struct cmdline *pCmdline);


void toggle_verbose();

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
        exit(0);
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
    exit(0);
}

void exec_pipe(struct cmdline *pCmdline) {
    int pipe_descriptor[2];
    int pipe_status = pipe(pipe_descriptor);
    if (pipe_status == -1) {
        perror("Pipe has failed");
    } else {
        if (fork() == 0) {
            dup2(pipe_descriptor[0], 0);
            close(pipe_descriptor[1]);
            execvp(*(pCmdline->seq[1]), *(pCmdline->seq + 1));
        }
        dup2(pipe_descriptor[1], 1);
        close(pipe_descriptor[0]);
        execvp(*(pCmdline->seq[0]), *(pCmdline->seq));
    }
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
    /* pCmdline->seq[0] is the first command and pCmdline->seq[1] is the second if a pipe is used */
    pid_t pid;
    int status = 0;
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

            /* Special commands */
            if (strcmp(*pCmdline->seq[0], "exit") == 0) {
                exit(0);
            } else if (strcmp(*pCmdline->seq[0], "jobs") == 0) {
                exec_jobs();
            } else if (strcmp(*pCmdline->seq[0], "v") == 0) {
                toggle_verbose();
            }
                /* Regular commands */
            else {
                if (pCmdline->seq[1]) {
                    exec_pipe(pCmdline);
                } else {
                    status = execvp(*(pCmdline->seq[0]), *(pCmdline->seq));
                    if (status == -1) {
                        printf("/!\\ command %s not found /!\\\n", *(pCmdline->seq[0]));
                        exit(0);
                    }
                }
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
    exit(0);
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
            free(line);
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
        exec_commands(l);

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
