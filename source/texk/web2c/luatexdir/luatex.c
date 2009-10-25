/* texmf.c: Hand-coded routines for TeX or Metafont in C.  Originally
   written by Tim Morgan, drawing from other Unix ports of TeX.  This is
   a collection of miscellany, everything that's easier (or only
   possible) to do in C.
   
   This file is public domain.  */

/* This file is used to create texextra.c etc., with this line
   changed to include texd.h or mfd.h.  The ?d.h file is what
   #defines TeX or MF, which avoids the need for a special
   Makefile rule.  */
#include "luatex.h"

static const char __svn_version[] =
    "$Id$ "
    "$URL$";

#define TeX

int luatex_version = 44;        /* \.{\\luatexversion}  */
int luatex_revision = '0';      /* \.{\\luatexrevision}  */
int luatex_date_info = -extra_version_info;     /* the compile date is negated */
char *luatex_version_string = "beta-0.44.0";
char *engine_name = "luatex";   /* the name of this engine */



#include <kpathsea/c-ctype.h>
#include <kpathsea/line.h>
#include <kpathsea/readable.h>
#include <kpathsea/variable.h>
#include <kpathsea/absolute.h>
#include <kpathsea/recorder.h>

#include <time.h>               /* For `struct tm'.  */
#if defined (HAVE_SYS_TIME_H)
#  include <sys/time.h>
#elif defined (HAVE_SYS_TIMEB_H)
#  include <sys/timeb.h>
#endif

#if defined(__STDC__)
#  include <locale.h>
#endif

#include <signal.h>             /* Catch interrupts.  */

#include <texmfmp-help.h>

/* {tex,mf}d.h defines TeX, MF, INI, and other such symbols.
   Unfortunately there's no way to get the banner into this code, so
   just repeat the text.  */
#define edit_var "TEXEDIT"

/* Shell escape.

   If shellenabledp == 0, all shell escapes are forbidden.
   If (shellenabledp == 1 && restrictedshell == 0), any command
     is allowed for a shell escape.
   If (shellenabledp == 1 && restrictedshell == 1), only commands
     given in the configuration file as
   shell_escape_commands = kpsewhich,ebb,extractbb,mpost,metafun,...
     (no spaces between commands) in texmf.cnf are allowed for a shell
     escape in a restricted form: command name and arguments should be
     separated by a white space. The first word should be a command
     name. The quotation character for an argument with spaces,
     including a pathname, should be ".  ' should not be used.

     Internally, all arguments are quoted by ' (Unix) or " (Windows)
     before calling the system() function in order to forbid execution
     of any embedded command.  In addition, on Windows, special
     characters of cmd.exe are escaped by using (^).

   If the --shell-escape option is given, we set
     shellenabledp = 1 and restrictedshell = 0, i.e., any command is allowed.
   If the --shell-restricted option is given, we set
     shellenabledp = 1 and restrictedshell = 1, i.e., only given cmds allowed.
   If the --no-shell-escape option is given, we set
     shellenabledp = -1 (and restrictedshell is irrelevant).
   If none of these option are given, there are three cases:
   (1) In the case where
       shell_escape = y or
       shell_escape = t or
       shell_escape = 1
       it becomes shellenabledp = 1 and restrictedshell = 0,
       that is, any command is allowed.
   (2) In the case where
       shell_escape = p
       it becomes shellenabledp = 1 and restrictedshell = 1,
       that is, restricted shell escape is allowed.
   (3) In all other cases, shellenabledp = 0, that is, shell
       escape is forbidden. The value of restrictedshell is
       irrelevant if shellenabledp == 0.
*/

#ifdef TeX

/* cmdlist is a list of allowed commands which are given like this:
   shell_escape_commands = kpsewhich,ebb,extractbb,mpost,metafun
   in texmf.cnf. */

static char **cmdlist = NULL;

void mk_shellcmdlist(char *v)
{
    char **p;
    char *q, *r;
    int n;

    q = v;
    n = 0;

/* analyze the variable shell_escape_commands = foo,bar,...
   spaces before and after (,) are not allowed. */

    while ((r = strchr(q, ',')) != 0) {
        n++;
        r++;
        q = r;
    }
    if (*q)
        n++;
    cmdlist = (char **) xmalloc((n + 1) * sizeof(char *));
    p = cmdlist;
    q = v;
    while ((r = strchr(q, ',')) != 0) {
        *r = '\0';
        *p = (char *) xmalloc(strlen(q) + 1);
        strcpy(*p, q);
        *r = ',';
        r++;
        q = r;
        p++;
    }
    if (*q) {
        *p = (char *) xmalloc(strlen(q) + 1);
        strcpy(*p, q);
        p++;
        *p = NULL;
    } else
        *p = NULL;
}

