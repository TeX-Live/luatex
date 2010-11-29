% texfileio.w
% 
% Copyright 2009-2010 Taco Hoekwater <taco@@luatex.org>

% This file is part of LuaTeX.

% LuaTeX is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free
% Software Foundation; either version 2 of the License, or (at your
% option) any later version.

% LuaTeX is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
% FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License for more details.

% You should have received a copy of the GNU General Public License along
% with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

@ @c
#include <string.h>
#include "ptexlib.h"
#include <kpathsea/absolute.h>

static const char _svn_version[] =
    "$Id$"
    "$URL$";

@ @c
#define end_line_char int_par(end_line_char_code)

@ The bane of portability is the fact that different operating systems treat
input and output quite differently, perhaps because computer scientists
have not given sufficient attention to this problem. People have felt somehow
that input and output are not part of ``real'' programming. Well, it is true
that some kinds of programming are more fun than others. With existing
input/output conventions being so diverse and so messy, the only sources of
joy in such parts of the code are the rare occasions when one can find a
way to make the program a little less bad than it might have been. We have
two choices, either to attack I/O now and get it over with, or to postpone
I/O until near the end. Neither prospect is very attractive, so let's
get it over with.

The basic operations we need to do are (1)~inputting and outputting of
text, to or from a file or the user's terminal; (2)~inputting and
outputting of eight-bit bytes, to or from a file; (3)~instructing the
operating system to initiate (``open'') or to terminate (``close'') input or
output from a specified file; (4)~testing whether the end of an input
file has been reached.

\TeX\ needs to deal with two kinds of files.
We shall use the term |alpha_file| for a file that contains textual data,
and the term |byte_file| for a file that contains eight-bit binary information.
These two types turn out to be the same on many computers, but
sometimes there is a significant distinction, so we shall be careful to
distinguish between them. Standard protocols for transferring
such files from computer to computer, via high-speed networks, are
now becoming available to more and more communities of users.

The program actually makes use also of a third kind of file, called a
|word_file|, when dumping and reloading base information for its own
initialization.  We shall define a word file later; but it will be possible
for us to specify simple operations on word files before they are defined.

@ We finally did away with |nameoffile| and |namelength|, but the variables
have to be kept otherwise there will be link errors from |openclose.c| in
the web2c library 

@c
char *nameoffile;
int namelength;


@ When input files are opened via a callback, they will also be read using
callbacks. for that purpose, the |open_read_file_callback| returns an
integer to uniquely identify a callback table. This id replaces the file
point |f| in this case, because the input does not have to be a file
in the traditional sense.

Signalling this fact is achieved by having two arrays of integers.

@c
int *input_file_callback_id;
int read_file_callback_id[17];

@ Handle -output-directory.

We assume that it is OK to look here first.  Possibly it
would be better to replace lookups in "." with lookups in the
|output_directory| followed by "." but to do this requires much more
invasive surgery in libkpathsea.  

@c
static char *find_in_output_directory(const char *s)
{
    if (output_directory && !kpse_absolute_p(s, false)) {
        FILE *f_ptr;
        char *ftemp = concat3(output_directory, DIR_SEP_STRING, s);
        f_ptr = fopen(ftemp, "rb");     /* this code is used for input files only */
        if (f_ptr) {
            fclose(f_ptr);
            return ftemp;
        } else {
            free(ftemp);

        }
    }
    return NULL;
}

@ find an \.{\\input} or \.{\\read} file. |n| differentiates between those case. 

@c
char *luatex_find_read_file(const char *s, int n, int callback_index)
{
    char *ftemp = NULL;
    int callback_id = callback_defined(callback_index);
    if (callback_id > 0) {
        (void) run_callback(callback_id, "dS->S", n, s, &ftemp);
    } else {
        /* use kpathsea here */
        ftemp = find_in_output_directory(s);
        if (!ftemp)
            ftemp = kpse_find_file(s, kpse_tex_format, 1);
    }
    if (ftemp) {
        if (fullnameoffile)
            free(fullnameoffile);
        fullnameoffile = xstrdup(ftemp);
    }
    return ftemp;
}

@ find other files types
@c
char *luatex_find_file(const char *s, int callback_index)
{
    char *ftemp = NULL;
    int callback_id = callback_defined(callback_index);
    if (callback_id > 0) {
        (void) run_callback(callback_id, "S->S", s, &ftemp);

    } else {
        /* use kpathsea here */
        switch (callback_index) {
        case find_enc_file_callback:
            ftemp = kpse_find_file(s, kpse_enc_format, 0);
            break;
        case find_sfd_file_callback:
            ftemp = kpse_find_file(s, kpse_sfd_format, 0);
            break;
        case find_map_file_callback:
            ftemp = kpse_find_file(s, kpse_fontmap_format, 0);
            break;
        case find_type1_file_callback:
            ftemp = kpse_find_file(s, kpse_type1_format, 0);
            break;
        case find_truetype_file_callback:
            ftemp = kpse_find_file(s, kpse_truetype_format, 0);
            break;
        case find_opentype_file_callback:
            ftemp = kpse_find_file(s, kpse_opentype_format, 0);
            if (ftemp == NULL)
                ftemp = kpse_find_file(s, kpse_truetype_format, 0);
            break;
        case find_data_file_callback:
            ftemp = find_in_output_directory(s);
            if (!ftemp)
                ftemp = kpse_find_file(s, kpse_tex_format, 0);
            break;
        case find_font_file_callback:
            ftemp = kpse_find_file(s, kpse_ofm_format, 0);
            if (ftemp == NULL)
                ftemp = kpse_find_file(s, kpse_tfm_format, 0);
            break;
        case find_vf_file_callback:
            ftemp = kpse_find_file(s, kpse_ovf_format, 0);
            if (ftemp == NULL)
                ftemp = kpse_find_file(s, kpse_vf_format, 0);
            break;
        default:
            printf
                ("luatex_find_file(): do not know how to handle file %s of type %d\n",
                 s, callback_index);
            break;
        }
    }
    return ftemp;
}


