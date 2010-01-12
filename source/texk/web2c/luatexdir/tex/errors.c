/* errors.c
   
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

#include "ptexlib.h"



static const char _svn_version[] =
    "$Id$"
    "$URL$";

#define new_line_char int_par(new_line_char_code)

/*
When something anomalous is detected, \TeX\ typically does something like this:
$$\vbox{\halign{#\hfil\cr
|print_err("Something anomalous has been detected");|\cr
|help3("This is the first line of my offer to help.")|\cr
|("This is the second line. I'm trying to")|\cr
|("explain the best way for you to proceed.");|\cr
|error;|\cr}}$$
A two-line help message would be given using |help2|, etc.; these informal
helps should use simple vocabulary that complements the words used in the
official error message that was printed. (Outside the U.S.A., the help
messages should preferably be translated into the local vernacular. Each
line of help is at most 60 characters long, in the present implementation,
so that |max_print_line| will not be exceeded.)

The |print_err| procedure supplies a `\.!' before the official message,
and makes sure that the terminal is awake if a stop is going to occur.
The |error| procedure supplies a `\..' after the official message, then it
shows the location of the error; and if |interaction=error_stop_mode|,
it also enters into a dialog with the user, during which time the help
message may be printed.
@^system dependencies@>
*/

int interaction;                /* current level of interaction */
int interactionoption;          /* set from command line */
char *last_error;

void print_err(char *s)
{
    if (interaction == error_stop_mode) {
        wake_up_terminal();
    }
    if (filelineerrorstylep)
        print_file_line();
    else
        tprint_nl("! ");
    tprint(s);
    last_error = s;
}

/*
\TeX\ is careful not to call |error| when the print |selector| setting
might be unusual. The only possible values of |selector| at the time of
error messages are

\yskip\hang|no_print| (when |interaction=batch_mode|
  and |log_file| not yet open);

\hang|term_only| (when |interaction>batch_mode| and |log_file| not yet open);

\hang|log_only| (when |interaction=batch_mode| and |log_file| is open);

\hang|term_and_log| (when |interaction>batch_mode| and |log_file| is open).
*/


void fixup_selector(boolean log_opened)
{
    if (interaction == batch_mode)
        selector = no_print;
    else
        selector = term_only;
    if (log_opened)
        selector = selector + 2;
}

/*
A global variable |deletions_allowed| is set |false| if the |get_next|
routine is active when |error| is called; this ensures that |get_next|
and related routines like |get_token| will never be called recursively.
A similar interlock is provided by |set_box_allowed|.
@^recursion@>

The global variable |history| records the worst level of error that
has been detected. It has four possible values: |spotless|, |warning_issued|,
|error_message_issued|, and |fatal_error_stop|.

Another global variable, |error_count|, is increased by one when an
|error| occurs without an interactive dialog, and it is reset to zero at
the end of every paragraph.  If |error_count| reaches 100, \TeX\ decides
that there is no point in continuing further.
*/

boolean deletions_allowed;      /* is it safe for |error| to call |get_token|? */
boolean set_box_allowed;        /* is it safe to do a \.{\\setbox} assignment? */
int history;                    /* has the source input been clean so far? */
int error_count;                /* the number of scrolled errors since the last paragraph ended */
int interrupt;                  /* should \TeX\ pause for instructions? */
boolean OK_to_interrupt;        /* should interrupts be observed? */


/*
The value of |history| is initially |fatal_error_stop|, but it will
be changed to |spotless| if \TeX\ survives the initialization process.
*/

void initialize_errors(void)
{
    if (interactionoption == unspecified_mode)
        interaction = error_stop_mode;
    else
        interaction = interactionoption;
    deletions_allowed = true;
    set_box_allowed = true;
    OK_to_interrupt = true;
    /* |history| is initialized elsewhere */
}

/*
It is possible for |error| to be called recursively if some error arises
when |get_token| is being used to delete a token, and/or if some fatal error
occurs while \TeX\ is trying to fix a non-fatal one. But such recursion
@^recursion@>
is never more than two levels deep.
*/

/*
Individual lines of help are recorded in the array |help_line|. 
*/

char *help_line[7];             /* helps for the next |error| */
boolean use_err_help;           /* should the |err_help| list be shown? */