/* Called from maininit.  Not static because also called from
   luatexdir/lua/luainit.c.  */

void init_shell_escape(void)
{
    if (shellenabledp < 0) {    /* --no-shell-escape on cmd line */
        shellenabledp = 0;

    } else {
        if (shellenabledp == 0) {       /* no shell options on cmd line, check cnf */
            char *v1 = kpse_var_value("shell_escape");
            if (v1) {
                if (*v1 == 't' || *v1 == 'y' || *v1 == '1') {
                    shellenabledp = 1;
                } else if (*v1 == 'p') {
                    shellenabledp = 1;
                    restrictedshell = 1;
                }
                free(v1);
            }
        }

        /* If shell escapes are restricted, get allowed cmds from cnf.  */
        if (shellenabledp && restrictedshell == 1) {
            char *v2 = kpse_var_value("shell_escape_commands");
            if (v2) {
                mk_shellcmdlist(v2);
                free(v2);
            }
        }
    }
}

#  ifdef WIN32
#    define QUOTE '"'
#  else
#    define QUOTE '\''
#  endif

#  ifdef WIN32
static int char_needs_quote(int c)
{
/* special characters of cmd.exe */

    return (c == '&' || c == '|' || c == '%' || c == '<' ||
            c == '>' || c == ';' || c == ',' || c == '(' || c == ')');
}
#  endif

static int Isspace(char c)
{
    return (c == ' ' || c == '\t');
}

/* return values:
  -1 : invalid quotation of an argument
   0 : command is not allowed
   2 : restricted shell escape, CMD is allowed.
   
   We set *SAFECMD to a safely-quoted version of *CMD; this is what
   should get executed.  And we set CMDNAME to its first word; this is
   what is checked against the shell_escape_commands list.  */

int shell_cmd_is_allowed(char **cmd, char **safecmd, char **cmdname)
{
    char **p;
    char *buf;
    char *s, *d;
    int pre, spaces;
    int allow = 0;

    /* pre == 1 means that the previous character is a white space
       pre == 0 means that the previous character is not a white space */
    buf = (char *) xmalloc(strlen(*cmd) + 1);
    strcpy(buf, *cmd);
    s = buf;
    while (Isspace(*s))
        s++;
    d = s;
    while (!Isspace(*d) && *d)
        d++;
    *d = '\0';

    /* *cmdname is the first word of the command line.  For example,
     *cmdname == "kpsewhich" for
     \write18{kpsewhich --progname=dvipdfm --format="other text files" config}
     */
    *cmdname = xstrdup(s);
    free(buf);

    /* Is *cmdname listed in a texmf.cnf vriable as
       shell_escape_commands = foo,bar,... ? */
    p = cmdlist;
    if (p) {
        while (*p) {
            if (strcmp(*p, *cmdname) == 0) {
                /* *cmdname is found in the list, so restricted shell escape
                   is allowed */
                allow = 2;
                break;
            }
            p++;
        }
    }
    if (allow == 2) {
        spaces = 0;
        for (s = *cmd; *s; s++) {
            if (Isspace(*s))
                spaces++;
        }

        /* allocate enough memory (too much?) */
#  ifdef WIN32
        *safecmd = (char *) xmalloc(2 * strlen(*cmd) + 3 + 2 * spaces);
#  else
        *safecmd = (char *) xmalloc(strlen(*cmd) + 3 + 2 * spaces);
#  endif

        /* make a safe command line *safecmd */
        s = *cmd;
        while (Isspace(*s))
            s++;
        d = *safecmd;
        while (!Isspace(*s) && *s)
            *d++ = *s++;

        pre = 1;
        while (*s) {
            /* Quotation given by a user.  " should always be used; we
               transform it below.  On Unix, if ' is used, simply immediately
               return a quotation error.  */
            if (*s == '\'') {
                return -1;
            }

            if (*s == '"') {
                /* All arguments are quoted as 'foo' (Unix) or "foo" (Windows)
                   before calling system(). Therefore closing QUOTE is necessary
                   if the previous character is not a white space.
                   example:
                   --format="other text files" becomes
                   '--format=''other text files' (Unix)
                   "--format=""other test files" (Windows) */

                if (pre == 0)
                    *d++ = QUOTE;

                pre = 0;
                /* output the quotation mark for the quoted argument */
                *d++ = QUOTE;
                s++;

                while (*s != '"') {
                    /* Closing quotation mark is missing */
                    if (*s == '\0')
                        return -1;
#  ifdef WIN32
                    if (char_needs_quote(*s))
                        *d++ = '^';
#  endif
                    *d++ = *s++;
                }

                /* Closing quotation mark will be output afterwards, so
                   we do nothing here */
                s++;

                /* The character after the closing quotation mark
                   should be a white space or NULL */
                if (!Isspace(*s) && *s)
                    return -1;

                /* Beginning of a usual argument */
            } else if (pre == 1 && !Isspace(*s)) {
                pre = 0;
                *d++ = QUOTE;
#  ifdef WIN32
                if (char_needs_quote(*s))
                    *d++ = '^';
#  endif
                *d++ = *s++;
                /* Ending of a usual argument */

            } else if (pre == 0 && Isspace(*s)) {
                pre = 1;
                /* Closing quotation mark */
                *d++ = QUOTE;
                *d++ = *s++;
            } else {
                /* Copy a character from *cmd to *safecmd. */
#  ifdef WIN32
                if (char_needs_quote(*s))
                    *d++ = '^';
#  endif
                *d++ = *s++;
            }
        }
        /* End of the command line */
        if (pre == 0) {
            *d++ = QUOTE;
        }
        *d = '\0';
    }

    return allow;
}