@  Open an input file F, using the kpathsea format FILEFMT and passing
   |FOPEN_MODE| to fopen.  The filename is in `fn'.  We return whether or 
   not the open succeeded.

@c
boolean
luatex_open_input(FILE ** f_ptr, const char *fn, int filefmt,
                  const_string fopen_mode, boolean must_exist)
{
    string fname = NULL;
    /* We havent found anything yet. */
    *f_ptr = NULL;
    if (fullnameoffile)
        free(fullnameoffile);
    fullnameoffile = NULL;
    fname = kpse_find_file(fn, (kpse_file_format_type) filefmt, must_exist);
    if (fname) {
        fullnameoffile = xstrdup(fname);
        /* If we found the file in the current directory, don't leave
           the `./' at the beginning of `fn', since it looks
           dumb when `tex foo' says `(./foo.tex ... )'.  On the other
           hand, if the user said `tex ./foo', and that's what we
           opened, then keep it -- the user specified it, so we
           shouldn't remove it.  */
        if (fname[0] == '.' && IS_DIR_SEP(fname[1])
            && (fn[0] != '.' || !IS_DIR_SEP(fn[1]))) {
            unsigned i = 0;
            while (fname[i + 2] != 0) {
                fname[i] = fname[i + 2];
                i++;
            }
            fname[i] = 0;
        }
        /* This fopen is not allowed to fail. */
        *f_ptr = xfopen(fname, fopen_mode);
    }
    if (*f_ptr) {
        recorder_record_input(fname);
    }
    return *f_ptr != NULL;
}

@ @c
boolean luatex_open_output(FILE ** f_ptr, const char *fn,
                           const_string fopen_mode)
{
    char *fname;
    boolean absolute = kpse_absolute_p(fn, false);

    /* If we have an explicit output directory, use it. */
    if (output_directory && !absolute) {
        fname = concat3(output_directory, DIR_SEP_STRING, fn);
    } else {
        fname = xstrdup(fn);
    }

    /* Is the filename openable as given?  */
    *f_ptr = fopen(fname, fopen_mode);

    if (!*f_ptr) {
        /* Can't open as given.  Try the envvar.  */
        string texmfoutput = kpse_var_value("TEXMFOUTPUT");

        if (texmfoutput && *texmfoutput && !absolute) {
            fname = concat3(texmfoutput, DIR_SEP_STRING, fn);
            *f_ptr = fopen(fname, fopen_mode);
        }
    }
    if (*f_ptr) {
        recorder_record_output(fname);
    }
    free(fname);
    return *f_ptr != NULL;
}


@ @c
boolean lua_a_open_in(alpha_file * f, char *fn, int n)
{
    int k;
    char *fnam;                 /* string returned by find callback */
    int callback_id;
    boolean ret = true;         /* return value */
    boolean file_ok = true;     /* the status so far  */
    if (n == 0) {
        input_file_callback_id[iindex] = 0;
    } else {
        read_file_callback_id[n] = 0;
    }
    fnam = luatex_find_read_file(fn, n, find_read_file_callback);
    if (!fnam)
        return false;
    callback_id = callback_defined(open_read_file_callback);
    if (callback_id > 0) {
        k = run_and_save_callback(callback_id, "S->", fnam);
        if (k > 0) {
            ret = true;
            if (n == 0)
                input_file_callback_id[iindex] = k;
            else
                read_file_callback_id[n] = k;
        } else {
            file_ok = false;    /* read failed */
        }
    } else {                    /* no read callback */
        if (openinnameok(fnam)) {
            ret =
                open_in_or_pipe(f, fnam, kpse_tex_format, FOPEN_RBIN_MODE,
                                (n == 0 ? true : false));
        } else {
            file_ok = false;    /* open failed */
        }
    }
    if (!file_ok) {
        ret = false;
    }
    return ret;
}

@ @c
boolean lua_a_open_out(alpha_file * f, char *fn, int n)
{
    boolean test;
    str_number fnam;
    int callback_id;
    boolean ret = false;
    callback_id = callback_defined(find_write_file_callback);
    if (callback_id > 0) {
        fnam = 0;
        test = run_callback(callback_id, "dS->s", n, fn, &fnam);
        if ((test) && (fnam != 0) && (str_length(fnam) > 0)) {
            ret = open_outfile(f, fn, FOPEN_W_MODE);
        }
    } else {
        if (openoutnameok(fn)) {
            ret = open_out_or_pipe(f, fn, FOPEN_W_MODE);
        }
    }
    return ret;
}

@ @c
boolean lua_b_open_out(alpha_file * f, char *fn)
{
    boolean test;
    str_number fnam;
    int callback_id;
    boolean ret = false;
    callback_id = callback_defined(find_output_file_callback);
    if (callback_id > 0) {
        fnam = 0;
        test = run_callback(callback_id, "S->s", fn, &fnam);
        if ((test) && (fnam != 0) && (str_length(fnam) > 0)) {
            ret = open_outfile(f, fn, FOPEN_WBIN_MODE);
        }
    } else {
        if (openoutnameok(fn)) {
            ret = luatex_open_output(f, fn, FOPEN_WBIN_MODE);
        }
    }
    return ret;
}

@ @c
void lua_a_close_in(alpha_file f, int n)
{                               /* close a text file */
    boolean ret;
    int callback_id;
    if (n == 0)
        callback_id = input_file_callback_id[iindex];
    else
        callback_id = read_file_callback_id[n];
    if (callback_id > 0) {
        ret = run_saved_callback(callback_id, "close", "->");
        destroy_saved_callback(callback_id);
        if (n == 0)
            input_file_callback_id[iindex] = 0;
        else
            read_file_callback_id[n] = 0;
    } else {
        close_file_or_pipe(f);
    }
}

@ @c
void lua_a_close_out(alpha_file f)
{                               /* close a text file */
    close_file_or_pipe(f);
}


@ Binary input and output are done with C's ordinary 
procedures, so we don't have to make any other special arrangements for
binary~I/O. Text output is also easy to do with standard routines.
The treatment of text input is more difficult, however, because
of the necessary translation to |ASCII_code| values.
\TeX's conventions should be efficient, and they should
blend nicely with the user's operating environment.

Input from text files is read one line at a time, using a routine called
|lua_input_ln|. This function is defined in terms of global variables called
|buffer|, |first|, and |last| that will be described in detail later; for
now, it suffices for us to know that |buffer| is an array of |ASCII_code|
values, and that |first| and |last| are indices into this array
representing the beginning and ending of a line of text.

@c
packed_ASCII_code *buffer;      /* lines of characters being read */
int first;                      /* the first unused position in |buffer| */
int last;                       /* end of the line just input to |buffer| */
int max_buf_stack;              /* largest index used in |buffer| */


@ The |lua_input_ln| function brings the next line of input from the specified
file into available positions of the buffer array and returns the value
|true|, unless the file has already been entirely read, in which case it
returns |false| and sets |last:=first|.  In general, the |ASCII_code|
numbers that represent the next line of the file are input into
|buffer[first]|, |buffer[first+1]|, \dots, |buffer[last-1]|; and the
global variable |last| is set equal to |first| plus the length of the
line. Trailing blanks are removed from the line; thus, either |last=first|
(in which case the line was entirely blank) or |buffer[last-1]<>" "|.

An overflow error is given, however, if the normal actions of |lua_input_ln|
would make |last>=buf_size|; this is done so that other parts of \TeX\
can safely look at the contents of |buffer[last+1]| without overstepping
the bounds of the |buffer| array. Upon entry to |lua_input_ln|, the condition
|first<buf_size| will always hold, so that there is always room for an
``empty'' line.

The variable |max_buf_stack|, which is used to keep track of how large
the |buf_size| parameter must be to accommodate the present job, is
also kept up to date by |lua_input_ln|.

If the |bypass_eoln| parameter is |true|, |lua_input_ln| will do a |get|
before looking at the first character of the line; this skips over
an |eoln| that was in |f^|. The procedure does not do a |get| when it
reaches the end of the line; therefore it can be used to acquire input
from the user's terminal as well as from ordinary text files.

Since the inner loop of |lua_input_ln| is part of \TeX's ``inner loop''---each
character of input comes in at this place---it is wise to reduce system
overhead by making use of special routines that read in an entire array
of characters at once, if such routines are available. 
@^inner loop@>

@c
boolean lua_input_ln(alpha_file f, int n, boolean bypass_eoln)
{
    boolean lua_result;
    int last_ptr;
    int callback_id;
    (void) bypass_eoln;         /* todo: variable can be removed */
    if (n == 0)
        callback_id = input_file_callback_id[iindex];
    else
        callback_id = read_file_callback_id[n];
    if (callback_id > 0) {
        last = first;
        last_ptr = first;
        lua_result =
            run_saved_callback(callback_id, "reader", "->l", &last_ptr);
        if ((lua_result == true) && (last_ptr != 0)) {
            last = last_ptr;
            if (last > max_buf_stack)
                max_buf_stack = last;
        } else {
            lua_result = false;
        }
    } else {
        lua_result = input_ln(f, bypass_eoln);
    }
    if (lua_result == true) {
        /* Fix up the input buffer using callbacks */
        if (last >= first) {
            callback_id = callback_defined(process_input_buffer_callback);
            if (callback_id > 0) {
                last_ptr = first;
                lua_result =
                    run_callback(callback_id, "l->l", (last - first),
                                 &last_ptr);
                if ((lua_result == true) && (last_ptr != 0)) {
                    last = last_ptr;
                    if (last > max_buf_stack)
                        max_buf_stack = last;
                }
            }
        }
        return true;
    }
    return false;
}


@ We need a special routine to read the first line of \TeX\ input from
the user's terminal. This line is different because it is read before we
have opened the transcript file; there is sort of a ``chicken and
egg'' problem here. If the user types `\.{\\input paper}' on the first
line, or if some macro invoked by that line does such an \.{\\input},
the transcript file will be named `\.{paper.log}'; but if no \.{\\input}
commands are performed during the first line of terminal input, the transcript
file will acquire its default name `\.{texput.log}'. (The transcript file
will not contain error messages generated by the first line before the
first \.{\\input} command.)
@.texput@>

