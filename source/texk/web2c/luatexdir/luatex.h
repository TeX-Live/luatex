/* luatex.h
   
   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
   Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>

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

#ifndef LUATEX_H
#  define LUATEX_H

/* texmf.h: Main include file for TeX and Metafont in C. This file is
   included by {tex,mf}d.h, which is the first include in the C files
   output by web2c.  */

#include "cpascal.h"

#include <kpathsea/c-pathch.h> /* for IS_DIR_SEP, used in the change files */
#include <kpathsea/tex-make.h> /* for kpse_make_tex_discard_errors */

/* If we have these macros, use them, as they provide a better guide to
   the endianess when cross-compiling. */
#if defined (BYTE_ORDER) && defined (BIG_ENDIAN) && defined (LITTLE_ENDIAN)
#ifdef WORDS_BIGENDIAN
#undef WORDS_BIGENDIAN
#endif
#if BYTE_ORDER == BIG_ENDIAN
#define WORDS_BIGENDIAN
#endif
#endif
/* More of the same, but now NeXT-specific. */
#ifdef NeXT
#ifdef WORDS_BIGENDIAN
#undef WORDS_BIGENDIAN
#endif
#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN
#endif
#endif


/* Some things are the same except for the name.  */

#define TEXMFPOOLNAME "luatex.pool"
#define TEXMFENGINENAME "luatex"

#define DUMP_FILE fmt_file
#define DUMP_FORMAT kpse_fmt_format
#define write_dvi WRITE_OUT
#define flush_dvi flush_out
#define OUT_FILE dvi_file
#define OUT_BUF dvi_buf

/* Restore underscores.  */
#define kpsetexformat kpse_tex_format
#define mainbody main_body
#define t_open_in topenin

/* Hacks for TeX that are better not to #ifdef, see texmfmp.c.  */
extern int tfmtemp, texinputtype;

/* TeX, MF and MetaPost use this.  */
extern boolean openinnameok (const_string);
extern boolean openoutnameok (const_string);

/* pdfTeX uses these for pipe support */
extern boolean open_in_or_pipe (FILE **, int, const_string fopen_mode);
extern boolean open_out_or_pipe (FILE **, const_string fopen_mode);
extern void close_file_or_pipe (FILE *);

/* Executing shell commands.  */
extern void mk_shellcmdlist (char *);
extern void init_shell_escape (void);
extern int shell_cmd_is_allowed (char **cmd, char **safecmd, char **cmdname);
extern int runsystem (char *cmd);

#ifndef GLUERATIO_TYPE
#define GLUERATIO_TYPE double
#endif
typedef GLUERATIO_TYPE glueratio;

#if defined(__DJGPP__) && defined (IPC)
#undef IPC
#endif

#ifdef IPC
extern void ipcpage (int);
#endif /* IPC */


/* How to output to the GF or DVI file.  */
#define	WRITE_OUT(a, b)							\
  if (fwrite ((char *) &OUT_BUF[a], sizeof (OUT_BUF[a]),		\
                 (int) ((b) - (a) + 1), OUT_FILE) 			\
      != (int) ((b) - (a) + 1))						\
    FATAL_PERROR ("fwrite");

#define flush_out() fflush (OUT_FILE)

/* Read a line of input as quickly as possible.  */
#define	input_ln(stream, flag) input_line (stream)

extern boolean input_line (FILE *);

#include <luatexdir/ptexlib.h>

#define BANNER "This is LuaTeX, Version LUATEX-VERSION"
#define COPYRIGHT_HOLDER "Taco Hoekwater"
#define AUTHOR NULL
#define PROGRAM_HELP LUATEXHELP
#define BUG_ADDRESS "dev-luatex@ntg.nl"
#define DUMP_VAR TEX_format_default
#define DUMP_LENGTH_VAR format_default_length
#define DUMP_OPTION "fmt"
#define DUMP_EXT ".fmt"
#define INPUT_FORMAT kpse_tex_format
#define INI_PROGRAM "luainitex"
#define VIR_PROGRAM "luavirtex"
#define TEXMFENGINENAME "luatex"

/* this counteracts the macro definition in cpascal.h */
#undef Xchr
#define Xchr(a) a

#endif