/* We should only be called with shellenabledp == 1.
   Return value:
   -1 if a quotation syntax error.
   0 if CMD is not allowed; given shellenabledp==1, this is because
      shell escapes are restricted and CMD is not allowed.
   1 if shell escapes are not restricted, hence any command is allowed.
   2 if shell escapes are restricted and CMD is allowed.  */

int runsystem(char *cmd)
{
    int allow = 0;
    char *safecmd = NULL;
    char *cmdname = NULL;

    if (shellenabledp <= 0) {
        return 0;
    }

    /* If restrictedshell == 0, any command is allowed. */
    if (restrictedshell == 0)
        allow = 1;
    else
        allow = shell_cmd_is_allowed(&cmd, &safecmd, &cmdname);

    if (allow == 1)
        (void) system(cmd);
    else if (allow == 2)
        (void) system(safecmd);

    if (safecmd)
        free(safecmd);
    if (cmdname)
        free(cmdname);

    return allow;
}

/* Like runsystem(), the runpopen() function is called only when
   shellenabledp == 1.   Unlike runsystem(), here we write errors to
   stderr, since we have nowhere better to use; and of course we return
   a file handle (or NULL) instead of a status indicator.  */

static FILE *runpopen(char *cmd, char *mode)
{
    FILE *f = NULL;
    char *safecmd = NULL;
    char *cmdname = NULL;
    int allow;

    /* If restrictedshell == 0, any command is allowed. */
    if (restrictedshell == 0)
        allow = 1;
    else
        allow = shell_cmd_is_allowed(&cmd, &safecmd, &cmdname);

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
#endif

/* The main program, etc.  */

extern void lua_initialize(int ac, char **av);

/* What we were invoked as and with.  */
char **argv;
int argc;

/* The C version of what might wind up in DUMP_VAR.  */
const_string dump_name;

/* The C version of the jobname, if given. */
const_string c_job_name;

/* Full source file name. */
extern string fullnameoffile;

/* The main body of the WEB is transformed into this procedure.  */
extern TEXDLL void mainbody(void);

char *ptexbanner;

#if !defined(WIN32) || defined(__MINGW32__)
/* The entry point: set up for reading the command line, which will
   happen in `topenin', then call the main body.  */

int main(int ac, string * av)
{
#  ifdef __EMX__
    _wildcard(&ac, &av);
    _response(&ac, &av);
#  endif

#  ifdef WIN32
    _setmaxstdio(2048);
#  endif

    lua_initialize(ac, av);

    /* Call the real main program.  */
    mainbody();

    return EXIT_SUCCESS;
}
#endif                          /* !(WIN32 || __MINGW32__) */


/* This is supposed to ``open the terminal for input'', but what we
   really do is copy command line arguments into TeX's or Metafont's
   buffer, so they can handle them.  If nothing is available, or we've
   been called already (and hence, argc==0), we return with
   `last=first'.  */

void topenin(void)
{
    int i;


    buffer[first] = 0;          /* In case there are no arguments.  */

    if (optind < argc) {        /* We have command line arguments.  */
        int k = first;
        for (i = optind; i < argc; i++) {
            char *ptr = &(argv[i][0]);
            /* Don't use strcat, since in Aleph the buffer elements aren't
               single bytes.  */
            while (*ptr) {
                buffer[k++] = *(ptr++);
            }
            buffer[k++] = ' ';
        }
        argc = 0;               /* Don't do this again.  */
        buffer[k] = 0;
    }

    /* Find the end of the buffer.  */
    for (last = first; buffer[last]; ++last);

    /* Make `last' be one past the last non-blank character in `buffer'.  */
    /* ??? The test for '\r' should not be necessary.  */
    for (--last; last >= first
         && ISBLANK(buffer[last]) && buffer[last] != '\r'; --last);
    last++;

    /* One more time, this time converting to TeX's internal character
       representation.  */
}

/* IPC for TeX.  By Tom Rokicki for the NeXT; it makes TeX ship out the
   DVI file in a pipe to TeXView so that the output can be displayed
   incrementally.  Shamim Mohamed adapted it for Web2c.  */
#if defined (TeX) && defined (IPC)

#  include <sys/socket.h>
#  include <fcntl.h>
#  ifndef O_NONBLOCK            /* POSIX */
#    ifdef O_NDELAY             /* BSD */
#      define O_NONBLOCK O_NDELAY
#    else
#      ifdef FNDELAY            /* NeXT */
#        define O_NONBLOCK O_FNDELAY
#      else
what the fcntl ? cannot implement IPC without equivalent for O_NONBLOCK.
#      endif                    /* no FNDELAY */
#    endif                      /* no O_NDELAY */
#  endif                        /* no O_NONBLOCK */
#  ifndef IPC_PIPE_NAME         /* $HOME is prepended to this.  */
#    define IPC_PIPE_NAME "/.TeXview_Pipe"
#  endif
#  ifndef IPC_SERVER_CMD        /* Command to run to start the server.  */
#    define IPC_SERVER_CMD "open `which TeXview`"
#  endif
    struct msg {
    short namelength;           /* length of auxiliary data */
    int eof;                    /* new eof for dvi file */
#  if 0                         /* see usage of struct msg below */
    char more_data[0];          /* where the rest of the stuff goes */
#  endif
};

static char *ipc_name;
static struct sockaddr *ipc_addr;
static int ipc_addr_len;

static int ipc_make_name(void)
{
    if (ipc_addr_len == 0) {
        string s = getenv("HOME");
        if (s) {
            ipc_addr = (struct sockaddr *) xmalloc(strlen(s) + 40);
            ipc_addr->sa_family = 0;
            ipc_name = ipc_addr->sa_data;
            strcpy(ipc_name, s);
            strcat(ipc_name, IPC_PIPE_NAME);
            ipc_addr_len = strlen(ipc_name) + 3;
        }
    }
    return ipc_addr_len;
}


static int sock = -1;

static int ipc_is_open(void)
{
    return sock >= 0;
}


static void ipc_open_out(void)
{
#  ifdef IPC_DEBUG
    fputs("tex: Opening socket for IPC output ...\n", stderr);
#  endif
    if (sock >= 0) {
        return;
    }

    if (ipc_make_name() < 0) {
        sock = -1;
        return;
    }

    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock >= 0) {
        if (connect(sock, ipc_addr, ipc_addr_len) != 0
            || fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
            close(sock);
            sock = -1;
            return;
        }
#  ifdef IPC_DEBUG
        fputs("tex: Successfully opened IPC socket.\n", stderr);
#  endif
    }
}


