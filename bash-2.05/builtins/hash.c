/* hash.c, created from hash.def. */
#line 23 "d:/jicamasdk/progs/bash-2.05/builtins/hash.def"

#line 36 "d:/jicamasdk/progs/bash-2.05/builtins/hash.def"

#include <config.h>

#include <stdio.h>

#include "../bashtypes.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <errno.h>

#include "../bashansi.h"

#include "../shell.h"
#include "../builtins.h"
#include "../flags.h"
#include "../findcmd.h"
#include "../hashcmd.h"
#include "common.h"
#include "bashgetopt.h"

extern int dot_found_in_search;
extern char *this_command_name;

static int add_hashed_command __P((char *, int));
static int print_hashed_commands __P((void));
static int list_hashed_filename_targets __P((WORD_LIST *));

/* Print statistics on the current state of hashed commands.  If LIST is
   not empty, then rehash (or hash in the first place) the specified
   commands. */
int
hash_builtin (list)
     WORD_LIST *list;
{
  int expunge_hash_table, list_targets, opt;
  char *w, *pathname;

  if (hashing_enabled == 0)
    {
      builtin_error ("hashing disabled");
      return (EXECUTION_FAILURE);
    }

  expunge_hash_table = list_targets = 0;
  pathname = (char *)NULL;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "rp:t")) != -1)
    {
      switch (opt)
	{
	case 'r':
	  expunge_hash_table = 1;
	  break;
	case 'p':
	  pathname = list_optarg;
	  break;
	case 't':
	  list_targets = 1;
	  break;
	default:
	  builtin_usage ();
	  return (EX_USAGE);
	}
    }
  list = loptend;

  /* hash -t requires at least one argument. */
  if (list == 0 && list_targets)
    {
      builtin_error("-t: argument required");
      return (EXECUTION_FAILURE);
    }

  /* We want hash -r to be silent, but hash -- to print hashing info.  That
     is the reason for the test of expunge_hash_table. */
  if (list == 0 && expunge_hash_table == 0)
    {
      if (print_hashed_commands () == 0)
	printf ("%s: hash table empty\n", this_command_name);

      return (EXECUTION_SUCCESS);
    }

  if (expunge_hash_table)
    flush_hashed_filenames ();

  /* If someone runs `hash -r -t xyz' he will be disappointed. */
  if (list_targets)
    {
      return (list_hashed_filename_targets (list));
    }
      
#if defined (RESTRICTED_SHELL)
  if (restricted && pathname && strchr (pathname, '/'))
    {
      builtin_error ("%s: restricted", pathname);
      return (EXECUTION_FAILURE);
    }
#endif

  for (opt = EXECUTION_SUCCESS; list; list = list->next)
    {
      /* Add or rehash the specified commands. */
      w = list->word->word;
      if (pathname)
	{
	  if (is_directory (pathname))
	    {
#ifdef EISDIR
	      builtin_error ("%s: %s", pathname, strerror (EISDIR));
#else
	      builtin_error ("%s: is a directory", pathname);
#endif
	      opt = EXECUTION_FAILURE;
	    }
	  else
	    remember_filename (w, pathname, 0, 0);
	}
      else if (absolute_program (w))
	continue;
      else if (add_hashed_command (w, 0))
	opt = EXECUTION_FAILURE;
    }

  fflush (stdout);
  return (opt);
}

static int
add_hashed_command (w, quiet)
     char *w;
     int quiet;
{
  int rv;
  char *full_path;

  rv = 0;
  if (find_function (w) == 0 && find_shell_builtin (w) == 0)
    {
      full_path = find_user_command (w);
      if (full_path && executable_file (full_path))
	remember_filename (w, full_path, dot_found_in_search, 0);
      else
	{
	  if (quiet == 0)
	    builtin_error ("%s: not found", w);
	  rv++;
	}
      if (full_path)
	free (full_path);
    }
  return (rv);
}

/* Print information about current hashed info. */
static int
print_hashed_commands ()
{
  BUCKET_CONTENTS *item_list;
  int bucket, any_printed;

  if (hashed_filenames == 0)
    return (0);

  for (bucket = any_printed = 0; bucket < hashed_filenames->nbuckets; bucket++)
    {
      item_list = get_hash_bucket (bucket, hashed_filenames);
      if (item_list == 0)
	continue;

      if (any_printed == 0)
	{
	  printf ("hits\tcommand\n");
	  any_printed++;
	}

      for ( ; item_list; item_list = item_list->next)
	printf ("%4d\t%s\n", item_list->times_found, pathdata(item_list)->path);

    }
  return (any_printed);
}

static int
list_hashed_filename_targets (list)
     WORD_LIST *list;
{
  int all_found, multiple;
  char *target;
  WORD_LIST *l;

  all_found = 1;
  multiple = list->next != 0;

  for (l = list; l; l = l->next)
    {
      target = find_hashed_filename (l->word->word);
      if (target == 0)
	{
	  all_found = 0;
	  builtin_error ("%s: not found", l->word->word);
	  continue;
	}
      if (multiple)
        printf ("%s\t", l->word->word);
      printf ("%s\n", target);
    }

  return (all_found ? EXECUTION_SUCCESS : EXECUTION_FAILURE);
}