The first line is special also because it may be read before \TeX\ has
input a format file. In such cases, normal error messages cannot yet
be given. The following code uses concepts that will be explained later.

@ Different systems have different ways to get started. But regardless of
what conventions are adopted, the routine that initializes the terminal
should satisfy the following specifications:

\yskip\textindent{1)}It should open file |term_in| for input from the
  terminal. (The file |term_out| will already be open for output to the
  terminal.)

\textindent{2)}If the user has given a command line, this line should be
  considered the first line of terminal input. Otherwise the
  user should be prompted with `\.{**}', and the first line of input
  should be whatever is typed in response.

\textindent{3)}The first line of input, which might or might not be a
  command line, should appear in locations |first| to |last-1| of the
  |buffer| array.

\textindent{4)}The global variable |loc| should be set so that the
  character to be read next by \TeX\ is in |buffer[loc]|. This
  character should not be blank, and we should have |loc<last|.

\yskip\noindent(It may be necessary to prompt the user several times
before a non-blank line comes in. The prompt is `\.{**}' instead of the
later `\.*' because the meaning is slightly different: `\.{\\input}' need
not be typed immediately after~`\.{**}'.)

The following program does the required initialization.
Iff anything has been specified on the command line, then |t_open_in|
will return with |last > first|.
@^system dependencies@>

@c
boolean init_terminal(void)
{                               /* gets the terminal input started */
    t_open_in();
    if (last > first) {
        iloc = first;
        while ((iloc < last) && (buffer[iloc] == ' '))
            incr(iloc);
        if (iloc < last) {
            return true;
        }
    }
    while (1) {
        wake_up_terminal();
        fputs("**", term_out);
        update_terminal();
        if (!input_ln(term_in, true)) {
            /* this shouldn't happen */
            fputs("\n! End of file on the terminal... why?\n", term_out);
            return false;
        }
        iloc = first;
        while ((iloc < last) && (buffer[iloc] == ' '))
            incr(iloc);
        if (iloc < last) {
            return true;        /* return unless the line was all blank */
        }
        fputs("Please type the name of your input file.\n", term_out);
    }
}


@ Here is a procedure that asks the user to type a line of input,
assuming that the |selector| setting is either |term_only| or |term_and_log|.
The input is placed into locations |first| through |last-1| of the
|buffer| array, and echoed on the transcript file if appropriate.

@c
void term_input(void)
{                               /* gets a line from the terminal */
    int k;                      /* index into |buffer| */
    update_terminal();          /* now the user sees the prompt for sure */
    if (!input_ln(term_in, true))
        fatal_error("End of file on the terminal!");
    term_offset = 0;            /* the user's line ended with \.{<return>} */
    decr(selector);             /* prepare to echo the input */
    if (last != first) {
        for (k = first; k <= last - 1; k++)
            print_char(buffer[k]);
    }
    print_ln();
    incr(selector);             /* restore previous status */
}


@ It's time now to fret about file names.  Besides the fact that different
operating systems treat files in different ways, we must cope with the
fact that completely different naming conventions are used by different
groups of people. The following programs show what is required for one
particular operating system; similar routines for other systems are not
difficult to devise.
@^fingers@>
@^system dependencies@>

\TeX\ assumes that a file name has three parts: the name proper; its
``extension''; and a ``file area'' where it is found in an external file
system.  The extension of an input file or a write file is assumed to be
`\.{.tex}' unless otherwise specified; it is `\.{.log}' on the
transcript file that records each run of \TeX; it is `\.{.tfm}' on the font
metric files that describe characters in the fonts \TeX\ uses; it is
`\.{.dvi}' on the output files that specify typesetting information; and it
is `\.{.fmt}' on the format files written by \.{INITEX} to initialize \TeX.
The file area can be arbitrary on input files, but files are usually
output to the user's current area.  If an input file cannot be
found on the specified area, \TeX\ will look for it on a special system
area; this special area is intended for commonly used input files like
\.{webmac.tex}.

Simple uses of \TeX\ refer only to file names that have no explicit
extension or area. For example, a person usually says `\.{\\input} \.{paper}'
or `\.{\\font\\tenrm} \.= \.{helvetica}' instead of `\.{\\input}
\.{paper.new}' or `\.{\\font\\tenrm} \.= \.{<csd.knuth>test}'. Simple file
names are best, because they make the \TeX\ source files portable;
whenever a file name consists entirely of letters and digits, it should be
treated in the same way by all implementations of \TeX. However, users
need the ability to refer to other files in their environment, especially
when responding to error messages concerning unopenable files; therefore
we want to let them use the syntax that appears in their favorite
operating system.