static void ipc_close_out(void)
{
#  ifdef IPC_DEBUG
    fputs("tex: Closing output socket ...\n", stderr);
#  endif
    if (ipc_is_open()) {
        close(sock);
        sock = -1;
    }
}


static void ipc_snd(int n, int is_eof, char *data)
{
    struct {
        struct msg msg;
        char more_data[1024];
    } ourmsg;

#  ifdef IPC_DEBUG
    fputs("tex: Sending message to socket ...\n", stderr);
#  endif
    if (!ipc_is_open()) {
        return;
    }

    ourmsg.msg.namelength = n;
    ourmsg.msg.eof = is_eof;
    if (n) {
        strcpy(ourmsg.more_data, data);
    }
    n += sizeof(struct msg);
#  ifdef IPC_DEBUG
    fputs("tex: Writing to socket...\n", stderr);
#  endif
    if (write(sock, &ourmsg, n) != n) {
        ipc_close_out();
    }
#  ifdef IPC_DEBUG
    fputs("tex: IPC message sent.\n", stderr);
#  endif
}


/* This routine notifies the server if there is an eof, or the filename
   if a new DVI file is starting.  This is the routine called by TeX.
   Aleph defines str_start(#) as str_start_ar[# - too_big_char], with
   too_big_char = biggest_char + 1 = 65536 (omstr.ch).*/

