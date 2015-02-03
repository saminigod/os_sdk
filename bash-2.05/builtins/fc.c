/* fc.c, created from fc.def. */
#line 23 "d:/jicamasdk/progs/bash-2.05/builtins/fc.def"

#line 48 "d:/jicamasdk/progs/bash-2.05/builtins/fc.def"

#include <config.h>

#if defined (HISTORY)
#ifndef _MINIX
#  include <sys/param.h>
#endif
#include "../bashtypes.h"
#include "posixstat.h"
#ifndef _MINIX
#  include <sys/file.h>
#endif

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <stdio.h>
#include <chartypes.h>

#include "../bashansi.h"
#include <errno.h>

#include "../shell.h"
#include "../builtins.h"
#include "../flags.h"
#include "../bashhist.h"
#include "maxpath.h"
#include <readline/history.h>
#include "bashgetopt.h"
#include "common.h"

#if !defined (errno)
extern int errno;
#endif /* !errno */

extern int echo_input_at_read;
extern int current_command_line_count;
extern int literal_history;

extern int unlink __P((const char *));

extern FILE *sh_mktmpfp __P((char *, int, char **));

/* **************************************************************** */
/*								    */
/*	The K*rn shell style fc command (Fix Command)		    */
/*								    */
/* **************************************************************** */

/* fc builtin command (fix command) for Bash for those who
   like K*rn-style history better than csh-style.

     fc [-e ename] [-nlr] [first] [last]

   FIRST and LAST can be numbers specifying the range, or FIRST can be
   a string, which means the most recent command beginning with that
   string.

   -e ENAME selects which editor to use.  Default is FCEDIT, then EDITOR,
      then the editor which corresponds to the current readline editing
      mode, then vi.

   -l means list lines instead of editing.
   -n means no line numbers listed.
   -r means reverse the order of the lines (making it newest listed first).

     fc -e - [pat=rep ...] [command]
     fc -s [pat=rep ...] [command]

   Equivalent to !command:sg/pat/rep execpt there can be multiple PAT=REP's.
*/

/* Data structure describing a list of global replacements to perform. */
typedef struct repl {
  struct repl *next;
  char *pat;
  char *rep;
} REPL;

/* Accessors for HIST_ENTRY lists that are called HLIST. */
#define histline(i) (hlist[(i)]->line)
#define histdata(i) (hlist[(i)]->data)

#define FREE_RLIST() \
	do { \
		for (rl = rlist; rl; ) { \
			REPL *r;	\
			r = rl->next; \
			if (rl->pat) \
				free (rl->pat); \
			if (rl->rep) \
				free (rl->rep); \
			free (rl); \
			rl = r; \
		} \
	} while (0)

static char *fc_dosubs __P((char *, REPL *));
static char *fc_gethist __P((char *, HIST_ENTRY **));
static int fc_gethnum __P((char *, HIST_ENTRY **));
static int fc_number __P((WORD_LIST *));
static void fc_replhist __P((char *));
#ifdef INCLUDE_UNUSED
static char *fc_readline __P((FILE *));
static void fc_addhist __P((char *));
#endif

/* String to execute on a file that we want to edit. */
#define FC_EDIT_COMMAND "${FCEDIT:-${EDITOR:-vi}}"