The following procedures don't allow spaces to be part of
file names; but some users seem to like names that are spaced-out.
System-dependent changes to allow such things should probably
be made with reluctance, and only when an entire file name that
includes spaces is ``quoted'' somehow.

Here are the global values that file names will be scanned into.

@c
str_number cur_name;            /* name of file just scanned */
str_number cur_area;            /* file area just scanned, or \.{""} */
str_number cur_ext;             /* file extension just scanned, or \.{""} */


@ The file names we shall deal with have the
following structure:  If the name contains `\./' or `\.:'
(for Amiga only), the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the last
`\..' to the end, otherwise the file extension is null.

We can scan such file names easily by using two global variables that keep track
of the occurrences of area and extension delimiters:

@c
pool_pointer area_delimiter;    /* the most recent `\./', if any */
pool_pointer ext_delimiter;     /* the relevant `\..', if any */


@ Input files that can't be found in the user's area may appear in a standard
system area called |TEX_area|. Font metric files whose areas are not given
explicitly are assumed to appear in a standard system area called
|TEX_font_area|.  $\Omega$'s compiled translation process files whose areas
are not given explicitly are assumed to appear in a standard system area. 
These system area names will, of course, vary from place to place.

@c
#define append_to_fn(A) do {                                    \
        c=(A);                                                  \
        if (c!='"') {                                           \
            if (k<file_name_size) fn[k++]=(unsigned char)(c);   \
        }                                                       \
    } while (0)


char *pack_file_name(str_number n, str_number a, str_number e)
{
    ASCII_code c;               /* character being packed */
    unsigned char *j;           /* index into |str_pool| */
    int k = 0;                  /* number of positions filled in |fn| */
    unsigned char *fn = xmallocarray(packed_ASCII_code,
                                     str_length(a) + str_length(n) +
                                     str_length(e) + 1);
    for (j = str_string(a); j < str_string(a) + str_length(a); j++)
        append_to_fn(*j);
    for (j = str_string(n); j < str_string(n) + str_length(n); j++)
        append_to_fn(*j);
    for (j = str_string(e); j < str_string(e) + str_length(e); j++)
        append_to_fn(*j);
    fn[k] = 0;
    return (char *) fn;
}



