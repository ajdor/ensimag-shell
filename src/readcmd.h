/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

/*
 * changelog: add background, 2010, Grégory Mounié
 */

#ifndef __READCMD_H
#define __READCMD_H

/* If GNU Readline is not available, comment the  next line to use readcmd
   internal simple line reader */
#define USE_GNU_READLINE

/* Read a command line from input stream. Return null when input closed.
Display an error and call exit() in case of memory exhaustion. */
struct cmdline *readcmd(char *prompt);


/* Structure returned by readcmd() */
struct cmdline {
	char *err;	/* If not null, it is an error message that should be
			   displayed. The other fields are null. */
	char *in;	/* If not null : name of file for input redirection. */
	char *out;	/* If not null : name of file for output redirection. */
        int   bg;       /* If set the command must run in background */ 
	char ***seq;	/* See comment below */
};

/* Field seq of struct cmdline :
A command line is a sequence of commands whose output is linked to the input
of the next command by a pipe. To describe such a structure :
A command is an array of strings (char **), whose last item is a null pointer.
A sequence is an array of commands (char ***), whose last item is a null
pointer.
When the user enters an empty line, seq[0] is NULL.
*/
#endif