int
fc_builtin (list)
     WORD_LIST *list;
{
  register int i;
  register char *sep;
  int numbering, reverse, listing, execute;
  int histbeg, histend, last_hist, retval, opt;
  FILE *stream;
  REPL *rlist, *rl;
  char *ename, *command, *newcom;
  HIST_ENTRY **hlist;
  char *fn;

  numbering = 1;
  reverse = listing = execute = 0;
  ename = (char *)NULL;

  /* Parse out the options and set which of the two forms we're in. */
  reset_internal_getopt ();
  lcurrent = list;		/* XXX */
  while (fc_number (loptend = lcurrent) == 0 &&
	 (opt = internal_getopt (list, ":e:lnrs")) != -1)
    {
      switch (opt)
	{
	case 'n':
	  numbering = 0;
	  break;

	case 'l':
	  listing = 1;
	  break;

	case 'r':
	  reverse = 1;
	  break;

	case 's':
	  execute = 1;
	  break;

	case 'e':
	  ename = list_optarg;
	  break;

	default:
	  builtin_usage ();
	  return (EX_USAGE);
	}
    }

  list = loptend;

  if (ename && (*ename == '-') && (ename[1] == '\0'))
    execute = 1;

  /* The "execute" form of the command (re-run, with possible string
     substitutions). */
  if (execute)
    {
      rlist = (REPL *)NULL;
      while (list && ((sep = (char *)strchr (list->word->word, '=')) != NULL))
	{
	  *sep++ = '\0';
	  rl = (REPL *)xmalloc (sizeof (REPL));
	  rl->next = (REPL *)NULL;
	  rl->pat = savestring (list->word->word);
	  rl->rep = savestring (sep);

	  if (rlist == NULL)
	    rlist = rl;
	  else
	    {
	      rl->next = rlist;
	      rlist = rl;
	    }
	  list = list->next;
	}

      /* If we have a list of substitutions to do, then reverse it
	 to get the replacements in the proper order. */

      if (rlist && rlist->next)
	rlist = (REPL *)reverse_list ((GENERIC_LIST *) rlist);

      hlist = history_list ();

      /* If we still have something in list, it is a command spec.
	 Otherwise, we use the most recent command in time. */
      command = fc_gethist (list ? list->word->word : (char *)NULL, hlist);

      if (command == NULL)
	{
	  builtin_error ("no command found");
	  if (rlist)
	    FREE_RLIST ();

	  return (EXECUTION_FAILURE);
	}

      if (rlist)
	{
	  newcom = fc_dosubs (command, rlist);
	  free (command);
	  FREE_RLIST ();
	  command = newcom;
	}

      fprintf (stderr, "%s\n", command);
      fc_replhist (command);	/* replace `fc -s' with command */
      return (parse_and_execute (command, "fc", SEVAL_NOHIST));
    }

  /* This is the second form of the command (the list-or-edit-and-rerun
     form). */
  hlist = history_list ();
  if (hlist == 0)
    return (EXECUTION_SUCCESS);
  for (i = 0; hlist[i]; i++);

  /* With the Bash implementation of history, the current command line
     ("fc blah..." and so on) is already part of the history list by
     the time we get to this point.  This just skips over that command
     and makes the last command that this deals with be the last command
     the user entered before the fc.  We need to check whether the
     line was actually added (HISTIGNORE may have caused it to not be),
     so we check hist_last_line_added. */

  last_hist = i - 1 - hist_last_line_added;

  if (list)
    {
      histbeg = fc_gethnum (list->word->word, hlist);
      list = list->next;

      if (list)
	histend = fc_gethnum (list->word->word, hlist);
      else
	histend = listing ? last_hist : histbeg;
    }
  else
    {
      /* The default for listing is the last 16 history items. */
      if (listing)
	{
	  histend = last_hist;
	  histbeg = histend - 16;
	  if (histbeg < 0)
	    histbeg = 0;
	}
      else
	/* For editing, it is the last history command. */
	histbeg = histend = last_hist;
    }

  /* We print error messages for line specifications out of range. */
  if ((histbeg < 0) || (histend < 0) ||
      (histbeg > last_hist) || (histend > last_hist))
    {
      builtin_error ("history specification out of range");
      return (EXECUTION_FAILURE);
    }

  if (histend < histbeg)
    {
      i = histend;
      histend = histbeg;
      histbeg = i;

      reverse = 1;
    }

  if (listing)
    stream = stdout;
  else
    {
      numbering = 0;
      stream = sh_mktmpfp ("bash-fc", MT_USERANDOM|MT_USETMPDIR, &fn);
      if (stream == 0)
	{
	  builtin_error ("cannot open temp file %s", fn ? fn : "");
	  FREE (fn);
	  return (EXECUTION_FAILURE);
	}
    }

  for (i = reverse ? histend : histbeg; reverse ? i >= histbeg : i <= histend; reverse ? i-- : i++)
    {
      QUIT;
      if (numbering)
	fprintf (stream, "%d", i + history_base);
      if (listing)
	fprintf (stream, "\t%c", histdata (i) ? '*' : ' ');
      fprintf (stream, "%s\n", histline (i));
    }

  if (listing)
    return (EXECUTION_SUCCESS);

  fclose (stream);

  /* Now edit the file of commands. */
  if (ename)
    {
      command = (char *)xmalloc (strlen (ename) + strlen (fn) + 2);
      sprintf (command, "%s %s", ename, fn);
    }
  else
    {
      command = (char *)xmalloc (3 + strlen (FC_EDIT_COMMAND) + strlen (fn));
      sprintf (command, "%s %s", FC_EDIT_COMMAND, fn);
    }
  retval = parse_and_execute (command, "fc", SEVAL_NOHIST);
  if (retval != EXECUTION_SUCCESS)
    {
      unlink (fn);
      free (fn);
      return (EXECUTION_FAILURE);
    }

  /* Make sure parse_and_execute doesn't turn this off, even though a
     call to parse_and_execute farther up the function call stack (e.g.,
     if this is called by vi_edit_and_execute_command) may have already
     called bash_history_disable. */
  remember_on_history = 1;

  /* Turn on the `v' flag while fc_execute_file runs so the commands
     will be echoed as they are read by the parser. */
  begin_unwind_frame ("fc builtin");
  add_unwind_protect ((Function *)xfree, fn);
  add_unwind_protect (unlink, fn);
  unwind_protect_int (echo_input_at_read);
  echo_input_at_read = 1;
    
  retval = fc_execute_file (fn);

  run_unwind_frame ("fc builtin");

  return (retval);
}

/* Return 1 if LIST->word->word is a legal number for fc's use. */
static int
fc_number (list)
     WORD_LIST *list;
{
  char *s;

  if (list == 0)
    return 0;
  s = list->word->word;
  if (*s == '-')
    s++;
  return (legal_number (s, (long *)NULL));
}

