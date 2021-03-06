/* bashgetopt.h -- extern declarations for stuff defined in bashgetopt.c. */

/* Copyright (C) 1993 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

/* See getopt.h for the explanation of these variables. */

#if !defined (__BASH_GETOPT_H)
#  define __BASH_GETOPT_H

#include <stdc.h>

extern char *list_optarg;

extern int list_optopt;

extern WORD_LIST *lcurrent;
extern WORD_LIST *loptend;

extern int internal_getopt __P((WORD_LIST *, char *));
extern void reset_internal_getopt __P((void));
extern void report_bad_option __P((void));

#endif /* !__BASH_GETOPT_H */
