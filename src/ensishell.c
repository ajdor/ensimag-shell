/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* To disable Scheme interpreter (Guile support), comment the
 * following line.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */
#define USE_GUILE

#ifdef USE_GUILE
#include <guile/2.0/libguile.h>

int executer(char *line)
{
	/* change this code to execute the command line */
	/* identical to standard execution */
	printf("Not implemented: can not execute %s", line);

	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(executer(scm_to_latin1_stringn(x, 0)));
}
#endif



int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#ifdef USE_GUILE
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1) {
		struct cmdline *l;
		char *line;
		int i, j;
		char *prompt = "ensishell>";

		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			printf("exit\n");
			exit(0);
		}

#ifdef USE_GNU_READLINE
		add_history(line);
#endif


#ifdef USE_GUILE
		if (line[0] == '(') {
/* The line is a scheme command */
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_latin1_string(catchligne));
                        continue;
                }
#endif

		l = parsecmd(line);

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}
		

		
		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) printf("background (&)\n");

		/* Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
                        for (j=0; cmd[j]!=0; j++) {
                                printf("'%s' ", cmd[j]);
                        }
			printf("\n");
		}
	}
}
