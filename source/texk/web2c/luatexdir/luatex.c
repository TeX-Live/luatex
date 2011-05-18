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
#include "ptexlib.h"
#include "lua/luatex-api.h"
#include "luatex_svnversion.h"

static const char _svn_version[] =
    "$Id$ "
    "$URL$";

#define TeX

int luatex_svn = luatex_svn_revision;
int luatex_version = 70;        /* \.{\\luatexversion}  */
int luatex_revision = '0';      /* \.{\\luatexrevision}  */
int luatex_date_info = -extra_version_info;     /* the compile date is negated */
const char *luatex_version_string = "beta-0.70.0";
const char *engine_name = "luatex";     /* the name of this engine */

#include <kpathsea/c-ctype.h>
#include <kpathsea/line.h>
#include <kpathsea/readable.h>
#include <kpathsea/variable.h>
#include <kpathsea/absolute.h>
#include <kpathsea/recorder.h>
#ifdef WIN32
#include <kpathsea/concatn.h>
#endif


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
    unsigned int n;

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
    cmdlist = (char **) xmalloc((unsigned) ((n + 1) * sizeof(char *)));
    p = cmdlist;
    q = v;
    while ((r = strchr(q, ',')) != 0) {
        *r = '\0';
        *p = (char *) xmalloc((unsigned) strlen(q) + 1);
        strcpy(*p, q);
        *r = ',';
        r++;
        q = r;
        p++;
    }
    if (*q) {
        *p = (char *) xmalloc((unsigned) strlen(q) + 1);
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

int shell_cmd_is_allowed(const char **cmd, char **safecmd, char **cmdname)
{
    char **p;
    char *buf;
    char *s, *d;
    const char *ss;
    int pre;
    unsigned spaces;
    int allow = 0;

    /* pre == 1 means that the previous character is a white space
       pre == 0 means that the previous character is not a white space */
    buf = (char *) xmalloc((unsigned) strlen(*cmd) + 1);
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
        for (ss = *cmd; *ss; ss++) {
            if (Isspace(*ss))
                spaces++;
        }

        /* allocate enough memory (too much?) */
#  ifdef WIN32
        *safecmd =
            (char *) xmalloc(2 * (unsigned) strlen(*cmd) + 3 + 2 * spaces);
#  else
        *safecmd = (char *) xmalloc((unsigned) strlen(*cmd) + 3 + 2 * spaces);
#  endif

        /* make a safe command line *safecmd */
        ss = *cmd;
        while (Isspace(*ss))
            ss++;
        d = *safecmd;
        while (!Isspace(*ss) && *ss)
            *d++ = *ss++;

        pre = 1;
        while (*ss) {
            /* Quotation given by a user.  " should always be used; we
               transform it below.  On Unix, if ' is used, simply immediately
               return a quotation error.  */
            if (*ss == '\'') {
                return -1;
            }

            if (*ss == '"') {
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
                ss++;

                while (*ss != '"') {
                    /* Illegal use of ', or closing quotation mark is missing */
                    if (*ss == '\'' || *ss == '\0')
                        return -1;
#  ifdef WIN32
                    if (char_needs_quote(*ss))
                        *d++ = '^';
#  endif
                    *d++ = *ss++;
                }

                /* Closing quotation mark will be output afterwards, so
                   we do nothing here */
                ss++;

                /* The character after the closing quotation mark
                   should be a white space or NULL */
                if (!Isspace(*ss) && *ss)
                    return -1;

                /* Beginning of a usual argument */
            } else if (pre == 1 && !Isspace(*ss)) {
                pre = 0;
                *d++ = QUOTE;
#  ifdef WIN32
                if (char_needs_quote(*ss))
                    *d++ = '^';
#  endif
                *d++ = *ss++;
                /* Ending of a usual argument */

            } else if (pre == 0 && Isspace(*ss)) {
                pre = 1;
                /* Closing quotation mark */
                *d++ = QUOTE;
                *d++ = *ss++;
            } else {
                /* Copy a character from *cmd to *safecmd. */
#  ifdef WIN32
                if (char_needs_quote(*ss))
                    *d++ = '^';
#  endif
                *d++ = *ss++;
            }
        }
        /* End of the command line */
        if (pre == 0) {
            *d++ = QUOTE;
        }
        *d = '\0';
#ifdef WIN32
        {
          char *p, *q, *r;
          p = *safecmd;
          if (!(IS_DIR_SEP (p[0]) && IS_DIR_SEP (p[1])) &&
              !(p[1] == ':' && IS_DIR_SEP (p[2]))) { 
            p = (char *) kpse_var_value ("SELFAUTOLOC");
            if (p) {
              r = *safecmd;
              while (*r && !Isspace(*r))
                r++;
              if (*r == '\0')
                q = concatn ("\"", p, "/", *safecmd, "\"", NULL);
              else {
                *r = '\0';
                r++;
                while (*r && Isspace(*r))
                  r++;
                if (*r)
                  q = concatn ("\"", p, "/", *safecmd, "\" ", r, NULL);
                else
                  q = concatn ("\"", p, "/", *safecmd, "\"", NULL);
              }
              free (p);
              free (*safecmd);
              *safecmd = q;
            }
          }
        }
#endif
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
    if (restrictedshell == 0) {
        allow = 1;
    } else {
        const char *thecmd = cmd;
        allow = shell_cmd_is_allowed(&thecmd, &safecmd, &cmdname);
    }

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

#endif


/* What we were invoked as and with.  */
char **argv;
int argc;

/* The C version of what might wind up in |TEX_format_default|.  */
string dump_name;

/* The C version of the jobname, if given. */
const_string c_job_name;

const char *ptexbanner;

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
    main_body();

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
                buffer[k++] = (packed_ASCII_code) * (ptr++);
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

# ifdef WIN32
#  include <winsock2.h>
# else
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
# endif /* !WIN32 */

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
#ifdef WIN32
  u_long mode = 1;
#endif
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
        if (connect(sock, ipc_addr, ipc_addr_len) != 0 ||
#ifdef WIN32
        ioctlsocket (sock, FIONBIO, &mode) < 0
#else
        fcntl (sock, F_SETFL, O_NONBLOCK) < 0
#endif
           ) {
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
   if a new DVI file is starting.  This is the routine called by TeX.  */
void ipcpage(int is_eof)
{
    static boolean begun = false;
    unsigned len = 0;
    string p = NULL;

    if (!begun) {
        string name;            /* Just the filename.  */
        string cwd = xgetcwd();

        ipc_open_out();

        /* Have to pass whole filename to the other end, since it may have
           been started up and running as a daemon, e.g., as with the NeXT
           preview program.  */
	name = static_pdf->file_name;
        p = concat3(cwd, DIR_SEP_STRING, name);
        free(cwd);
        len = strlen(p);
        begun = true;
    }
    ipc_snd(len, is_eof, p);

    if (p) {
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
    string ret = (string) xmalloc((unsigned) strlen(name) + 3);
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


/* All our interrupt handler has to do is set TeX's or Metafont's global
   variable `interrupt'; then they will do everything needed.  */
#ifdef WIN32
/* Win32 doesn't set SIGINT ... */
static BOOL WINAPI catch_interrupt(DWORD arg)
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

void get_date_and_time(int *minutes, int *day, int *month, int *year)
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
void get_seconds_and_micros(int *seconds, int *micros)
{
#if defined (HAVE_GETTIMEOFDAY)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *seconds = (int)tv.tv_sec;
    *micros = (int)tv.tv_usec;
#elif defined (HAVE_FTIME)
    struct timeb tb;
    ftime(&tb);
    *seconds = tb.time;
    *micros = tb.millitm * 1000;
#else
    time_t myclock = time((time_t *) NULL);
    *seconds = (int) myclock;
    *micros = 0;
#endif
}

/*
  Generating a better seed numbers
  */
int getrandomseed(void)
{
#if defined (HAVE_GETTIMEOFDAY)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int)(tv.tv_usec + 1000000 * tv.tv_usec);
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
        buffer[last++] = (packed_ASCII_code) i;

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

#  define SWAP(x, y) do { temp = x; x = y; y = temp; } while (0)


/* Make the NITEMS items pointed at by P, each of size SIZE, be the
   opposite-endianness of whatever they are now.  */

void swap_items(char *pp, int nitems, int size)
{
    char temp;
    unsigned total = (unsigned) (nitems * size);
    char *q = xmalloc(total);
    char *p = q;
    memcpy(p,pp,total);
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

    case 12:
        while (nitems--) {
            SWAP(p[0], p[11]);
            SWAP(p[1], p[10]);
            SWAP(p[2], p[9]);
            SWAP(p[3], p[8]);
            SWAP(p[4], p[7]);
            SWAP(p[5], p[6]);
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
    memcpy(pp,q,total);
    xfree(q); 
}
#endif                          /* not WORDS_BIGENDIAN and not NO_DUMP_SHARE */



/* Get the job name to be used, which may have been set from the
   command line. */
str_number getjobname(str_number name)
{
    str_number ret = name;
    if (c_job_name != NULL)
        ret = maketexstring(c_job_name);
    return ret;
}
