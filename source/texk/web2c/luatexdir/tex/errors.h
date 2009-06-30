/* errors.h

   Copyright 2009 Taco Hoekwater <taco@luatex.org>

   This file is part of LuaTeX.

   LuaTeX is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   LuaTeX is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with LuaTeX; if not, see <http://www.gnu.org/licenses/>. */

/* $Id$ */

#ifndef ERRORS_H
#  define ERRORS_H

/*
The global variable |interaction| has four settings, representing increasing
amounts of user interaction:
*/

typedef enum {
    batch_mode = 0,             /* omits all stops and omits terminal output */
    nonstop_mode = 1,           /* omits all stops */
    scroll_mode = 2,            /* omits error stops */
    error_stop_mode = 3,        /* stops at every opportunity to interact */
    unspecified_mode = 4,       /* extra value for command-line switch */
} interaction_settings;

extern int interaction;         /* current level of interaction */
extern int interactionoption;   /* set from command line */
extern char *last_error;

extern void initialize_errors(void);
extern void print_err(char *s);

extern void fixup_selector(boolean log_opened);

extern boolean deletions_allowed;
extern boolean set_box_allowed;
extern int history;
extern int error_count;
extern integer interrupt;
extern boolean OK_to_interrupt;

typedef enum {
    spotless = 0,               /* |history| value when nothing has been amiss yet */
    warning_issued = 1,         /* |history| value when |begin_diagnostic| has been called */
    error_message_issued = 2,   /* |history| value when |error| has been called */
    fatal_error_stop = 3,       /* |history| value when termination was premature */
} error_states;

extern char *help_line[7];      /* helps for the next |error| */
extern boolean use_err_help;    /* should the |err_help| list be shown? */

/* these macros are just temporary, until everything is in C */

#  define hlp1(A)           help_line[0]=A
#  define hlp2(A,B)         help_line[1]=A; hlp1(B)
#  define hlp3(A,B,C)       help_line[2]=A; hlp2(B,C)
#  define hlp4(A,B,C,D)     help_line[3]=A; hlp3(B,C,D)
#  define hlp5(A,B,C,D,E)   help_line[4]=A; hlp4(B,C,D,E)
#  define hlp6(A,B,C,D,E,F) help_line[5]=A; hlp5(B,C,D,E,F)

#  define help0()                 help_line[0]=NULL     /* sometimes there might be no help */
#  define help1(A)           do { help_line[1]=NULL; hlp1(A);           } while (0)
#  define help2(A,B)         do { help_line[2]=NULL; hlp2(B,A);         } while (0)
#  define help3(A,B,C)       do { help_line[3]=NULL; hlp3(C,B,A);       } while (0)
#  define help4(A,B,C,D)     do { help_line[4]=NULL; hlp4(D,C,B,A);     } while (0)
#  define help5(A,B,C,D,E)   do { help_line[5]=NULL; hlp5(E,D,C,B,A);   } while (0)
#  define help6(A,B,C,D,E,F) do { help_line[6]=NULL; hlp6(F,E,D,C,B,A); } while (0)

extern void do_final_end(void);
extern void jump_out(void);
extern void error(void);
extern void int_error(integer n);
extern void normalize_selector(void);
extern void succumb(void);
extern void fatal_error(char *s);
extern void lua_norm_error(char *s);
extern void lua_fatal_error(char *s);
extern void overflow(char *s, integer n);
extern void confusion(char *s);
extern void check_interrupt(void);
extern void pause_for_instructions(void);

extern void tex_error(char *msg, char **hlp);

extern void back_error (void);
extern void ins_error (void);

#endif