@ A messier routine is also needed, since format file names must be scanned
before \TeX's string mechanism has been initialized. We shall use the
global variable |TEX_format_default| to supply the text for default system areas
and extensions related to format files.
@^system dependencies@>

Under {\mc UNIX} we don't give the area part, instead depending
on the path searching that will happen during file opening.  Also, the
length will be set in the main program.

@c
char *TEX_format_default;


@ This part of the program becomes active when a ``virgin'' \TeX\ is trying to get going, 
just after the preliminary initialization, or when the user is substituting another
format file by typing `\.\&' after the initial `\.{**}' prompt.  The buffer
contains the first line of input in |buffer[loc..(last-1)]|, where
|loc<last| and |buffer[loc]<>" "|.

@c
char *open_fmt_file(void)
{
    int j;                      /* the first space after the format file name */
    char *fmt = NULL;
    int dist;
    j = iloc;
    if (buffer[iloc] == '&') {
        incr(iloc);
        j = iloc;
        buffer[last] = ' ';
        while (buffer[j] != ' ')
            incr(j);
        fmt = xmalloc((unsigned) (j - iloc + 1));
        strncpy(fmt, (char *) (buffer + iloc), (size_t) (j - iloc));
        fmt[j - iloc] = 0;
        dist = (int) (strlen(fmt) - strlen(DUMP_EXT));
        if (!(strstr(fmt, DUMP_EXT) == fmt + dist))
            fmt = concat(fmt, DUMP_EXT);
        if (zopen_w_input(&fmt_file, fmt, DUMP_FORMAT, FOPEN_RBIN_MODE))
            goto FOUND;
        wake_up_terminal();
        fprintf(stdout, "Sorry, I can't find the format `%s'; will try `%s'.\n",
                fmt, TEX_format_default);
        update_terminal();
    }
    /* now pull out all the stops: try for the system \.{plain} file */
    fmt = TEX_format_default;
    if (!zopen_w_input(&fmt_file, fmt, DUMP_FORMAT, FOPEN_RBIN_MODE)) {
        wake_up_terminal();
        fprintf(stdout, "I can't find the format file `%s'!\n",
                TEX_format_default);
        return NULL;
    }
  FOUND:
    iloc = j;
    return fmt;
}


@ The global variable |name_in_progress| is used to prevent recursive
use of |scan_file_name|, since the |begin_name| and other procedures
communicate via global variables. Recursion would arise only by
devious tricks like `\.{\\input\\input f}'; such attempts at sabotage
must be thwarted. Furthermore, |name_in_progress| prevents \.{\\input}
@^recursion@>
from being initiated when a font size specification is being scanned.

Another global variable, |job_name|, contains the file name that was first
\.{\\input} by the user. This name is extended by `\.{.log}' and `\.{.dvi}'
and `\.{.fmt}' in the names of \TeX's output files.

@c
boolean name_in_progress;       /* is a file name being scanned? */
str_number job_name;            /* principal file name */
boolean log_opened;             /* has the transcript file been opened? */


@ Initially |job_name=0|; it becomes nonzero as soon as the true name is known.
We have |job_name=0| if and only if the `\.{log}' file has not been opened,
except of course for a short time just after |job_name| has become nonzero.

@c
unsigned char *texmf_log_name;  /* full name of the log file */


@ The |open_log_file| routine is used to open the transcript file and to help
it catch up to what has previously been printed on the terminal.

@c
void open_log_file(void)
{
    int old_setting;            /* previous |selector| setting */
    int k;                      /* index into |buffer| */
    int l;                      /* end of first input line */
    char *fn;
    old_setting = selector;
    if (job_name == 0)
        job_name = getjobname(maketexstring("texput")); /* TODO */
    fn = pack_job_name(".fls");
    recorder_change_filename(fn);
    fn = pack_job_name(".log");
    while (!lua_a_open_out(&log_file, fn, 0)) {
        /* Try to get a different log file name */
        /* Sometimes |open_log_file| is called at awkward moments when \TeX\ is
           unable to print error messages or even to |show_context|.
           The |prompt_file_name| routine can result in a |fatal_error|, but the |error|
           routine will not be invoked because |log_opened| will be false.

           The normal idea of |batch_mode| is that nothing at all should be written
           on the terminal. However, in the unusual case that
           no log file could be opened, we make an exception and allow
           an explanatory message to be seen.

           Incidentally, the program always refers to the log file as a `\.{transcript
           file}', because some systems cannot use the extension `\.{.log}' for
           this file.
         */
        selector = term_only;
        fn = prompt_file_name("transcript file name", ".log");
    }
    texmf_log_name = (unsigned char *) xstrdup(fn);
    selector = log_only;
    log_opened = true;
    if (callback_defined(start_run_callback) == 0) {
        /* Print the banner line, including the date and time */
        log_banner(luatex_version_string, luatex_date_info, luatex_svn);

        input_stack[input_ptr] = cur_input;     /* make sure bottom level is in memory */
        tprint_nl("**");
        l = input_stack[0].limit_field; /* last position of first line */
        if (buffer[l] == end_line_char)
            decr(l);            /* TODO: multichar endlinechar */
        for (k = 1; k <= l; k++)
            print_char(buffer[k]);
        print_ln();             /* now the transcript file contains the first line of input */
    }
    flush_loggable_info();      /* should be done always */
    selector = old_setting + 2; /* |log_only| or |term_and_log| */
}

@ This function is needed by synctex to make its log appear in the right
spot when |output_directory| is set.

@c
char *get_full_log_name (void)
{
   if (output_directory) {
       char *ret  = xmalloc(strlen((char *)texmf_log_name)+2+strlen(output_directory));
       ret = strcpy(ret, output_directory);
       strcat(ret, "/");
       strcat(ret, (char *)texmf_log_name);
       return ret;
   } else {
       return xstrdup((const char*)texmf_log_name);
   } 
}

