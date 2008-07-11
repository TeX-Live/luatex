% $Id: mpost.w 619 2008-07-09 15:53:44Z taco $
%
% Copyright 2008 Taco Hoekwater.
%
% This program is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 2 of the License, or
% (at your option) any later version.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with this program.  If not, see <http://www.gnu.org/licenses/>.

\font\tenlogo=logo10 % font used for the METAFONT logo
\def\MP{{\tenlogo META}\-{\tenlogo POST}}

\def\title{MetaPost executable}
\def\[#1]{#1.}
\pdfoutput=1

@* \[1] Metapost executable.

Now that all of \MP\ is a library, a separate program is needed to 
have our customary command-line interface. 

@ First, here are the C includes. |avl.h| is needed because of an 
|avl_allocator| that is defined in |mplib.h|

@d true 1
@d false 0
 
@c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mplib.h>
#include <mpxout.h>
#ifdef WIN32
#include <process.h>
#endif
#include <kpathsea/kpathsea.h>
extern char *kpathsea_version_string;
extern string kpse_program_name;
@= /*@@null@@*/ @> static char *mpost_tex_program = NULL;
static int debug = 0; /* debugging for makempx */

@ Allocating a bit of memory, with error detection:

@d mpost_xfree(A) do { if (A!=NULL) free(A); A=NULL; } while (0)

@c
@= /*@@only@@*/ /*@@out@@*/ @> static void  *mpost_xmalloc (size_t bytes) {
  void *w = malloc (bytes); 
  if (w==NULL) {
    fprintf(stderr,"Out of memory!\n");
    exit(EXIT_FAILURE);
  }
  return w;
}
@= /*@@only@@*/ @> static char *mpost_xstrdup(const char *s) {
  char *w; 
  w = strdup(s);
  if (w==NULL) {
    fprintf(stderr,"Out of memory!\n");
    exit(EXIT_FAILURE);
  }
  return w;
}
static char *mpost_itoa (int i) {
  char res[32] ;
  unsigned idx = 30;
  unsigned v = (unsigned)abs(i);
  memset(res,0,32*sizeof(char));
  while (v>=10) {
    char d = (char)(v % 10);
    v = v / 10;
    res[idx--] = d;
  }
  res[idx--] = (char)v;
  if (i<0) {
      res[idx--] = '-';
  }
  return mpost_xstrdup(res+idx);
}


@ @c
static void mpost_run_editor (MP mp, char *fname, int fline) {
  char *temp, *command, *edit_value;
  char c;
  boolean sdone, ddone;
  sdone = ddone = false;
  edit_value = kpse_var_value ("MPEDIT");
  if (edit_value == NULL)
    edit_value = getenv("EDITOR");
  if (edit_value == NULL) {
    fprintf (stderr,"call_edit: can't find a suitable MPEDIT or EDITOR variable\n");
    exit(mp_status(mp));    
  }
  command = (string) mpost_xmalloc (strlen (edit_value) + strlen(fname) + 11 + 3);
  temp = command;
  while ((c = *edit_value++) != (char)0) {
      if (c == '%')   {
        switch (c = *edit_value++) {
	  case 'd':
	    if (ddone) {
              fprintf (stderr,"call_edit: `%%d' appears twice in editor command\n");
              exit(EXIT_FAILURE);  
            } else {
              char *s = mpost_itoa(fline);
              if (s != NULL) {
                while (*s != '\0')
	          *temp++ = *s++;
                free(s);
              }
              ddone = true;
            }
            break;
	  case 's':
            if (sdone) {
              fprintf (stderr,"call_edit: `%%s' appears twice in editor command\n");
              exit(EXIT_FAILURE);
            } else {
              while (*fname != '\0')
		*temp++ = *fname++;
              *temp++ = '.';
	      *temp++ = 'm';
	      *temp++ = 'p';
              sdone = true;
            }
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
     } else {
     	*temp++ = c;
     }
   }
  *temp = '\0';
  if (system (command) != 0)
    fprintf (stderr, "! Trouble executing `%s'.\n", command);
  exit(EXIT_FAILURE);
}

@ 
@<Register the callback routines@>=
options->run_editor = mpost_run_editor;

@
@c 
static string normalize_quotes (const char *name, const char *mesg) {
    boolean quoted = false;
    boolean must_quote = (strchr(name, ' ') != NULL);
    /* Leave room for quotes and NUL. */
    string ret = (string)mpost_xmalloc(strlen(name)+3);
    string p;
    const_string q;
    p = ret;
    if (must_quote)
        *p++ = '"';
    for (q = name; *q != '\0'; q++) {
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
        exit(EXIT_FAILURE);
    }
    return ret;
}

@ @c 
@= /*@@null@@*/ @> static char *makempx_find_file (MPX mpx, const char *nam, 
                                                   const char *mode, int ftype) {
  int fmt;
  boolean req;
  (void) mpx;
  if (mode[0] != 'r') { 
     return strdup(nam);
  }
  req = true; fmt = -1;
  switch(ftype) {
  case mpx_tfm_format:       fmt = kpse_tfm_format; break;
  case mpx_vf_format:        fmt = kpse_vf_format; req = false; break;
  case mpx_trfontmap_format: fmt = kpse_mpsupport_format; break;
  case mpx_trcharadj_format: fmt = kpse_mpsupport_format; break;
  case mpx_desc_format:      fmt = kpse_troff_font_format; break;
  case mpx_fontdesc_format:  fmt = kpse_troff_font_format; break;
  case mpx_specchar_format:  fmt = kpse_mpsupport_format; break;
  }
  if (fmt<0) return NULL;
  return  kpse_find_file (nam, fmt, req);
}

@ Invoke makempx (or troffmpx) to make sure there is an up-to-date
   .mpx file for a given .mp file.  (Original from John Hobby 3/14/90) 

@d default_args " --parse-first-line --interaction=nonstopmode"
@d TEX     "tex"
@d TROFF   "soelim | eqn -Tps -d$$ | troff -Tps"

@c
#ifndef MPXCOMMAND
#define MPXCOMMAND "makempx"
#endif
static int mpost_run_make_mpx (MP mp, char *mpname, char *mpxname) {
  int ret;
  char *cnf_cmd = kpse_var_value ("MPXCOMMAND");
  
  if (cnf_cmd != NULL && (strcmp (cnf_cmd, "0")==0)) {
    /* If they turned off this feature, just return success.  */
    ret = 0;

  } else {
    /* We will invoke something. Compile-time default if nothing else.  */
    char *cmd;
    char *tmp = normalize_quotes(mpname, "mpname");
    char *qmpname = kpse_find_file (tmp,kpse_mp_format, true);
    char *qmpxname = normalize_quotes(mpxname, "mpxname");
    mpost_xfree(tmp);
    if (cnf_cmd!=NULL) {
      if (mp_troff_mode(mp)!=0)
        cmd = concatn (cnf_cmd, " -troff ",
                     qmpname, " ", qmpxname, NULL);
      else if (mpost_tex_program!=NULL && *mpost_tex_program != '\0')
        cmd = concatn (cnf_cmd, " -tex=", mpost_tex_program, " ",
                     qmpname, " ", qmpxname, NULL);
      else
        cmd = concatn (cnf_cmd, " -tex ", qmpname, " ", qmpxname, NULL);
  
      /* Run it.  */
      ret = system (cmd);
      free (cmd);
      mpost_xfree(qmpname);
      mpost_xfree(qmpxname);
    } else {
      mpx_options * mpxopt;
      char *s = NULL;
      char *maincmd = NULL;
      int mpxmode = mp_troff_mode(mp);
      char *mpversion = mp_metapost_version () ;
      mpxopt = mpost_xmalloc(sizeof(mpx_options));
      if (mpost_tex_program != NULL && *mpost_tex_program != '\0') {
        maincmd = mpost_xstrdup(mpost_tex_program);
      } else {
        if (mpxmode == mpx_tex_mode) {
          s = kpse_var_value("TEX");
          if (s==NULL) s = kpse_var_value("MPXMAINCMD");
          if (s==NULL) s = mpost_xstrdup (TEX);
          maincmd = (char *)mpost_xmalloc (strlen(s)+strlen(default_args)+1);
          strcpy(maincmd,s);
          strcat(maincmd,default_args);
          free(s);
        } else {
          s = kpse_var_value("TROFF");
          if (s==NULL) s = kpse_var_value("MPXMAINCMD");
          if (s==NULL) s = mpost_xstrdup (TROFF);
          maincmd = s;
        }
      }
      mpxopt->mode = mpxmode;
      mpxopt->cmd  = maincmd;
      mpxopt->mptexpre = kpse_var_value("MPTEXPRE");
      mpxopt->debug = debug;
      mpxopt->mpname = qmpname;
      mpxopt->mpxname = qmpxname;
      mpxopt->find_file = makempx_find_file;
      {
        char *banner = "% Written by metapost version ";
        mpxopt->banner = mpost_xmalloc(strlen(mpversion)+strlen(banner)+1);
        strcpy (mpxopt->banner, banner);
        strcat (mpxopt->banner, mpversion);
      }
      ret = mpx_makempx(mpxopt);
      mpost_xfree(mpxopt->cmd);
      mpost_xfree(mpxopt->mptexpre);
      mpost_xfree(mpxopt->banner);
      mpost_xfree(mpxopt->mpname);
      mpost_xfree(mpxopt->mpxname);
      mpost_xfree(mpxopt);
      mpost_xfree(mpversion);
    }
  }

  mpost_xfree (cnf_cmd);
  return (int)(ret == 0);
}

@ 
@<Register the callback routines@>=
if (!nokpse)
  options->run_make_mpx = mpost_run_make_mpx;


@ @c 
static int get_random_seed (void) {
  int ret = 0;
#if defined (HAVE_GETTIMEOFDAY)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  ret = (tv.tv_usec + 1000000 * tv.tv_usec);
#elif defined (HAVE_FTIME)
  struct timeb tb;
  ftime(&tb);
  ret = (tb.millitm + 1000 * tb.time);
#else
  time_t clock = time ((time_t*)NULL);
  struct tm *tmptr = localtime(&clock);
  if (tmptr!=NULL)
    ret = (tmptr->tm_sec + 60*(tmptr->tm_min + 60*tmptr->tm_hour));
#endif
  return ret;
}

@ @<Register the callback routines@>=
options->random_seed = get_random_seed();

@ @c 
static char *mpost_find_file(MP mp, const char *fname, const char *fmode, int ftype)  {
  size_t l ;
  char *s;
  (void)mp;
  s = NULL;
  if (fmode[0]=='r') {
	if (ftype>=mp_filetype_text) {
      s = kpse_find_file (fname, kpse_mp_format, 0); 
    } else {
    switch(ftype) {
    case mp_filetype_program: 
      l = strlen(fname);
   	  if (l>3 && strcmp(fname+l-3,".mf")==0) {
   	    s = kpse_find_file (fname, kpse_mf_format, 0); 
      } else {
   	    s = kpse_find_file (fname, kpse_mp_format, 0); 
      }
      break;
    case mp_filetype_memfile: 
      s = kpse_find_file (fname, kpse_mem_format, 0); 
      break;
    case mp_filetype_metrics: 
      s = kpse_find_file (fname, kpse_tfm_format, 0); 
      break;
    case mp_filetype_fontmap: 
      s = kpse_find_file (fname, kpse_fontmap_format, 0); 
      break;
    case mp_filetype_font: 
      s = kpse_find_file (fname, kpse_type1_format, 0); 
      break;
    case mp_filetype_encoding: 
      s = kpse_find_file (fname, kpse_enc_format, 0); 
      break;
    }
    }
  } else {
    if (fname!=NULL)
      s = mpost_xstrdup(fname); /* when writing */
  }
  return s;
}

@  @<Register the callback routines@>=
if (!nokpse)
  options->find_file = mpost_find_file;

@ @c 
static void *mpost_open_file(MP mp, const char *fname, const char *fmode, int ftype)  {
  char realmode[3];
  char *s;
  if (ftype==mp_filetype_terminal) {
    return (fmode[0] == 'r' ? stdin : stdout);
  } else if (ftype==mp_filetype_error) {
    return stderr;
  } else { 
    s = mpost_find_file (mp, fname, fmode, ftype);
    if (s!=NULL) {
      void *ret = NULL;
      realmode[0] = *fmode;
	  realmode[1] = 'b';
	  realmode[2] = '\0';
      ret = (void *)fopen(s,realmode);
      free(s);
      return ret;
    }
  }
  return NULL;
}

@  @<Register the callback routines@>=
if (!nokpse)
  options->open_file = mpost_open_file;


@ At the moment, the command line is very simple.

@d option_is(A) ((strncmp(argv[a],"--" A, strlen(A)+2)==0) || 
       (strncmp(argv[a],"-" A, strlen(A)+1)==0))
@d option_arg(B) (optarg != NULL && strncmp(optarg,B, strlen(B))==0)


@<Read and set command line options@>=
{
  char *mpost_optarg;
  boolean ini_version_test = false;
  while (++a<argc) {
    mpost_optarg = strstr(argv[a],"=") ;
    if (mpost_optarg!=NULL) {
      mpost_optarg++;
      if (*mpost_optarg == '\0')  mpost_optarg=NULL;
    }
    if (option_is("ini")) {
      ini_version_test = true;
    } else if (option_is("debug")) {
      debug = 1;
    } else if (option_is ("kpathsea-debug")) {
      if (mpost_optarg!=NULL)
        kpathsea_debug |= atoi (mpost_optarg);
    } else if (option_is("mem")) {
      if (mpost_optarg!=NULL) {
        mpost_xfree(options->mem_name);
        options->mem_name = mpost_xstrdup(mpost_optarg);
        if (user_progname == NULL) 
	    user_progname = mpost_optarg;
      }
    } else if (option_is("jobname")) {
      if (mpost_optarg!=NULL) {
        mpost_xfree(options->job_name);
        options->job_name = mpost_xstrdup(mpost_optarg);
      }
    } else if (option_is ("progname")) {
      user_progname = mpost_optarg;
    } else if (option_is("troff")) {
      options->troff_mode = (int)true;
    } else if (option_is ("tex")) {
      mpost_tex_program = mpost_optarg;
    } else if (option_is("interaction")) {
      if (option_arg("batchmode")) {
        options->interaction = mp_batch_mode;
      } else if (option_arg("nonstopmode")) {
        options->interaction = mp_nonstop_mode;
      } else if (option_arg("scrollmode")) {
        options->interaction = mp_scroll_mode;
      } else if (option_arg("errorstopmode")) {
        options->interaction = mp_error_stop_mode;
      } else {
        fprintf(stdout,"unknown option argument %s\n", argv[a]);
      }
    } else if (option_is("no-kpathsea")) {
      nokpse=true;
    } else if (option_is("help")) {
      @<Show help and exit@>;
    } else if (option_is("version")) {
      @<Show version and exit@>;
    } else if (option_is("")) {
      continue; /* ignore unknown options */
    } else {
      break;
    }
  }
  options->ini_version = (int)ini_version_test;
}

@ 
@<Show help...@>=
{
fprintf(stdout,
"\n"
"Usage: mpost [OPTION] [&MEMNAME] [MPNAME[.mp]] [COMMANDS]\n"
"\n"
"  Run MetaPost on MPNAME, usually creating MPNAME.NNN (and perhaps\n"
"  MPNAME.tfm), where NNN are the character numbers generated.\n"
"  Any remaining COMMANDS are processed as MetaPost input,\n"
"  after MPNAME is read.\n\n");
fprintf(stdout,
"  If no arguments or options are specified, prompt for input.\n"
"\n"
"  -ini                      be inimpost, for dumping mem files\n"
"  -interaction=STRING       set interaction mode (STRING=batchmode/nonstopmode/\n"
"                            scrollmode/errorstopmode)\n"
"  -jobname=STRING           set the job name to STRING\n"
"  -progname=STRING          set program (and mem) name to STRING\n");
fprintf(stdout,
"  -tex=TEXPROGRAM           use TEXPROGRAM for text labels\n"
"  -kpathsea-debug=NUMBER    set path searching debugging flags according to\n"
"                            the bits of NUMBER\n"
"  -mem=MEMNAME or &MEMNAME  use MEMNAME instead of program name or a %%& line\n"
"  -troff                    set prologues:=1 and assume TEXPROGRAM is really troff\n"
"  -help                     display this help and exit\n"
"  -version                  output version information and exit\n"
"\n"
"Email bug reports to mp-implementors@@tug.org.\n"
"\n");
  exit(EXIT_SUCCESS);
}

@ 
@<Show version...@>=
{
  char *s = mp_metapost_version();
fprintf(stdout, 
"\n"
"MetaPost %s\n"
"Copyright 2008 AT&T Bell Laboratories.\n"
"There is NO warranty.  Redistribution of this software is\n"
"covered by the terms of both the MetaPost copyright and\n"
"the Lesser GNU General Public License.\n"
"For more information about these matters, see the file\n"
"named COPYING and the MetaPost source.\n"
"Primary author of MetaPost: John Hobby.\n"
"Current maintainer of MetaPost: Taco Hoekwater.\n"
"\n", s);
  mpost_xfree(s);
  exit(EXIT_SUCCESS);
}

@ The final part of the command line, after option processing, is
stored in the \MP\ instance, this will be taken as the first line of
input.

@d command_line_size 256

@<Copy the rest of the command line@>=
{
  mpost_xfree(options->command_line);
  options->command_line = mpost_xmalloc(command_line_size);
  strcpy(options->command_line,"");
  if (a<argc) {
    k=0;
    for(;a<argc;a++) {
      char *c = argv[a];
      while (*c != '\0') {
	    if (k<(command_line_size-1)) {
          options->command_line[k++] = *c;
        }
        c++;
      }
      options->command_line[k++] = ' ';
    }
	while (k>0) {
      if (options->command_line[(k-1)] == ' ') 
        k--; 
      else 
        break;
    }
    options->command_line[k] = '\0';
  }
}

@ A simple function to get numerical |texmf.cnf| values
@c
static int setup_var (int def, const char *var_name, boolean nokpse) {
  if (!nokpse) {
    char * expansion = kpse_var_value (var_name);
    if (expansion) {
      int conf_val = atoi (expansion);
      free (expansion);
      if (conf_val > 0) {
        return conf_val;
      }
    }
  }
  return def;
}

@ @<Set up the banner line@>=
{
  char * mpversion = mp_metapost_version () ;
  const char * banner = "This is MetaPost, version ";
  const char * kpsebanner_start = " (";
  const char * kpsebanner_stop = ")";
  mpost_xfree(options->banner);
  options->banner = mpost_xmalloc(strlen(banner)+
                            strlen(mpversion)+
                            strlen(kpsebanner_start)+
                            strlen(kpathsea_version_string)+
                            strlen(kpsebanner_stop)+1);
  strcpy (options->banner, banner);
  strcat (options->banner, mpversion);
  strcat (options->banner, kpsebanner_start);
  strcat (options->banner, kpathsea_version_string);
  strcat (options->banner, kpsebanner_stop);
  mpost_xfree(mpversion);
}

@ Precedence order is:

\item {} \.{-mem=MEMNAME} on the command line 
\item {} \.{\&MEMNAME} on the command line 
\item {} \.{\%\&MEM} as first line inside input file
\item {} \.{argv[0]} if all else fails

@<Discover the mem name@>=
{
  char *m = NULL; /* head of potential |mem_name| */
  char *n = NULL; /* a moving pointer */
  if (options->command_line != NULL && *(options->command_line) == '&'){
    m = mpost_xstrdup(options->command_line+1);
    n = m;
    while (*n != '\0' && *n != ' ') n++;
    while (*n == ' ') n++;
    if (*n != '\0') { /* more command line to follow */
      char *s = mpost_xstrdup(n);
      if (n>m) n--;
      while (*n == ' ' && n>m) n--;
      n++;
      *n ='\0'; /* this terminates |m| */
      mpost_xfree(options->command_line);
      options->command_line = s;
    } else { /* only \.{\&MEMNAME} on command line */
      if (n>m) n--;
      while (*n == ' ' && n>m) n--;
      n++;
      *n ='\0'; /* this terminates |m| */
      mpost_xfree(options->command_line);
    }
    if ( options->mem_name == NULL && *m != '\0') {
      mpost_xfree(options->mem_name); /* for lint only */
      options->mem_name = m;
    } else {
      mpost_xfree(m);
    }
  }
}
if ( options->mem_name == NULL ) {
  char *m = NULL; /* head of potential |job_name| */
  char *n = NULL; /* a moving pointer */
  if (options->command_line != NULL && *(options->command_line) != '\\'){
    m = mpost_xstrdup(options->command_line);
    n = m;
    while (*n != '\0' && *n != ' ') n++;
    if (n>m) {
      char *fname;
      *n='\0';
      fname = m;
      if (!nokpse)
        fname = kpse_find_file(m,kpse_mp_format,true);
      if (fname == NULL) {
        mpost_xfree(m);
      } else {
        FILE *F = fopen(fname,"r");
        if (F==NULL) {
          mpost_xfree(fname);
        } else {
          char *line = mpost_xmalloc(256);
          if (fgets(line,255,F) == NULL) {
            (void)fclose(F);
            mpost_xfree(fname);
            mpost_xfree(line);
          } else {
            (void)fclose(F);
            while (*line != '\0' && *line == ' ') line++;
            if (*line == '%') {
              n = m = line+1;
              while (*n != '\0' && *n == ' ') n++;
              if (*n == '&') {
                m = n+1;
                while (*n != '\0' && *n != ' ') n++;
                if (n>(m+1)) {
                  n--;
                  while (*n == ' ' && n>m) n--;
                  *n ='\0'; /* this terminates |m| */
                  options->mem_name = mpost_xstrdup(m);
                  mpost_xfree(fname);
                } else {
                  mpost_xfree(fname);
                  mpost_xfree(line);    
                }
              }
            }
          }
        }
      }
    } else {
      mpost_xfree(m);
    }
  }
}
if ( options->mem_name == NULL )
  if (kpse_program_name!=NULL)
    options->mem_name = mpost_xstrdup(kpse_program_name);


@ Now this is really it: \MP\ starts and ends here.

@c 
int main (int argc, char **argv) { /* |start_here| */
  int k; /* index into buffer */
  int history; /* the exit status */
  MP mp; /* a metapost instance */
  struct MP_options * options; /* instance options */
  int a=0; /* argc counter */
  boolean nokpse = false; /* switch to {\it not} enable kpse */
  char *user_progname = NULL; /* If the user overrides argv[0] with -progname.  */
  options = mp_options();
  options->ini_version       = (int)false;
  options->print_found_names = (int)true;
  @<Read and set command line options@>;
  @= /*@@-nullpass@@*/ @> 
  if (!nokpse)
    kpse_set_program_name("mpost", user_progname);  
  @= /*@@=nullpass@@*/ @> 
  if(putenv((char *)"engine=metapost"))
    fprintf(stdout,"warning: could not set up $engine\n");
  options->main_memory       = setup_var (50000,"main_memory",nokpse);
  options->hash_size         = (unsigned)setup_var (16384,"hash_size",nokpse);
  options->max_in_open       = setup_var (25,"max_in_open",nokpse);
  options->param_size        = setup_var (1500,"param_size",nokpse);
  options->error_line        = setup_var (79,"error_line",nokpse);
  options->half_error_line   = setup_var (50,"half_error_line",nokpse);
  options->max_print_line    = setup_var (100,"max_print_line",nokpse);
  @<Set up the banner line@>;
  @<Copy the rest of the command line@>;
  if (options->ini_version!=(int)true) {
    @<Discover the mem name@>;
  }
  @<Register the callback routines@>;
  mp = mp_initialize(options);
  mpost_xfree(options->command_line);
  mpost_xfree(options->mem_name);
  mpost_xfree(options->job_name);
  mpost_xfree(options->banner);
  free(options);
  if (mp==NULL)
	exit(EXIT_FAILURE);
  history = mp_status(mp);
  if (history!=0)
	exit(history);
  history = mp_run(mp);
  (void)mp_finish(mp);
  exit(history);
}