/*
The |jump_out| procedure just cuts across all active procedure levels and
exits the program. It is used when there is no recovery from a particular error.
*/

void do_final_end(void)
{
    update_terminal();
    ready_already = 0;
    if ((history != spotless) && (history != warning_issued))
        uexit(1);
    else
        uexit(0);
}

void jump_out(void)
{
    close_files_and_terminate();
    do_final_end();
}

void error(void)
{                               /* completes the job of error reporting */
    ASCII_code c;               /* what the user types */
    int callback_id;
    int s1, s2, s3, s4;         /* used to save global variables when deleting tokens */
    boolean t;
    int i;
    if (history < error_message_issued)
        history = error_message_issued;
    print_char('.');
    callback_id = callback_defined(show_error_hook_callback);
    if (callback_id > 0)
        t = run_callback(callback_id, "->");
    show_context();
    if (haltonerrorp) {
        history = fatal_error_stop;
        jump_out();
    }
    if (interaction == error_stop_mode) {
        /* Get user's advice and |return| */
        while (1) {
          CONTINUE:
            clear_for_error_prompt();
            prompt_input("? ");
            if (last == first)
                return;
            c = buffer[first];
            if (c >= 'a')
                c = c + 'A' - 'a';      /* convert to uppercase */
            /* Interpret code |c| and |return| if done */

            /* It is desirable to provide an `\.E' option here that gives the user
               an easy way to return from \TeX\ to the system editor, with the offending
               line ready to be edited. But such an extension requires some system
               wizardry, so the present implementation simply types out the name of the
               file that should be  edited and the relevant line number.

               There is a secret `\.D' option available when the debugging routines haven't
               been commented~out. */

            switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (deletions_allowed) {
                    /* Delete \(c)|c-"0"| tokens and |goto continue| */
                    /* We allow deletion of up to 99 tokens at a time */
                    s1 = cur_tok;
                    s2 = cur_cmd;
                    s3 = cur_chr;
                    s4 = align_state;
                    align_state = 1000000;
                    OK_to_interrupt = false;
                    if ((last > first + 1) && (buffer[first + 1] >= '0')
                        && (buffer[first + 1] <= '9'))
                        c = c * 10 + buffer[first + 1] - '0' * 11;
                    else
                        c = c - '0';
                    while (c > 0) {
                        get_token();    /* one-level recursive call of |error| is possible */
                        decr(c);
                    }
                    cur_tok = s1;
                    cur_cmd = s2;
                    cur_chr = s3;
                    align_state = s4;
                    OK_to_interrupt = true;
                    help2("I have just deleted some text, as you asked.",
                          "You can now delete more, or insert, or whatever.");
                    show_context();
                    goto CONTINUE;
                }
                break;
#ifdef DEBUG
            case 'D':
                debug_help();
                goto CONTINUE;
                break;
#endif
            case 'E':
                if (base_ptr > 0) {
                    tprint_nl("You want to edit file ");
                    slow_print(input_stack[base_ptr].name_field);
                    tprint(" at line ");
                    print_int(line);
                    interaction = scroll_mode;
                    jump_out();
                }
                break;
            case 'H':
                /* Print the help information and |goto continue| */
                if (use_err_help) {
                    give_err_help();
                    use_err_help = false;
                } else {
                    if (help_line[0] == NULL) {
                        help2
                            ("Sorry, I don't know how to help in this situation.",
                             "Maybe you should try asking a human?");
                    }
                    i = 0;
                    while (help_line[i] != NULL)
                        tprint_nl(help_line[i++]);
                    help4("Sorry, I already gave what help I could...",
                          "Maybe you should try asking a human?",
                          "An error might have occurred before I noticed any problems.",
                          "``If all else fails, read the instructions.''");
                    goto CONTINUE;
                }
                break;
            case 'I':
                /* Introduce new material from the terminal and |return| */
                /* When the following code is executed, |buffer[(first+1)..(last-1)]| may
                   contain the material inserted by the user; otherwise another prompt will
                   be given. In order to understand this part of the program fully, you need
                   to be familiar with \TeX's input stacks. */

                begin_file_reading();   /* enter a new syntactic level for terminal input */
                /* now |state=mid_line|, so an initial blank space will count as a blank */
                if (last > first + 1) {
                    iloc = first + 1;
                    buffer[first] = ' ';
                } else {
                    prompt_input("insert>");
                    iloc = first;
                }
                first = last;
                ilimit = last - 1;      /* no |end_line_char| ends this line */
                return;
                break;
            case 'Q':
            case 'R':
            case 'S':
                /* Change the interaction level and |return| */
                /* Here the author of \TeX\ apologizes for making use of the numerical
                   relation between |"Q"|, |"R"|, |"S"|, and the desired interaction settings
                   |batch_mode|, |nonstop_mode|, |scroll_mode|. */
                error_count = 0;
                interaction = batch_mode + c - 'Q';
                tprint("OK, entering ");
                switch (c) {
                case 'Q':
                    tprint_esc("batchmode");
                    decr(selector);
                    break;
                case 'R':
                    tprint_esc("nonstopmode");
                    break;
                case 'S':
                    tprint_esc("scrollmode");
                    break;
                }
                tprint("...");
                print_ln();
                update_terminal();
                return;
                break;
            case 'X':
                interaction = scroll_mode;
                jump_out();
                break;
            default:
                break;
            }
            /* Print the menu of available options */
            tprint
                ("Type <return> to proceed, S to scroll future error messages,");
            tprint_nl("R to run without stopping, Q to run quietly,");
            tprint_nl("I to insert something, ");
            if (base_ptr > 0)
                tprint("E to edit your file,");
            if (deletions_allowed)
                tprint_nl
                    ("1 or ... or 9 to ignore the next 1 to 9 tokens of input,");
            tprint_nl("H for help, X to quit.");
        }

    }
    incr(error_count);
    if (error_count == 100) {
        tprint_nl("(That makes 100 errors; please try again.)");
        history = fatal_error_stop;
        jump_out();
    }
    /* Put help message on the transcript file */
    if (interaction > batch_mode)
        decr(selector);         /* avoid terminal output */
    if (use_err_help) {
        print_ln();
        give_err_help();
    } else {
        int i = 0;
        while (help_line[i] != NULL)
            tprint_nl(help_line[i++]);
    }
    print_ln();
    if (interaction > batch_mode)
        incr(selector);         /* re-enable terminal output */
    print_ln();
}