void ipcpage(int is_eof)
{
    static boolean begun = false;
    unsigned len = 0;
    string p = (string) "";

    if (!begun) {
        string name;            /* Just the filename.  */
        string cwd = xgetcwd();

        ipc_open_out();
#  if !defined(Aleph)
        len = strstart[outputfilename + 1] - strstart[outputfilename];
#  else
        len = strstartar[outputfilename + 1 - 65536L] -
            strstartar[outputfilename - 65536L];
#  endif
        name = (string) xmalloc(len + 1);
#  if !defined(Aleph)
        strncpy(name, (string) & strpool[strstart[outputfilename]], len);
#  else
        {
            unsigned i;
            for (i = 0; i < len; i++)
                name[i] = strpool[i + strstartar[outputfilename - 65536L]];
        }
#  endif
        name[len] = 0;

        /* Have to pass whole filename to the other end, since it may have
           been started up and running as a daemon, e.g., as with the NeXT
           preview program.  */
        p = concat3(cwd, DIR_SEP_STRING, name);
        free(name);
        len = strlen(p);
        begun = true;
    }
    ipc_snd(len, is_eof, p);

    if (len > 0) {
        free(p);
    }
}
#endif                          /* TeX && IPC */

/* Normalize quoting of filename -- that is, only quote if there is a space,
   and always use the quote-name-quote style. */
string normalize_quotes(const_string name, const_string mesg)
{
    boolean quoted = false;
    boolean must_quote = (strchr(name, ' ') != NULL);
    /* Leave room for quotes and NUL. */
    string ret = (string) xmalloc(strlen(name) + 3);
    string p;
    const_string q;
    p = ret;
    if (must_quote)
        *p++ = '"';
    for (q = name; *q; q++) {
        if (*q == '"')
            quoted = !quoted;
        else
            *p++ = *q;
    }
    if (must_quote)
        *p++ = '"';
    *p = '\0';
    if (quoted) {
        fprintf(stderr, "! Unbalanced quotes in %s %s\n", mesg, name);
        uexit(1);
    }
    return ret;
}

/* Return true if FNAME is acceptable as a name for \openout, \openin, or
   \input.  */

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
           by IS_DIR_SEP, we cannot use "/../" itself. */
        const_string dotpair = strstr(fname, "..");
        while (dotpair) {
            /* If dotpair[2] == DIR_SEP, then dotpair[-1] is well-defined,
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

boolean openoutnameok(const_string fname)
{
    /* For output, default to paranoid. */
    return opennameok(fname, "openout_any", "p", ok_writing);
}

/* 
  piped I/O
 */

/* The code that implements popen() needs an array for tracking 
   possible pipe file pointers, because these need to be
   closed using pclose().
*/