/* Return an absolute index into HLIST which corresponds to COMMAND.  If
   COMMAND is a number, then it was specified in relative terms.  If it
   is a string, then it is the start of a command line present in HLIST. */
static int
fc_gethnum (command, hlist)
     char *command;
     HIST_ENTRY **hlist;
{
  int sign = 1, n, clen;
  register int i, j;
  register char *s;

  /* Count history elements. */
  for (i = 0; hlist[i]; i++);

  /* With the Bash implementation of history, the current command line
     ("fc blah..." and so on) is already part of the history list by
     the time we get to this point.  This just skips over that command
     and makes the last command that this deals with be the last command
     the user entered before the fc.  We need to check whether the
     line was actually added (HISTIGNORE may have caused it to not be),
     so we check hist_last_line_added. */
  i -= 1 + hist_last_line_added;

  /* No specification defaults to most recent command. */
  if (command == NULL)
    return (i);

  /* Otherwise, there is a specification.  It can be a number relative to
     the current position, or an absolute history number. */
  s = command;

  /* Handle possible leading minus sign. */
  if (s && (*s == '-'))
    {
      sign = -1;
      s++;
    }

  if (s && DIGIT(*s))
    {
      n = atoi (s);
      n *= sign;

      /* Anything specified greater than the last history element that we
	 deal with is an error. */
      if (n > i + history_base)
	return (-1);

      /* If the value is negative or zero, then it is an offset from
	 the current history item. */
      if (n < 0)
	return (i + n + 1);
      else if (n == 0)
	return (i);
      else
	return (n - history_base);
    }

  clen = strlen (command);
  for (j = i; j >= 0; j--)
    {
      if (STREQN (command, histline (j), clen))
	return (j);
    }
  return (-1);
}

/* Locate the most recent history line which begins with
   COMMAND in HLIST, and return a malloc()'ed copy of it. */
static char *
fc_gethist (command, hlist)
     char *command;
     HIST_ENTRY **hlist;
{
  int i;

  if (!hlist)
    return ((char *)NULL);

  i = fc_gethnum (command, hlist);

  if (i >= 0)
    return (savestring (histline (i)));
  else
    return ((char *)NULL);
}

#ifdef INCLUDE_UNUSED
/* Read the edited history lines from STREAM and return them
   one at a time.  This can read unlimited length lines.  The
   caller should free the storage. */
static char *
fc_readline (stream)
     FILE *stream;
{
  register int c;
  int line_len = 0, lindex = 0;
  char *line = (char *)NULL;

  while ((c = getc (stream)) != EOF)
    {
      if ((lindex + 2) >= line_len)
	line = (char *)xrealloc (line, (line_len += 128));

      if (c == '\n')
	{
	  line[lindex++] = '\n';
	  line[lindex++] = '\0';
	  return (line);
	}
      else
	line[lindex++] = c;
    }

  if (!lindex)
    {
      if (line)
	free (line);

      return ((char *)NULL);
    }

  if (lindex + 2 >= line_len)
    line = (char *)xrealloc (line, lindex + 3);

  line[lindex++] = '\n';	    /* Finish with newline if none in file */
  line[lindex++] = '\0';
  return (line);
}
#endif

/* Perform the SUBS on COMMAND.
   SUBS is a list of substitutions, and COMMAND is a simple string.
   Return a pointer to a malloc'ed string which contains the substituted
   command. */
static char *
fc_dosubs (command, subs)
     char *command;
     REPL *subs;
{
  register char *new, *t;
  register REPL *r;

  for (new = savestring (command), r = subs; r; r = r->next)
    {
      t = strsub (new, r->pat, r->rep, 1);
      free (new);
      new = t;
    }
  return (new);
}

/* Use `command' to replace the last entry in the history list, which,
   by this time, is `fc blah...'.  The intent is that the new command
   become the history entry, and that `fc' should never appear in the
   history list.  This way you can do `r' to your heart's content. */
static void
fc_replhist (command)
     char *command;
{
  register int i;
  HIST_ENTRY **hlist, *histent, *discard;
  int n;

  if (command == 0 || *command == '\0')
    return;

  hlist = history_list ();

  if (hlist == NULL)
    return;

  for (i = 0; hlist[i]; i++);
  i--;

  /* History_get () takes a parameter that should be
     offset by history_base. */

  histent = history_get (history_base + i);	/* Don't free this */
  if (histent == NULL)
    return;

  n = strlen (command);

  if (command[n - 1] == '\n')
    command[n - 1] = '\0';

  if (command && *command)
    {
      discard = remove_history (i);
      if (discard)
	{
	  FREE (discard->line);
	  free ((char *) discard);
	}
      maybe_add_history (command);	/* Obeys HISTCONTROL setting. */
    }
}

#ifdef INCLUDE_UNUSED
/* Add LINE to the history, after removing a single trailing newline. */
static void
fc_addhist (line)
     char *line;
{
  register int n;

  n = strlen (line);

  if (line[n - 1] == '\n')
    line[n - 1] = '\0';

  if (line && *line)
    maybe_add_history (line);
}
#endif

#endif /* HISTORY */