/*
A dozen or so error messages end with a parenthesized integer, so we
save a teeny bit of program space by declaring the following procedure:
*/

void int_error(int n)
{
    tprint(" (");
    print_int(n);
    print_char(')');
    error();
}

/*
In anomalous cases, the print selector might be in an unknown state;
the following subroutine is called to fix things just enough to keep
running a bit longer.
*/

void normalize_selector(void)
{
    if (log_opened)
        selector = term_and_log;
    else
        selector = term_only;
    if (job_name == 0)
        open_log_file();
    if (interaction == batch_mode)
        decr(selector);
}

/* The following procedure prints \TeX's last words before dying */

void succumb(void)
{
    if (interaction == error_stop_mode)
        interaction = scroll_mode;      /* no more interaction */
    if (log_opened)
        error();
#ifdef DEBUG
    if (interaction > batch_mode)
        debug_help();
#endif
    history = fatal_error_stop;
    jump_out();                 /* irrecoverable error */
}

void fatal_error(char *s)
{                               /* prints |s|, and that's it */
    normalize_selector();
    print_err("Emergency stop");
    help1(s);
    succumb();
}

void lua_norm_error(char *s)
{                               /* lua found a problem */
    int saved_new_line_char;
    saved_new_line_char = new_line_char;
    new_line_char = 10;
    print_err("LuaTeX error ");
    tprint(s);
    help2("The lua interpreter ran into a problem, so the",
          "remainder of this lua chunk will be ignored.");
    error();
    new_line_char = saved_new_line_char;
}

void lua_fatal_error(char *s)
{                               /* lua found a problem */
    new_line_char = 10;
    normalize_selector();
    print_err("LuaTeX fatal error ");
    tprint(s);
    succumb();
}

/* Here is the most dreaded error message */

void overflow(char *s, int n)
{                               /* stop due to finiteness */
    normalize_selector();
    print_err("TeX capacity exceeded, sorry [");
    tprint(s);
    print_char('=');
    print_int(n);
    print_char(']');
    help2("If you really absolutely need more capacity,",
          "you can ask a wizard to enlarge me.");
    succumb();
}

