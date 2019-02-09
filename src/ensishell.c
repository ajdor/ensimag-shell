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

int jobs[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int jobs_index = 0;

void execCommands(struct cmdline *pCmdline);

void execPipe();

void execBackground(pid_t pid);

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
    printf("exit\n");
    exit(0);
}

void execPipe(struct cmdline *l) {
}

void execJobs(pid_t pid) {
    int status;
    printf("[JOBS] Parent pid: %d\n", pid);
    for (int i = 0; i < 10; i++) {
        if (jobs[i] != -1 && waitpid(jobs[i], &status, WNOHANG)) {
            printf("[JOB] %d\n", jobs[i]);
            printf("\t[JOB STATUS] %d\n", status);
            if (WIFEXITED(status)) {
                printf("\texited, status=%d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("\tkilled by signal %d\n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("\tstopped by signal %d\n", WSTOPSIG(status));
            } else if (WIFCONTINUED(status)) {
                printf("\tcontinued\n");
            }
        }
    }
}

void execCommands(struct cmdline *l) {
    pid_t pid;
    /* l->seq[0] is the first command and l->seq[1] is the second if a pipe is used */
    int status = 0;
    switch (pid = fork()) {
        case -1:
            printf("/!\\ FORK FAILED /!\\\n");
            break;
        case 0:
            setpgid(pid, 0);
            if (strcmp(*l->seq[0], "jobs") == 0) {
                execJobs(pid);
            } else {
                execvp(*(l->seq[0]), *(l->seq));
                if (status < 0) {
                    printf("/!\\ command {%s} not found /!\\\n", *(l->seq[0]));
                } else {
                    printf("Executed %s with return code %d\n", *(l->seq[0]), status);
                    perror("/!\\ ERROR /!\\");
                }
                exit(1);
            }

            break;
        default:
            /* Ignore dead children */
            /* Temporary workaround before implementing SIGCHLD handler */
            signal(SIGCHLD, SIG_IGN);
            if (!l->bg) {
                waitpid(pid, 0, 0);
            } else {
                waitpid(-1, &status, WNOHANG);
                jobs[jobs_index] = pid;
                jobs_index++;
                if (WIFEXITED (status))
                    printf("Exited with code %d\n", WEXITSTATUS (status));
            }
            break;
    }
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
//        int i, j;
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
        execCommands(l);

        /* Check if any children process are done to remove them from jobs */
        pid_t deadPid;
        int deadStatus;
        while ((deadPid = waitpid(-1, &deadStatus, WNOHANG)) > 0) {
            printf("[proc %d exited with code %d]\n",
                   deadPid, WEXITSTATUS(deadStatus));
            /* here you can remove the pid from your jobs list */
            for (int i = 0; i < 10; i++) {
                if (jobs[i] == deadPid) {
                    jobs[i] = -1;
                    jobs_index--;
                }
            }
        }

        if (l->err) {
            /* Syntax error, read another command */
            printf("error: %s\n", l->err);
            continue;
        }

//        if (l->in) printf("in: %s\n", l->in);
//        if (l->out) printf("out: %s\n", l->out);
//        if (l->bg) printf("background (&)\n");

        /* Display each command of the pipe */
//        for (i = 0; l->seq[i] != 0; i++) {
//            char **cmd = l->seq[i];
//            printf("seq[%d]: ", i);
//            for (j = 0; cmd[j] != 0; j++) {
//                printf("'%s' ", cmd[j]);
//            }
//            printf("\n");
//        }
    }
}
