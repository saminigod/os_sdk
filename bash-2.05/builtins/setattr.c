/* setattr.c, created from setattr.def. */
#line 23 "d:/jicamasdk/progs/bash-2.05/builtins/setattr.def"

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <stdio.h>
#include "../bashansi.h"

#include "../shell.h"
#include "common.h"
#include "bashgetopt.h"

extern int posixly_correct;
extern int array_needs_making;
extern char *this_command_name;
extern sh_builtin_func_t *this_shell_builtin;

#ifdef ARRAY_VARS
extern int declare_builtin ();
#endif

#define READONLY_OR_EXPORT \
  (this_shell_builtin == readonly_builtin || this_shell_builtin == export_builtin)

#line 63 "d:/jicamasdk/progs/bash-2.05/builtins/setattr.def"

/* For each variable name in LIST, make that variable appear in the
   environment passed to simple commands.  If there is no LIST, then
   print all such variables.  An argument of `-n' says to remove the
   exported attribute from variables named in LIST.  An argument of
  -f indicates that the names present in LIST refer to functions. */
int
export_builtin (list)
     register WORD_LIST *list;
{
  return (set_or_show_attributes (list, att_exported, 0));
}

#line 88 "d:/jicamasdk/progs/bash-2.05/builtins/setattr.def"

/* For each variable name in LIST, make that variable readonly.  Given an
   empty LIST, print out all existing readonly variables. */
int
readonly_builtin (list)
     register WORD_LIST *list;
{
  return (set_or_show_attributes (list, att_readonly, 0));
}

/* For each variable name in LIST, make that variable have the specified
   ATTRIBUTE.  An arg of `-n' says to remove the attribute from the the
   remaining names in LIST. */
int
set_or_show_attributes (list, attribute, nodefs)
     register WORD_LIST *list;
     int attribute, nodefs;
{
  register SHELL_VAR *var;
  int assign, undo, functions_only, arrays_only, any_failed, assign_error, opt;
  char *name;
#if defined (ARRAY_VARS)
  WORD_LIST *nlist, *tlist;
  WORD_DESC *w;
#endif

  undo = functions_only = arrays_only = any_failed = assign_error = 0;
  /* Read arguments from the front of the list. */
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "anfp")) != -1)
    {
      switch (opt)
	{
	  case 'n':
	    undo = 1;
	    break;
	  case 'f':
	    functions_only = 1;
	    break;
#if defined (ARRAY_VARS)
	  case 'a':
	     arrays_only = 1;
	     break;
#endif
	  case 'p':
	    break;
	  default:
	    builtin_usage ();
	    return (EX_USAGE);
	}
    }
  list = loptend;

  if (list)
    {
      if (attribute & att_exported)
	array_needs_making = 1;

      /* Cannot undo readonly status, silently disallowed. */
      if (undo && (attribute & att_readonly))
	attribute &= ~att_readonly;

      while (list)
	{
	  name = list->word->word;

	  if (functions_only)		/* xxx -f name */
	    {
	      var = find_function (name);
	      if (var == 0)
		{
		  builtin_error ("%s: not a function", name);
		  any_failed++;
		}
	      else
		SETVARATTR (var, attribute, undo);

	      list = list->next;
	      continue;
	    }

	  /* xxx [-np] name[=value] */
	  assign = assignment (name);

	  if (assign)
	    name[assign] = '\0';

	  if (legal_identifier (name) == 0)
	    {
	      builtin_error ("`%s': not a valid identifier", name);
	      if (assign)
		assign_error++;
	      else
		any_failed++;
	      list = list->next;
	      continue;
	    }

	  if (assign)	/* xxx [-np] name=value */
	    {
	      name[assign] = '=';
#if defined (ARRAY_VARS)
	      /* Let's try something here.  Turn readonly -a xxx=yyy into
		 declare -ra xxx=yyy and see what that gets us. */
	      if (arrays_only)
		{
		  tlist = list->next;
		  list->next = (WORD_LIST *)NULL;
		  w = make_word ("-ra");
		  nlist = make_word_list (w, list);
		  opt = declare_builtin (nlist);
		  if (opt != EXECUTION_SUCCESS)
		    assign_error++;
		  list->next = tlist;
		  dispose_word (w);
		  free (nlist);
		}
	      else
#endif
	      /* This word has already been expanded once with command
		 and parameter expansion.  Call do_assignment_no_expand (),
		 which does not do command or parameter substitution.  If
		 the assignment is not performed correctly, flag an error. */
	      if (do_assignment_no_expand (name) == 0)
		assign_error++;
	      name[assign] = '\0';
	    }

	  set_var_attribute (name, attribute, undo);
	  list = list->next;
	}
    }
  else
    {
      SHELL_VAR **variable_list;
      register int i;

      if ((attribute & att_function) || functions_only)
	{
	  variable_list = all_shell_functions ();
	  if (attribute != att_function)
	    attribute &= ~att_function;	/* so declare -xf works, for example */
	}
      else
	variable_list = all_shell_variables ();

#if defined (ARRAY_VARS)
      if (attribute & att_array)
	{
	  arrays_only++;
	  if (attribute != att_array)
	    attribute &= ~att_array;
	}
#endif

      if (variable_list)
	{
	  for (i = 0; var = variable_list[i]; i++)
	    {
#if defined (ARRAY_VARS)
	      if (arrays_only && array_p (var) == 0)
		continue;
#endif
#if 0
	      if ((var->attributes & attribute) && invisible_p (var) == 0)
#else
	      if ((var->attributes & attribute))
#endif
		show_var_attributes (var, READONLY_OR_EXPORT, nodefs);
	    }
	  free (variable_list);
	}
    }

  return (assign_error ? EX_BADASSIGN
		       : ((any_failed == 0) ? EXECUTION_SUCCESS
  					    : EXECUTION_FAILURE));
}