/*
The program might sometime run completely amok, at which point there is
no choice but to stop. If no previous error has been detected, that's bad
news; a message is printed that is really intended for the \TeX\
maintenance person instead of the user (unless the user has been
particularly diabolical).  The index entries for `this can't happen' may
help to pinpoint the problem.
@^dry rot@>
*/

void confusion(char *s)
{                               /* consistency check violated; |s| tells where */
    normalize_selector();
    if (history < error_message_issued) {
        print_err("This can't happen (");
        tprint(s);
        print_char(')');
        help1("I'm broken. Please show this to someone who can fix can fix");
    } else {
        print_err("I can't go on meeting you like this");
        help2("One of your faux pas seems to have wounded me deeply...",
              "in fact, I'm barely conscious. Please fix it and try again.");
    }
    succumb();
}

/*
Users occasionally want to interrupt \TeX\ while it's running.
If the \PASCAL\ runtime system allows this, one can implement
a routine that sets the global variable |interrupt| to some nonzero value
when such an interrupt is signalled. Otherwise there is probably at least
a way to make |interrupt| nonzero using the \PASCAL\ debugger.
@^system dependencies@>
@^debugging@>
*/

void check_interrupt(void)
{
    if (interrupt != 0)
        pause_for_instructions();
}


/*
When an interrupt has been detected, the program goes into its
highest interaction level and lets the user have nearly the full flexibility of
the |error| routine.  \TeX\ checks for interrupts only at times when it is
safe to do this.
*/

void pause_for_instructions(void)
{
    if (OK_to_interrupt) {
        interaction = error_stop_mode;
        if ((selector == log_only) || (selector == no_print))
            incr(selector);
        print_err("Interruption");
        help3("You rang?",
              "Try to insert some instructions for me (e.g.,`I\\showlists'),",
              "unless you just want to quit by typing `X'.");
        deletions_allowed = false;
        error();
        deletions_allowed = true;
        interrupt = 0;
    }
}



void tex_error(char *msg, char **hlp)
{
    print_err(msg);
    if (hlp != NULL) {
        int i;
        for (i = 0; (hlp[i] != NULL && i <= 5); i++) {
            help_line[i] = hlp[i];
        }
        help_line[i] = NULL;
    } else {
        help_line[0] = NULL;
    }
    error();
}

/*
The |back_error| routine is used when we want to replace an offending token
just before issuing an error message. This routine, like |back_input|,
requires that |cur_tok| has been set. We disable interrupts during the
call of |back_input| so that the help message won't be lost.
*/

void back_error(void)
{                               /* back up one token and call |error| */
    OK_to_interrupt = false;
    back_input();
    OK_to_interrupt = true;
    error();
}

void ins_error(void)
{                               /* back up one inserted token and call |error| */
    OK_to_interrupt = false;
    back_input();
    token_type = inserted;
    OK_to_interrupt = true;
    error();
}


/*
When \TeX\ wants to typeset a character that doesn't exist, the
character node is not created; thus the output routine can assume
that characters exist when it sees them. The following procedure
prints a warning message unless the user has suppressed it.
*/

void char_warning(internal_font_number f, int c)
{
    int old_setting;            /* saved value of |tracing_online| */
    int k;                      /* index to current digit; we assume that $0\L n<16^{22}$ */
    if (int_par(tracing_lost_chars_code) > 0) {
        old_setting = int_par(tracing_online_code);
        if (int_par(tracing_lost_chars_code) > 1)
            int_par(tracing_online_code) = 1;
        begin_diagnostic();
        tprint_nl("Missing character: There is no ");
        print(c);
        tprint(" (U+");
        k = 0;
        if (c < 16)
            print_char('0');
        if (c < 256)
            print_char('0');
        if (c < 4096)
            print_char('0');
        do {
            dig[k] = c % 16;
            c = c / 16;
            incr(k);
        } while (c != 0);
        print_the_digs((eight_bits)k);
        tprint(") in font ");
        print_font_name(f);
        print_char('!');
        end_diagnostic(false);
        int_par(tracing_online_code) = old_setting;
    }
}
