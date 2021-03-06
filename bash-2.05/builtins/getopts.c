/* getopts.c, created from getopts.def. */
#line 23 "d:/jicamasdk/progs/bash-2.05/builtins/getopts.def"

#line 58 "d:/jicamasdk/progs/bash-2.05/builtins/getopts.def"

#include <config.h>

#include <stdio.h>

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include "../bashansi.h"

#include "../shell.h"
#include "common.h"
#include "bashgetopt.h"
#include "getopt.h"

#define G_EOF		-1
#define G_ILLEGAL_OPT	-2
#define G_ARG_MISSING	-3

extern char *this_command_name;

/* getopts_reset is magic code for when OPTIND is reset.  N is the
   value that has just been assigned to OPTIND. */
void
getopts_reset (newind)
     int newind;
{
  sh_optind = newind;
  sh_badopt = 0;
}

static int
getopts_bind_variable (name, value)
     char *name, *value;
{
  SHELL_VAR *v;

  if (legal_identifier (name))
    {
      v = bind_variable (name, value);
      return (v && (readonly_p (v) == 0)) ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
    }
  else
    {
      builtin_error ("`%s': not a valid identifier", name);
      return (EXECUTION_FAILURE);
    }
}

/* Error handling is now performed as specified by Posix.2, draft 11
   (identical to that of ksh-88).  The special handling is enabled if
   the first character of the option string is a colon; this handling
   disables diagnostic messages concerning missing option arguments
   and illegal option characters.  The handling is as follows.

   ILLEGAL OPTIONS:
        name -> "?"
        if (special_error) then
                OPTARG = option character found
                no error output
        else
                OPTARG unset
                diagnostic message
        fi
 
  MISSING OPTION ARGUMENT;
        if (special_error) then
                name -> ":"
                OPTARG = option character found
        else
                name -> "?"
                OPTARG unset
                diagnostic message
        fi
 */

static int
dogetopts (argc, argv)
     int argc;
     char **argv;
{
  int ret, special_error, old_opterr, i, n;
  char strval[2], numval[16];
  char *optstr;			/* list of options */
  char *name;			/* variable to get flag val */
  char *t;

  if (argc < 3)
    {
      builtin_usage ();
      return (EX_USAGE);
    }

  /* argv[0] is "getopts". */

  optstr = argv[1];
  name = argv[2];
  argc -= 2;
  argv += 2;

  special_error = optstr[0] == ':';

  if (special_error)
    {
      old_opterr = sh_opterr;
      optstr++;
      sh_opterr = 0;		/* suppress diagnostic messages */
    }

  if (argc > 1)
    {
      sh_getopt_restore_state (argv);
      t = argv[0];
      argv[0] = dollar_vars[0];
      ret = sh_getopt (argc, argv, optstr);
      argv[0] = t;
    }
  else if (rest_of_args == (WORD_LIST *)NULL)
    {
      for (i = 0; i < 10 && dollar_vars[i]; i++)
	;

      sh_getopt_restore_state (dollar_vars);
      ret = sh_getopt (i, dollar_vars, optstr);
    }
  else
    {
      register WORD_LIST *words;
      char **v;

      for (i = 0; i < 10 && dollar_vars[i]; i++)
	;
      for (words = rest_of_args; words; words = words->next, i++)
	;
      v = alloc_array (i + 1);
      for (i = 0; i < 10 && dollar_vars[i]; i++)
	v[i] = dollar_vars[i];
      for (words = rest_of_args; words; words = words->next, i++)
	v[i] = words->word->word;
      v[i] = (char *)NULL;
      sh_getopt_restore_state (v);
      ret = sh_getopt (i, v, optstr);
      free (v);
    }

  if (special_error)
    sh_opterr = old_opterr;

  /* Set the OPTIND variable in any case, to handle "--" skipping. */
  if (sh_optind < 10)
    {
      numval[14] = sh_optind + '0';
      numval[15] = '\0';
      i = 14;
    }
  else
    {
      numval[i = 15] = '\0';
      n = sh_optind;
      do
	{
	  numval[--i] = (n % 10) + '0';
	}
      while (n /= 10);
    }
  bind_variable ("OPTIND", numval + i);

  /* If an error occurred, decide which one it is and set the return
     code appropriately.  In all cases, the option character in error
     is in OPTOPT.  If an illegal option was encountered, OPTARG is
     NULL.  If a required option argument was missing, OPTARG points
     to a NULL string (that is, sh_optarg[0] == 0). */
  if (ret == '?')
    {
      if (sh_optarg == NULL)
	ret = G_ILLEGAL_OPT;
      else if (sh_optarg[0] == '\0')
	ret = G_ARG_MISSING;
    }
	    
  if (ret == G_EOF)
    {
      getopts_bind_variable (name, "?");
      return (EXECUTION_FAILURE);
    }

  if (ret == G_ILLEGAL_OPT)
    {
      /* Illegal option encountered. */
      ret = getopts_bind_variable (name, "?");

      if (special_error)
	{
	  strval[0] = (char)sh_optopt;
	  strval[1] = '\0';
	  bind_variable ("OPTARG", strval);
	}
      else
	makunbound ("OPTARG", shell_variables);

      return (ret);
    }

  if (ret == G_ARG_MISSING)
    {
      /* Required argument missing. */
      if (special_error)
	{
	  ret = getopts_bind_variable (name, ":");

	  strval[0] = (char)sh_optopt;
	  strval[1] = '\0';
	  bind_variable ("OPTARG", strval);
	}
      else
	{
	  ret = getopts_bind_variable (name, "?");
	  makunbound ("OPTARG", shell_variables);
	}
      return (ret);
    }			

  bind_variable ("OPTARG", sh_optarg);

  strval[0] = (char) ret;
  strval[1] = '\0';
  return (getopts_bind_variable (name, strval));
}

/* The getopts builtin.  Build an argv, and call dogetopts with it. */
int
getopts_builtin (list)
     WORD_LIST *list;
{
  char **av;
  int ac, ret;

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  reset_internal_getopt ();
  if (internal_getopt (list, "") != -1)
    {
      builtin_usage ();
      return (EX_USAGE);
    }
  list = loptend;

  av = make_builtin_argv (list, &ac);
  ret = dogetopts (ac, av);
  free ((char *)av);

  return (ret);
}
