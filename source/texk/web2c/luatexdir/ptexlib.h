/* ptexlib.h

   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
   Copyright 2006-2009 Taco Hoekwater <taco@luatex.org>

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

#ifndef PTEXLIB_H
#  define PTEXLIB_H

/* WEB2C macros and prototypes */
#  define EXTERN extern
#  include "luatex.h"

#  ifdef MSVC
extern double rint(double x);
#  endif

/* Replicate these here. They are hardcoded anyway */

#  define eTeX_version_string "2.2"     /* current \eTeX\ version */
#  define eTeX_version 2        /* \.{\\eTeXversion}  */
#  define eTeX_minor_version 2  /* \.{\\eTeXminorversion}  */
#  define eTeX_revision ".2"    /* \.{\\eTeXrevision} */

#  define Omega_version_string "1.15"   /* \.{\\OmegaVersion}  */
#  define Omega_version 1       /* \.{\\Omegaversion} */
#  define Omega_minor_version 15        /* \.{\\Omegaminorversion} */
#  define Omega_revision ".15"  /* \.{\\Omegarevision} */

#  define Aleph_version_string "0.0"    /* \.{\\AlephVersion} */
#  define Aleph_version 0       /* \.{\\Alephversion}  */
#  define Aleph_minor_version 0 /* \.{\\Alephminorversion} */
#  define Aleph_revision ".0"   /* \.{\\Alephrevision} */

#  define pdftex_version_string "-2.00.0"
                                        /* current \pdfTeX\ version */
#  define pdftex_version 200    /* \.{\\pdftexversion} */
#  define pdftex_revision "0"   /* \.{\\pdftexrevision} */

#  include <../lua51/lua.h>


/* pdftexlib macros from ptexmac.h */

/* Not all systems define it. */
#  ifndef M_PI
#    define M_PI           3.14159265358979323846       /* pi */
#  endif

#  ifdef WIN32
#    define inline __inline
#  endif

/**********************************************************************/
/* Pascal WEB macros */

#  define max_integer      0x7FFFFFFF
#  define max_dimen        0x3FFFFFFF

/**********************************************************************/

#  define PRINTF_BUF_SIZE     1024
#  define MAX_CSTRING_LEN     1024 * 1024
#  define MAX_PSTRING_LEN     1024
#  define SMALL_BUF_SIZE      256
#  define SMALL_ARRAY_SIZE    256

#  define check_buf(size, buf_size)                                 \
  if ((unsigned)(size) > (unsigned)(buf_size))                      \
    pdftex_fail("buffer overflow: %d > %d at file %s, line %d",     \
                (int)(size), (int)(buf_size), __FILE__,  __LINE__ )

#  define append_char_to_buf(c, p, buf, buf_size) do { \
    if (c == 9)                                        \
        c = 32;                                        \
    if (c == 13 || c == EOF)                           \
        c = 10;                                        \
    if (c != ' ' || (p > buf && p[-1] != 32)) {        \
        check_buf(p - buf + 1, (buf_size));            \
        *p++ = c;                                      \
    }                                                  \
} while (0)

#  define append_eol(p, buf, buf_size) do {            \
    check_buf(p - buf + 2, (buf_size));                \
    if (p - buf > 1 && p[-1] != 10)                    \
        *p++ = 10;                                     \
    if (p - buf > 2 && p[-2] == 32) {                  \
        p[-2] = 10;                                    \
        p--;                                           \
    }                                                  \
    *p = 0;                                            \
} while (0)

#  define remove_eol(p, buf) do {                      \
    p = strend(buf) - 1;                               \
    if (*p == 10)                                      \
        *p = 0;                                        \
} while (0)

#  define skip(p, c)   if (*p == c)  p++