@ Let's turn now to the procedure that is used to initiate file reading
when an `\.{\\input}' command is being processed.

@c
void start_input(void)
{                               /* \TeX\ will \.{\\input} something */
    str_number temp_str;
    char *fn;
    do {
        get_x_token();
    } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));

    back_input();
    if (cur_cmd != left_brace_cmd) {
        scan_file_name();       /* set |cur_name| to desired file name */
    } else {
        scan_file_name_toks();
    }
    fn = pack_file_name(cur_name, cur_area, cur_ext);
    while (1) {
        begin_file_reading();   /* set up |cur_file| and new level of input */
        if (lua_a_open_in(&cur_file, fn, 0))
            break;
        end_file_reading();     /* remove the level that didn't work */
        fn = prompt_file_name("input file name", "");
    }
    iname = maketexstring(fullnameoffile);
    /* Now that we have |fullnameoffile|, it is time to post-adjust 
      |cur_name| and |cur_ext| for trailing |.tex| */
    {
	char *n, *p;
	n = p = fullnameoffile + strlen(fullnameoffile);
	while (p>fullnameoffile) {
	    p--;
            if (IS_DIR_SEP(*p)) {
	        break;
            }
	}
	if (IS_DIR_SEP(*p)) {
	    p++;
	}
	while (n>fullnameoffile) {
	    n--;
            if (*n == '.') {
	        break;
            }
	}
	if (n>p) {
	    int q = *n;
	    cur_ext = maketexstring(n);
	    *n = 0;
	    cur_name = maketexstring(p);
	    *n = q;
        }
    }


    source_filename_stack[in_open] = iname;
    full_source_filename_stack[in_open] = xstrdup(fullnameoffile);
    /* we can try to conserve string pool space now */
    temp_str = search_string(iname);
    if (temp_str > 0) {
        flush_str(iname);
        iname = temp_str;
    }
    if (job_name == 0) {
        job_name = getjobname(cur_name);
        open_log_file();
    }
    /* |open_log_file| doesn't |show_context|, so |limit|
       and |loc| needn't be set to meaningful values yet */
    if (tracefilenames) {
        if (term_offset + (int) str_length(iname) > max_print_line - 2)
            print_ln();
        else if ((term_offset > 0) || (file_offset > 0))
            print_char(' ');
        print_char('(');
        tprint_file_name(NULL, (unsigned char *) fullnameoffile, NULL);
    }
    incr(open_parens);
    update_terminal();
    istate = new_line;
    /* Prepare new file {\sl Sync\TeX} information */
    synctexstartinput();        /* Give control to the {\sl Sync\TeX} controller */

    /* Read the first line of the new file */
    /* Here we have to remember to tell the |lua_input_ln| routine not to
       start with a |get|. If the file is empty, it is considered to
       contain a single blank line. */

    line = 1;
    if (lua_input_ln(cur_file, 0, false)) {
        ;
    }
    firm_up_the_line();
    if (end_line_char_inactive())
        decr(ilimit);
    else
        buffer[ilimit] = (packed_ASCII_code) end_line_char;
    first = ilimit + 1;
    iloc = istart;
}

@ Read and write dump files through zlib

@ Earlier versions recast |*f| from |FILE *| to |gzFile|, but there is
no guarantee that these have the same size, so a static variable 
is needed.

@c
static gzFile gz_fmtfile = NULL;

@ That second swap is to make sure following calls don't get
confused in the case of |dump_things|.

@c
void do_zdump(char *p, int item_size, int nitems, FILE * out_file)
{
    int err;
    (void) out_file;
    if (nitems == 0)
        return;
#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
    swap_items(p, nitems, item_size);
#endif
    if (gzwrite(gz_fmtfile, (void *) p, (unsigned) (item_size * nitems)) !=
        item_size * nitems) {
        fprintf(stderr, "! Could not write %d %d-byte item(s): %s.\n", nitems,
                item_size, gzerror(gz_fmtfile, &err));
        uexit(1);
    }
#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
    swap_items(p, nitems, item_size);
#endif
}

@ @c
void do_zundump(char *p, int item_size, int nitems, FILE * in_file)
{
    int err;
    (void) in_file;
    if (nitems == 0)
        return;
    if (gzread(gz_fmtfile, (void *) p, (unsigned) (item_size * nitems)) <= 0) {
        fprintf(stderr, "Could not undump %d %d-byte item(s): %s.\n",
                nitems, item_size, gzerror(gz_fmtfile, &err));
        uexit(1);
    }
#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
    swap_items(p, nitems, item_size);
#endif
}

@ @c
#define COMPRESSION "R3"

boolean zopen_w_input(FILE ** f, const char *fname, int format,
                      const_string fopen_mode)
{
    int callbackid;
    int res;
    char *fnam;
    callbackid = callback_defined(find_format_file_callback);
    if (callbackid > 0) {
        res = run_callback(callbackid, "S->S", fname, &fnam);
        if (res && fnam && strlen(fnam) > 0) {
            *f = xfopen(fnam, fopen_mode);
            if (*f == NULL) {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        res = luatex_open_input(f, fname, format, fopen_mode, true);
    }
    if (res) {
        gz_fmtfile = gzdopen(fileno(*f), "rb" COMPRESSION);
    }
    return res;
}

@ @c
boolean zopen_w_output(FILE ** f, const char *s, const_string fopen_mode)
{
    int res = 1;
    if (luainit) {
        *f = fopen(s, fopen_mode);
        if (*f == NULL) {
            return 0;
        }
    } else {
        res = luatex_open_output(f, s, fopen_mode);
    }
    if (res) {
        gz_fmtfile = gzdopen(fileno(*f), "wb" COMPRESSION);
    }
    return res;
}

@ @c
void zwclose(FILE * f)
{
    (void) f;
    gzclose(gz_fmtfile);
}

@  create the dvi or pdf file 
@c
int open_outfile(FILE ** f, const char *name, const char *mode)
{
    FILE *res;
    res = fopen(name, mode);
    if (res != NULL) {
        *f = res;
        return 1;
    }
    return 0;
}


@ the caller should set |tfm_buffer=NULL| and |tfm_size=0|
@c
int readbinfile(FILE * f, unsigned char **tfm_buffer, int *tfm_size)
{
    void *buf;
    int size;
    if (fseek(f, 0, SEEK_END) == 0) {
        size = (int) ftell(f);
        if (size > 0) {
            buf = xmalloc((unsigned) size);
            if (fseek(f, 0, SEEK_SET) == 0) {
                if (fread((void *) buf, (size_t) size, 1, f) == 1) {
                    *tfm_buffer = (unsigned char *) buf;
                    *tfm_size = size;
                    return 1;
                }
            }
        } else {
            *tfm_buffer = NULL;
            *tfm_size = 0;
            return 1;
        }
    }                           /* seek failed, or zero-sized file */
    return 0;
}


@ Like |runsystem()|, the |runpopen()| function is called only when
   |shellenabledp == 1|.   Unlike |runsystem()|, here we write errors to
   stderr, since we have nowhere better to use; and of course we return
   a file handle (or NULL) instead of a status indicator. 

@c
static FILE *runpopen(char *cmd, const char *mode)
{
    FILE *f = NULL;
    char *safecmd = NULL;
    char *cmdname = NULL;
    int allow;

    /* If restrictedshell == 0, any command is allowed. */
    if (restrictedshell == 0) {
        allow = 1;
    } else {
        const char *thecmd = cmd;
        allow = shell_cmd_is_allowed(&thecmd, &safecmd, &cmdname);
    }
    if (allow == 1)
        f = popen(cmd, mode);
    else if (allow == 2)
        f = popen(safecmd, mode);
    else if (allow == -1)
        fprintf(stderr, "\nrunpopen quotation error in command line: %s\n",
                cmd);
    else
        fprintf(stderr, "\nrunpopen command not allowed: %s\n", cmdname);

    if (safecmd)
        free(safecmd);
    if (cmdname)
        free(cmdname);
    return f;
}

@ Return true if FNAME is acceptable as a name for \.{\\openout}, \.{\\openin}, or
   \.{\\input}.  

@c
typedef enum ok_type {
    ok_reading,
    ok_writing
} ok_type;

static const_string ok_type_name[] = {
    "reading",
    "writing"
};

static boolean
opennameok(const_string fname, const_string check_var,
           const_string default_choice, ok_type action)
{
    /* We distinguish three cases:
       'a' (any)        allows any file to be opened.
       'r' (restricted) means disallowing special file names.
       'p' (paranoid)   means being really paranoid: disallowing special file
       names and restricting output files to be in or below
       the working directory or $TEXMFOUTPUT, while input files
       must be below the current directory, $TEXMFOUTPUT, or
       (implicitly) in the system areas.
       We default to "paranoid".  The error messages from TeX will be somewhat
       puzzling...
       This function contains several return statements...  */

    const_string open_choice = kpse_var_value(check_var);

    if (!open_choice)
        open_choice = default_choice;

    if (*open_choice == 'a' || *open_choice == 'y' || *open_choice == '1')
        return true;

#if defined (unix) && !defined (MSDOS)
    {
        const_string base = xbasename(fname);
        /* Disallow .rhosts, .login, etc.  Allow .tex (for LaTeX).  */
        if (base[0] == 0 ||
            (base[0] == '.' && !IS_DIR_SEP(base[1]) && !STREQ(base, ".tex"))) {
            fprintf(stderr, "%s: Not %s to %s (%s = %s).\n",
                    program_invocation_name, ok_type_name[action], fname,
                    check_var, open_choice);
            return false;
        }
    }
#else
    /* Other OSs don't have special names? */
#endif

    if (*open_choice == 'r' || *open_choice == 'n' || *open_choice == '0')
        return true;

    /* Paranoia supplied by Charles Karney...  */
    if (kpse_absolute_p(fname, false)) {
        const_string texmfoutput = kpse_var_value("TEXMFOUTPUT");
        /* Absolute pathname is only OK if TEXMFOUTPUT is set, it's not empty,
           fname begins the TEXMFOUTPUT, and is followed by / */
        if (!texmfoutput || *texmfoutput == '\0'
            || fname != strstr(fname, texmfoutput)
            || !IS_DIR_SEP(fname[strlen(texmfoutput)])) {
            fprintf(stderr, "%s: Not %s to %s (%s = %s).\n",
                    program_invocation_name, ok_type_name[action], fname,
                    check_var, open_choice);
            return false;
        }
    }
    /* For all pathnames, we disallow "../" at the beginning or "/../"
       anywhere.  */
    if (fname[0] == '.' && fname[1] == '.' && IS_DIR_SEP(fname[2])) {
        fprintf(stderr, "%s: Not %s to %s (%s = %s).\n",
                program_invocation_name, ok_type_name[action], fname,
                check_var, open_choice);
        return false;
    } else {
        /* Check for "/../".  Since more than one characted can be matched
           by |IS_DIR_SEP|, we cannot use "/../" itself. */
        const_string dotpair = strstr(fname, "..");
        while (dotpair) {
            /* If dotpair[2] == |DIR_SEP|, then dotpair[-1] is well-defined,
               because the "../" case was handled above. */
            if (IS_DIR_SEP(dotpair[2]) && IS_DIR_SEP(dotpair[-1])) {
                fprintf(stderr, "%s: Not %s to %s (%s = %s).\n",
                        program_invocation_name, ok_type_name[action], fname,
                        check_var, open_choice);
                return false;
            }
            /* Continue after the dotpair. */
            dotpair = strstr(dotpair + 2, "..");
        }
    }

    /* We passed all tests.  */
    return true;
}

boolean openinnameok(const_string fname)
{
    /* For input default to all. */
    return opennameok(fname, "openin_any", "a", ok_reading);
}

#if defined(WIN32) || defined(__MINGW32__) || defined(__CYGWIN__)

static int Isspace(char c)
{
    return (c == ' ' || c == '\t');
}

static boolean executable_filep(const_string fname)
{
    string p, q, base;
    string *pp;

    /*  check |openout_any| */
    p = kpse_var_value("openout_any");
    if (p && *p == 'p') {
        free(p);
/* get base name
   we cannot use xbasename() for abnormal names.
*/
        base = xstrdup(fname);
        p = strrchr(fname, '/');
        if (p) {
            p++;
            strcpy(base, p);
        }
        p = strrchr(base, '\\');
        if (p) {
            p++;
            strcpy(base, p);
        }
#  if defined(__CYGWIN__)
        for (p = base; *p; p++)
            *p = tolower(*p);
        p = base;
#  else
        p = (char *) strlwr(base);
#  endif
        for (q = p + strlen(p) - 1;
             (q >= p) && ((*q == '.') || (Isspace(*q))); q--) {
            *q = '\0';          /* remove trailing '.' , ' ' and '\t' */
        }
        q = strrchr(p, '.');    /* get extension part */
        pp = suffixlist;
        if (pp && q) {
            while (*pp) {
                if (strchr(fname, ':') || !strcmp(q, *pp)) {
                    fprintf(stderr,
                            "\nThe name %s is forbidden to open for writing.\n",
                            fname);
                    free(base);
                    return true;
                }
                pp++;
            }
        }
        free(base);
    } else if (p) {
        free(p);
    }
    return false;
}
#endif

boolean openoutnameok(const_string fname)
{
#if defined(WIN32) || defined(__MINGW32__) || defined(__CYGWIN__)
    /* Output of an executable file is restricted on Windows */
    if (executable_filep(fname))
        return false;
#endif
    /* For output, default to paranoid. */
    return opennameok(fname, "openout_any", "p", ok_writing);
}

 
@  piped I/O


@ The code that implements |popen()| needs an array for tracking 
   possible pipe file pointers, because these need to be
   closed using |pclose()|.

@c
static FILE *pipes[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

boolean open_in_or_pipe(FILE ** f_ptr, char *fn, int filefmt,
                        const_string fopen_mode, boolean must_exist)
{
    string fname = NULL;
    int i;                      /* iterator */

    /* opening a read pipe is straightforward, only have to
       skip past the pipe symbol in the file name. filename
       quoting is assumed to happen elsewhere (it does :-)) */

    if (shellenabledp && *fn == '|') {
        /* the user requested a pipe */
        *f_ptr = NULL;
        fname = (string) xmalloc((unsigned) (strlen(fn) + 1));
        strcpy(fname, fn);
        if (fullnameoffile)
            free(fullnameoffile);
        fullnameoffile = xstrdup(fname);
        recorder_record_input(fname + 1);
        *f_ptr = runpopen(fname + 1, "r");
        free(fname);
        for (i = 0; i <= 15; i++) {
            if (pipes[i] == NULL) {
                pipes[i] = *f_ptr;
                break;
            }
        }
        if (*f_ptr)
            setvbuf(*f_ptr, (char *) NULL, _IOLBF, 0);

        return *f_ptr != NULL;
    }

    return luatex_open_input(f_ptr, fn, filefmt, fopen_mode, must_exist);
}


boolean open_out_or_pipe(FILE ** f_ptr, char *fn, const_string fopen_mode)
{
    string fname;
    int i;                      /* iterator */

    /* opening a write pipe takes a little bit more work, because TeX
       will perhaps have appended ".tex".  To avoid user confusion as
       much as possible, this extension is stripped only when the command
       is a bare word.  Some small string trickery is needed to make
       sure the correct number of bytes is free()-d afterwards */

    if (shellenabledp && *fn == '|') {
        /* the user requested a pipe */
        fname = (string) xmalloc((unsigned) (strlen(fn) + 1));
        strcpy(fname, fn);
        if (strchr(fname, ' ') == NULL && strchr(fname, '>') == NULL) {
            /* mp and mf currently do not use this code, but it 
               is better to be prepared */
            if (STREQ((fname + strlen(fname) - 3), "tex"))
                *(fname + strlen(fname) - 4) = 0;
            *f_ptr = runpopen(fname + 1, "w");
            *(fname + strlen(fname)) = '.';
        } else {
            *f_ptr = runpopen(fname + 1, "w");
        }
        recorder_record_output(fname + 1);
        free(fname);

        for (i = 0; i <= 15; i++) {
            if (pipes[i] == NULL) {
                pipes[i] = *f_ptr;
                break;
            }
        }

        if (*f_ptr)
            setvbuf(*f_ptr, (char *) NULL, _IOLBF, 0);

        return *f_ptr != NULL;
    }

    return luatex_open_output(f_ptr, fn, fopen_mode);
}


void close_file_or_pipe(FILE * f)
{
    int i;                      /* iterator */

    if (shellenabledp) {
        for (i = 0; i <= 15; i++) {
        /* if this file was a pipe, |pclose()| it and return */
            if (pipes[i] == f) {
                if (f)
                    pclose(f);
                pipes[i] = NULL;
                return;
            }
        }
    }
    close_file(f);
}