static FILE *pipes[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

boolean open_in_or_pipe(FILE ** f_ptr, int filefmt, const_string fopen_mode)
{
    string fname = NULL;
    int i;                      /* iterator */

    /* opening a read pipe is straightforward, only have to
       skip past the pipe symbol in the file name. filename
       quoting is assumed to happen elsewhere (it does :-)) */

    if (shellenabledp && *(nameoffile + 1) == '|') {
        /* the user requested a pipe */
        *f_ptr = NULL;
        fname = (string) xmalloc(strlen((const_string) (nameoffile + 1)) + 1);
        strcpy(fname, (const_string) (nameoffile + 1));
#if !defined(pdfTeX)
        if (fullnameoffile)
            free(fullnameoffile);
        fullnameoffile = xstrdup(fname);
#endif
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

    return open_input(f_ptr, filefmt, fopen_mode);
}


boolean open_out_or_pipe(FILE ** f_ptr, const_string fopen_mode)
{
    string fname;
    int i;                      /* iterator */

    /* opening a write pipe takes a little bit more work, because TeX
       will perhaps have appended ".tex".  To avoid user confusion as
       much as possible, this extension is stripped only when the command
       is a bare word.  Some small string trickery is needed to make
       sure the correct number of bytes is free()-d afterwards */

    if (shellenabledp && *(nameoffile + 1) == '|') {
        /* the user requested a pipe */
        fname = (string) xmalloc(strlen((const_string) (nameoffile + 1)) + 1);
        strcpy(fname, (const_string) (nameoffile + 1));
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

    return open_output(f_ptr, fopen_mode);
}


void close_file_or_pipe(FILE * f)
{
    int i;                      /* iterator */

    if (shellenabledp) {
        /* if this file was a pipe, pclose() it and return */
        for (i = 0; i <= 15; i++) {
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

/* All our interrupt handler has to do is set TeX's or Metafont's global
   variable `interrupt'; then they will do everything needed.  */
#ifdef WIN32
/* Win32 doesn't set SIGINT ... */
BOOL WINAPI catch_interrupt(DWORD arg)
{
    switch (arg) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        interrupt = 1;
        return TRUE;
    default:
        /* No need to set interrupt as we are exiting anyway */
        return FALSE;
    }
}
#else                           /* not WIN32 */
static RETSIGTYPE catch_interrupt(int arg)
{
    (void) arg;
    interrupt = 1;
#  ifdef OS2
    (void) signal(SIGINT, SIG_ACK);
#  else
    (void) signal(SIGINT, catch_interrupt);
#  endif                        /* not OS2 */
}
#endif                          /* not WIN32 */

/* Besides getting the date and time here, we also set up the interrupt
   handler, for no particularly good reason.  It's just that since the
   `fix_date_and_time' routine is called early on (section 1337 in TeX,
   ``Get the first line of input and prepare to start''), this is as
   good a place as any.  */

void
get_date_and_time(integer * minutes, integer * day,
                  integer * month, integer * year)
{
    time_t myclock = time((time_t *) 0);
    struct tm *tmptr = localtime(&myclock);

    *minutes = tmptr->tm_hour * 60 + tmptr->tm_min;
    *day = tmptr->tm_mday;
    *month = tmptr->tm_mon + 1;
    *year = tmptr->tm_year + 1900;

    {
#ifdef SA_INTERRUPT
        /* Under SunOS 4.1.x, the default action after return from the
           signal handler is to restart the I/O if nothing has been
           transferred.  The effect on TeX is that interrupts are ignored if
           we are waiting for input.  The following tells the system to
           return EINTR from read() in this case.  From ken@cs.toronto.edu.  */

        struct sigaction a, oa;

        a.sa_handler = catch_interrupt;
        sigemptyset(&a.sa_mask);
        sigaddset(&a.sa_mask, SIGINT);
        a.sa_flags = SA_INTERRUPT;
        sigaction(SIGINT, &a, &oa);
        if (oa.sa_handler != SIG_DFL)
            sigaction(SIGINT, &oa, (struct sigaction *) 0);
#else                           /* no SA_INTERRUPT */
#  ifdef WIN32
        SetConsoleCtrlHandler(catch_interrupt, TRUE);
#  else                         /* not WIN32 */
        RETSIGTYPE(*old_handler) (int);

        old_handler = signal(SIGINT, catch_interrupt);
        if (old_handler != SIG_DFL)
            signal(SIGINT, old_handler);
#  endif                        /* not WIN32 */
#endif                          /* no SA_INTERRUPT */
    }
}

/*
 Getting a high resolution time.
 */
void get_seconds_and_micros(integer * seconds, integer * micros)
{
#if defined (HAVE_GETTIMEOFDAY)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;
    *micros = tv.tv_usec;
#elif defined (HAVE_FTIME)
    struct timeb tb;
    ftime(&tb);
    *seconds = tb.time;
    *micros = tb.millitm * 1000;
#else
    time_t myclock = time((time_t *) NULL);
    *seconds = myclock;
    *micros = 0;
#endif
}

/*
  Generating a better seed numbers
  */
integer getrandomseed()
{
#if defined (HAVE_GETTIMEOFDAY)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_usec + 1000000 * tv.tv_usec);
#elif defined (HAVE_FTIME)
    struct timeb tb;
    ftime(&tb);
    return (tb.millitm + 1000 * tb.time);
#else
    time_t myclock = time((time_t *) NULL);
    struct tm *tmptr = localtime(&myclock);
    return (tmptr->tm_sec + 60 * (tmptr->tm_min + 60 * tmptr->tm_hour));
#endif
}

/* Read a line of input as efficiently as possible while still looking
   like Pascal.  We set `last' to `first' and return `false' if we get
   to eof.  Otherwise, we return `true' and set last = first +
   length(line except trailing whitespace).  */

boolean input_line(FILE * f)
{
    int i = EOF;

    /* Recognize either LF or CR as a line terminator.  */
    last = first;
    while (last < buf_size && (i = getc(f)) != EOF && i != '\n' && i != '\r')
        buffer[last++] = i;

    if (i == EOF && errno != EINTR && last == first)
        return false;

    /* We didn't get the whole line because our buffer was too small.  */
    if (i != EOF && i != '\n' && i != '\r') {
        fprintf(stderr, "! Unable to read an entire line---bufsize=%u.\n",
                (unsigned) buf_size);
        fputs("Please increase buf_size in texmf.cnf.\n", stderr);
        uexit(1);
    }

    buffer[last] = ' ';
    if (last >= max_buf_stack)
        max_buf_stack = last;

    /* If next char is LF of a CRLF, read it.  */
    if (i == '\r') {
        while ((i = getc(f)) == EOF && errno == EINTR);
        if (i != '\n')
            ungetc(i, f);
    }

    /* Trim trailing whitespace.  */
    while (last > first && ISBLANK(buffer[last - 1]))
        --last;

    /* Don't bother using xord if we don't need to.  */

    return true;
}


/* This string specifies what the `e' option does in response to an
   error message.  */
static const_string edit_value = EDITOR;

/* This procedure originally due to sjc@s1-c.  TeX & Metafont call it when
   the user types `e' in response to an error, invoking a text editor on
   the erroneous source file.  FNSTART is how far into FILENAME the
   actual filename starts; FNLENGTH is how long the filename is.  */

void
calledit(packedASCIIcode * filename,
         poolpointer fnstart, integer fnlength, integer linenumber)
{
    char *temp, *command;
    char c;
    int sdone, ddone, i;

    sdone = ddone = 0;
    filename += fnstart;

    /* Close any open input files, since we're going to kill the job.  */
    for (i = 1; i <= in_open; i++)
        xfclose(input_file[i], "inputfile");

    /* Replace the default with the value of the appropriate environment
       variable or config file value, if it's set.  */
    temp = kpse_var_value(edit_var);
    if (temp != NULL)
        edit_value = temp;

    /* Construct the command string.  The `11' is the maximum length an
       integer might be.  */
    command = (string) xmalloc(strlen(edit_value) + fnlength + 11);

    /* So we can construct it as we go.  */
    temp = command;

    while ((c = *edit_value++) != 0) {
        if (c == '%') {
            switch (c = *edit_value++) {
            case 'd':
                if (ddone)
                    FATAL("call_edit: `%%d' appears twice in editor command");
                sprintf(temp, "%ld", (long int) linenumber);
                while (*temp != '\0')
                    temp++;
                ddone = 1;
                break;

            case 's':
                if (sdone)
                    FATAL("call_edit: `%%s' appears twice in editor command");
                for (i = 0; i < fnlength; i++)
                    *temp++ = Xchr(filename[i]);
                sdone = 1;
                break;

            case '\0':
                *temp++ = '%';
                /* Back up to the null to force termination.  */
                edit_value--;
                break;

            default:
                *temp++ = '%';
                *temp++ = c;
                break;
            }
        } else
            *temp++ = c;
    }

    *temp = 0;

    /* Execute the command.  */
#ifdef WIN32
    /* Win32 reimplementation of the system() command
       provides opportunity to call it asynchronously */
    if (win32_system(command, true) != 0)
#else
    if (system(command) != 0)
#endif
        fprintf(stderr, "! Trouble executing `%s'.\n", command);

    /* Quit, since we found an error.  */
    uexit(1);
}

/* Read and write dump files.  As distributed, these files are
   architecture dependent; specifically, BigEndian and LittleEndian
   architectures produce different files.  These routines always output
   BigEndian files.  This still does not guarantee them to be
   architecture-independent, because it is possible to make a format
   that dumps a glue ratio, i.e., a floating-point number.  Fortunately,
   none of the standard formats do that.  */

#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)      /* this fn */

/* This macro is always invoked as a statement.  It assumes a variable
   `temp'.  */

#  define SWAP(x, y) temp = (x); (x) = (y); (y) = temp


/* Make the NITEMS items pointed at by P, each of size SIZE, be the
   opposite-endianness of whatever they are now.  */

static void swap_items(char *p, int nitems, int size)
{
    char temp;

    /* Since `size' does not change, we can write a while loop for each
       case, and avoid testing `size' for each time.  */
    switch (size) {
        /* 16-byte items happen on the DEC Alpha machine when we are not
           doing sharable memory dumps.  */
    case 16:
        while (nitems--) {
            SWAP(p[0], p[15]);
            SWAP(p[1], p[14]);
            SWAP(p[2], p[13]);
            SWAP(p[3], p[12]);
            SWAP(p[4], p[11]);
            SWAP(p[5], p[10]);
            SWAP(p[6], p[9]);
            SWAP(p[7], p[8]);
            p += size;
        }
        break;

    case 8:
        while (nitems--) {
            SWAP(p[0], p[7]);
            SWAP(p[1], p[6]);
            SWAP(p[2], p[5]);
            SWAP(p[3], p[4]);
            p += size;
        }
        break;

    case 4:
        while (nitems--) {
            SWAP(p[0], p[3]);
            SWAP(p[1], p[2]);
            p += size;
        }
        break;

    case 2:
        while (nitems--) {
            SWAP(p[0], p[1]);
            p += size;
        }
        break;

    case 1:
        /* Nothing to do.  */
        break;

    default:
        FATAL1("Can't swap a %d-byte item for (un)dumping", size);
    }
}
#endif                          /* not WORDS_BIGENDIAN and not NO_DUMP_SHARE */


/* Here we write NITEMS items, each item being ITEM_SIZE bytes long.
   The pointer to the stuff to write is P, and we write to the file
   OUT_FILE.  */

void do_dump(char *p, int item_size, int nitems, FILE * out_file)
{
#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
    swap_items(p, nitems, item_size);
#endif

    if (fwrite(p, item_size, nitems, out_file) != (unsigned) nitems) {
        fprintf(stderr, "! Could not write %d %d-byte item(s).\n",
                nitems, item_size);
        uexit(1);
    }

    /* Have to restore the old contents of memory, since some of it might
       get used again.  */
#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
    swap_items(p, nitems, item_size);
#endif
}


/* Here is the dual of the writing routine.  */

void do_undump(char *p, int item_size, int nitems, FILE * in_file)
{
    if (fread(p, item_size, nitems, in_file) != (size_t) nitems)
        FATAL2("Could not undump %d %d-byte item(s)", nitems, item_size);

#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
    swap_items(p, nitems, item_size);
#endif
}

/* Look up VAR_NAME in texmf.cnf; assign either the value found there or
   DFLT to *VAR.  */

void setupboundvariable(integer * var, const_string var_name, integer dflt)
{
    string expansion = kpse_var_value(var_name);
    *var = dflt;

    if (expansion) {
        integer conf_val = atoi(expansion);
        /* It's ok if the cnf file specifies 0 for extra_mem_{top,bot}, etc.
           But negative numbers are always wrong.  */
        if (conf_val < 0 || (conf_val == 0 && dflt > 0)) {
            fprintf(stderr,
                    "%s: Bad value (%ld) in texmf.cnf for %s, keeping %ld.\n",
                    program_invocation_name,
                    (long) conf_val, var_name + 1, (long) dflt);
        } else {
            *var = conf_val;    /* We'll make further checks later.  */
        }
        free(expansion);
    }
}

/* FIXME -- some (most?) of this can/should be moved to the Pascal/WEB side. */

str_number makefullnamestring()
{
    return maketexstring(fullnameoffile);
}

/* Get the job name to be used, which may have been set from the
   command line. */
str_number getjobname(str_number name)
{
    str_number ret = name;
    if (c_job_name != NULL)
        ret = maketexstring(c_job_name);
    return ret;
}