#  define alloc_array(T, n, s) do {                    \
    if (T##_array == NULL) {                           \
        T##_limit = (s);                               \
        if ((unsigned)(n) > T##_limit)                 \
            T##_limit = (n);                           \
        T##_array = xtalloc(T##_limit, T##_entry);     \
        T##_ptr = T##_array;                           \
    }                                                  \
    else if ((unsigned)(T##_ptr - T##_array + (n)) > (unsigned)(T##_limit)) { \
        size_t last_ptr_index = T##_ptr - T##_array;          \
        T##_limit *= 2;                                \
        if ((unsigned)(T##_ptr - T##_array + (n)) > (unsigned)(T##_limit)) \
            T##_limit = T##_ptr - T##_array + (n);     \
        xretalloc(T##_array, T##_limit, T##_entry);    \
        T##_ptr = T##_array + last_ptr_index;          \
    }                                                  \
} while (0)

#  define define_array(T)                   \
T##_entry      *T##_ptr, *T##_array = NULL; \
size_t          T##_limit

#  define xfree(p)            do { if (p != NULL) free(p); p = NULL; } while (0)
#  define strend(s)           strchr(s, 0)
#  define xtalloc             XTALLOC
#  define xretalloc           XRETALLOC

#  define cmp_return(a, b) \
    if ((a) > (b))         \
        return 1;          \
    if ((a) < (b))         \
        return -1

#  define str_prefix(s1, s2)  (strncmp((s1), (s2), strlen(s2)) == 0)

/* that was ptexmac.h */

#  include "tex/mainbody.h"
#  include "tex/expand.h"
#  include "tex/conditional.h"
#  include "pdf/pdftypes.h"

/* synctex */
#  include "utils/synctex.h"

#  include "utils/avlstuff.h"
#  include "utils/managed-sa.h"
#  include "image/writeimg.h"
#  include "openbsd-compat.h"
#  include "dvi/dvigen.h"
#  include "pdf/pagetree.h"
#  include "pdf/pdfgen.h"
#  include "pdf/pdfpage.h"
#  include "pdf/pdftables.h"

#  include "pdf/pdfaction.h"
#  include "pdf/pdfannot.h"
#  include "pdf/pdfcolorstack.h"
#  include "pdf/pdfdest.h"
#  include "pdf/pdffont.h"
#  include "pdf/pdfglyph.h"
#  include "pdf/pdfimage.h"
#  include "pdf/pdflink.h"
#  include "pdf/pdflistout.h"
#  include "pdf/pdfliteral.h"
#  include "pdf/pdfobj.h"
#  include "pdf/pdfoutline.h"
#  include "pdf/pdfrule.h"
#  include "pdf/pdfsaverestore.h"
#  include "pdf/pdfsetmatrix.h"
#  include "pdf/pdfshipout.h"
#  include "pdf/pdfthread.h"
#  include "pdf/pdfxform.h"

#  include "lua/luagen.h"

#  include "font/luatexfont.h"
#  include "font/mapfile.h"
#  include "utils/utils.h"
#  include "image/writejbig2.h"

#  include "ocp/ocp.h"
#  include "ocp/ocplist.h"
#  include "ocp/runocp.h"
#  include "ocp/readocp.h"

#  include "lang/texlang.h"

#  include "tex/textcodes.h"
#  include "tex/mathcodes.h"

#  include "tex/align.h"
#  include "tex/directions.h"
#  include "tex/errors.h"
#  include "tex/inputstack.h"
#  include "tex/stringpool.h"
#  include "tex/textoken.h"
#  include "tex/printing.h"
#  include "tex/texfileio.h"
#  include "tex/arithmetic.h"
#  include "tex/nesting.h"
#  include "tex/packaging.h"
#  include "tex/linebreak.h"
#  include "tex/postlinebreak.h"
#  include "tex/scanning.h"
#  include "tex/buildpage.h"
#  include "tex/maincontrol.h"
#  include "tex/dumpdata.h"
#  include "tex/mainbody.h"
#  include "tex/extensions.h"
#  include "tex/texnodes.h"

#  include "tex/texmath.h"
#  include "tex/mlist.h"
#  include "tex/primitive.h"
#  include "tex/commands.h"
#  include "tex/equivalents.h"

/**********************************************************************/

#  include "tex/filename.h"

/* lua/luainit.c */
extern void write_svnversion(char *a);

/**********************************************************************/

extern halfword new_ligkern(halfword head, halfword tail);
extern halfword handle_ligaturing(halfword head, halfword tail);
extern halfword handle_kerning(halfword head, halfword tail);

halfword lua_hpack_filter(halfword head_node, scaled size, int pack_type,
                          int extrainfo, int d);
void lua_node_filter(int filterid, int extrainfo, halfword head_node,
                     halfword * tail_node);
halfword lua_vpack_filter(halfword head_node, scaled size, int pack_type,
                          scaled maxd, int extrainfo, int d);
void lua_node_filter_s(int filterid, char *extrainfo);
int lua_linebreak_callback(int is_broken, halfword head_node,
                           halfword * new_head);

void lua_pdf_literal(PDF pdf, int i);
void copy_pdf_literal(pointer r, pointer p);
void free_pdf_literal(pointer p);
void show_pdf_literal(pointer p);

void load_tex_patterns(int curlang, halfword head);
void load_tex_hyphenation(int curlang, halfword head);

/* lua/llualib.c */

void dump_luac_registers(void);
void undump_luac_registers(void);

/* lua/ltexlib.c */
void luacstring_start(int n);
void luacstring_close(int n);
int luacstring_cattable(void);
int luacstring_input(void);
int luacstring_partial(void);
int luacstring_final_line(void);

/* lua/luatoken.c */
void do_get_token_lua(int callback_id);

/* lua/luanode.c */
int visible_last_node_type(int n);
void print_node_mem_stats(void);

/* lua/limglib.c */
void vf_out_image(PDF pdf, unsigned i);

/* lua/ltexiolib.c */
void flush_loggable_info(void);

/* lua/luastuff.c */
void luatokencall(int p, int nameptr);
extern void late_lua(PDF pdf, halfword p);

extern void check_texconfig_init(void);

scaled divide_scaled(scaled s, scaled m, int dd);
scaled divide_scaled_n(double s, double m, double d);

#  include "tex/texdeffont.h"

/* lcallbacklib.c */

typedef enum {
    find_write_file_callback = 1,
    find_output_file_callback,
    find_image_file_callback,
    find_format_file_callback,
    find_read_file_callback, open_read_file_callback,
    find_ocp_file_callback, read_ocp_file_callback,
    find_vf_file_callback, read_vf_file_callback,
    find_data_file_callback, read_data_file_callback,
    find_font_file_callback, read_font_file_callback,
    find_map_file_callback, read_map_file_callback,
    find_enc_file_callback, read_enc_file_callback,
    find_type1_file_callback, read_type1_file_callback,
    find_truetype_file_callback, read_truetype_file_callback,
    find_opentype_file_callback, read_opentype_file_callback,
    find_sfd_file_callback, read_sfd_file_callback,
    find_pk_file_callback, read_pk_file_callback,
    show_error_hook_callback,
    process_input_buffer_callback, process_output_buffer_callback,
    start_page_number_callback, stop_page_number_callback,
    start_run_callback, stop_run_callback,
    define_font_callback,
    token_filter_callback,
    pre_output_filter_callback,
    buildpage_filter_callback,
    hpack_filter_callback, vpack_filter_callback,
    char_exists_callback,
    hyphenate_callback,
    ligaturing_callback,
    kerning_callback,
    pre_linebreak_filter_callback,
    linebreak_filter_callback,
    post_linebreak_filter_callback,
    mlist_to_hlist_callback,
    total_callbacks
} callback_callback_types;

extern int callback_set[];
extern int lua_active;

#  define callback_defined(a) callback_set[a]

extern int run_callback(int i, char *values, ...);
extern int run_saved_callback(int i, char *name, char *values, ...);
extern int run_and_save_callback(int i, char *values, ...);
extern void destroy_saved_callback(int i);
extern boolean get_callback(lua_State * L, int i);

extern void get_saved_lua_boolean(int i, char *name, boolean * target);
extern void get_saved_lua_number(int i, char *name, int *target);
extern void get_saved_lua_string(int i, char *name, char **target);

extern void get_lua_boolean(char *table, char *name, boolean * target);
extern void get_lua_number(char *table, char *name, int *target);
extern void get_lua_string(char *table, char *name, char **target);

extern char *get_lua_name(int i);

/* Additions to texmfmp.h for pdfTeX */

/* mark a char in font */
#  define pdf_mark_char(f,c) set_char_used(f,c,true)

/* test whether a char in font is marked */
#  define pdf_char_marked char_used

#  define pdfassert assert
#  define voidcast(a) (void *)(a)
#  define varmemcast(a) (memory_word *)(a)
#  define fixmemcast(a) (smemory_word *)(a)
extern volatile memory_word *varmem;
extern halfword var_mem_min;
extern halfword var_mem_max;
extern halfword get_node(int s);
extern void free_node(halfword p, int s);
extern void init_node_mem(int s);
extern void dump_node_mem(void);
extern void undump_node_mem(void);

extern void do_vf(internal_font_number tmp_f);

/* This routine has to return four values.  */
#  define	dateandtime(i,j,k,l) get_date_and_time (&(i), &(j), &(k), &(l))
extern void get_date_and_time(int *, int *, int *, int *);

/* Get high-res time info. */
#  define seconds_and_micros(i,j) get_seconds_and_micros (&(i), &(j))
extern void get_seconds_and_micros(int *, int *);

/* This routine has to return a scaled value. */
extern int getrandomseed(void);

/* Copy command-line arguments into the buffer, despite the name.  */
extern void topenin(void);

/* Can't prototype this since it uses poolpointer and ASCIIcode, which
   are defined later in mfd.h, and mfd.h uses stuff from here.  */
/* Therefore the department of ugly hacks decided to move this declaration
   to the *coerce.h files. */
/* extern void calledit (); */

/* Set an array size from texmf.cnf.  */
extern void setupboundvariable(int *, const_string, int);

/* here  are a few functions that used to be in coerce.h */

extern str_number getjobname(str_number);
extern str_number makefullnamestring();

#endif                          /* PTEXLIB_H */