/* Show the attributes for shell variable VAR.  If NODEFS is non-zero,
   don't show function definitions along with the name.  If PATTR is
   non-zero, it indicates we're being called from `export' or `readonly'.
   In POSIX mode, this prints the name of the calling builtin (`export'
   or `readonly') instead of `declare', and doesn't print function defs
   when called by `export' or `readonly'. */
int
show_var_attributes (var, pattr, nodefs)
     SHELL_VAR *var;
     int pattr, nodefs;
{
  char flags[6], *x;
  int i;

  i = 0;

  /* pattr == 0 means we are called from `declare'. */
  if (pattr == 0 || posixly_correct == 0)
    {
#if defined (ARRAY_VARS)
      if (array_p (var))
	flags[i++] = 'a';
#endif

      if (function_p (var))
	flags[i++] = 'f';

      if (integer_p (var))
	flags[i++] = 'i';

      if (readonly_p (var))
	flags[i++] = 'r';

      if (exported_p (var))
	flags[i++] = 'x';
    }
  else
    {
#if defined (ARRAY_VARS)
      if (array_p (var))
	flags[i++] = 'a';
#endif

      if (function_p (var))
	flags[i++] = 'f';
    }

  flags[i] = '\0';

  if (pattr == 0 || posixly_correct == 0)
    printf ("declare -%s ", i ? flags : "-");
  else if (i)
    printf ("%s -%s ", this_command_name, flags);
  else
    printf ("%s ", this_command_name);

#if defined (ARRAY_VARS)
 if (array_p (var))
    print_array_assignment (var, 1);
  else
#endif
  /* force `readline' and `export' to not print out function definitions
     when in POSIX mode. */
  if (nodefs || (function_p (var) && pattr != 0 && posixly_correct))
    printf ("%s\n", var->name);
  else if (function_p (var))
    printf ("%s\n", named_function_string (var->name, function_cell (var), 1));
  else if (invisible_p (var))
    printf ("%s\n", var->name);
  else
    {
      x = sh_double_quote (value_cell (var) ? value_cell (var) : "");
      printf ("%s=%s\n", var->name, x);
      free (x);
    }
  return (0);
}

int
show_name_attributes (name, nodefs)
     char *name;
     int nodefs;
{
  SHELL_VAR *var;

  var = find_tempenv_variable (name);
  if (var == 0)
    var = find_variable (name);

  if (var && invisible_p (var) == 0)
    {
      show_var_attributes (var, READONLY_OR_EXPORT, nodefs);
      if (tempvar_p (var))
	dispose_variable (var);
      return (0);
    }
  else
    return (1);
}

void
set_var_attribute (name, attribute, undo)
     char *name;
     int attribute, undo;
{
  SHELL_VAR *var, *tv;

  if (undo)
    var = find_variable (name);
  else
    {
      if (tv = find_tempenv_variable (name))
	{
	  var = bind_variable (tv->name, tv->value ? tv->value : "");
	  dispose_variable (tv);
	}
      else
	var = find_variable (name);

      if (var == 0)
	{
	  var = bind_variable (name, (char *)NULL);
	  VSETATTR (var, att_invisible);
	}
    }

  if (var)
    SETVARATTR (var, attribute, undo);

  if (var && (exported_p (var) || (attribute & att_exported)))
    array_needs_making++;	/* XXX */
}
