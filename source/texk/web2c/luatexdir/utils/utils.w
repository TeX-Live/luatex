% utils.w

% Copyright 1996-2006 Han The Thanh <thanh@@pdftex.org>
% Copyright 2006-2010 Taco Hoekwater <taco@@luatex.org>

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
static const char _svn_version[] =
    "$Id$ "
    "$URL$";

@ @c
#include "openbsd-compat.h"
#include <kpathsea/config.h> /* this is a trick to load mingw32's io.h early,
				using a macro redefinition of |eof()|. */
#include "sys/types.h"
#ifndef __MINGW32__
#  include "sysexits.h"
#else
#  define EX_SOFTWARE 70
#endif
#include <kpathsea/c-stat.h>
#include <kpathsea/c-fopen.h>
#include <string.h>
#include <time.h>
#include <float.h>              /* for |DBL_EPSILON| */
#include "zlib.h"
#include "ptexlib.h"
#include "md5.h"

#include "lua/luatex-api.h"     /* for ptexbanner */

#include "png.h"

/* POPPLER_VERSION is defined in poppler-config.h for poppler from
 * the TeX Live tree, or in the Makefile for an installed version.  */
#include "poppler-config.h"

@ @c
#define check_nprintf(size_get, size_want) \
    if ((unsigned)(size_get) >= (unsigned)(size_want)) \
        pdftex_fail ("snprintf failed: file %s, line %d", __FILE__, __LINE__);

char *cur_file_name = NULL;
static char print_buf[PRINTF_BUF_SIZE];
int epochseconds;
int microseconds;

/* define |char_ptr|, |char_array|, and |char_limit| */
typedef char char_entry;
define_array(char);

@ @c
#define SUBSET_TAG_LENGTH 6
void make_subset_tag(fd_entry * fd)
{
    int i, j = 0, a[SUBSET_TAG_LENGTH];
    md5_state_t pms;
    char *glyph;
    glw_entry *glw_glyph;
    struct avl_traverser t;
    md5_byte_t digest[16];
    void **aa;
    static struct avl_table *st_tree = NULL;
    if (st_tree == NULL)
        st_tree = avl_create(comp_string_entry, NULL, &avl_xallocator);
    assert(fd != NULL);
    assert(fd->gl_tree != NULL);
    assert(fd->fontname != NULL);
    assert(fd->subset_tag == NULL);
    fd->subset_tag = xtalloc(SUBSET_TAG_LENGTH + 1, char);
    do {
        md5_init(&pms);
        avl_t_init(&t, fd->gl_tree);
        if (is_cidkeyed(fd->fm)) {      /* |glw_entry| items */
            for (glw_glyph = (glw_entry *) avl_t_first(&t, fd->gl_tree);
                 glw_glyph != NULL; glw_glyph = (glw_entry *) avl_t_next(&t)) {
                glyph = malloc(24);
                sprintf(glyph, "%05u%05u ", glw_glyph->id, glw_glyph->wd);
                md5_append(&pms, (md5_byte_t *) glyph, (int) strlen(glyph));
                free(glyph);
            }
        } else {
            for (glyph = (char *) avl_t_first(&t, fd->gl_tree); glyph != NULL;
                 glyph = (char *) avl_t_next(&t)) {
                md5_append(&pms, (md5_byte_t *) glyph, (int) strlen(glyph));
                md5_append(&pms, (const md5_byte_t *) " ", 1);
            }
        }
        md5_append(&pms, (md5_byte_t *) fd->fontname,
                   (int) strlen(fd->fontname));
        md5_append(&pms, (md5_byte_t *) & j, sizeof(int));      /* to resolve collision */
        md5_finish(&pms, digest);
        for (a[0] = 0, i = 0; i < 13; i++)
            a[0] += digest[i];
        for (i = 1; i < SUBSET_TAG_LENGTH; i++)
            a[i] = a[i - 1] - digest[i - 1] + digest[(i + 12) % 16];
        for (i = 0; i < SUBSET_TAG_LENGTH; i++)
            fd->subset_tag[i] = (char) (a[i] % 26 + 'A');
        fd->subset_tag[SUBSET_TAG_LENGTH] = '\0';
        j++;
        assert(j < 100);
    }
    while ((char *) avl_find(st_tree, fd->subset_tag) != NULL);
    aa = avl_probe(st_tree, fd->subset_tag);
    assert(aa != NULL);
    if (j > 2)
        pdftex_warn
            ("\nmake_subset_tag(): subset-tag collision, resolved in round %d.\n",
             j);
}

@ @c
__attribute__ ((format(printf, 1, 2)))
void tex_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(print_buf, PRINTF_BUF_SIZE, fmt, args);
    tprint(print_buf);
    xfflush(stdout);
    va_end(args);
}

@ |pdftex_fail| may be called when a buffer overflow has happened/is
   happening, therefore may not call mktexstring.  However, with the
   current implementation it appears that error messages are misleading,
   possibly because pool overflows are detected too late.

   The output format of this fuction must be the same as |pdf_error| in
   pdftex.web! 

@c
__attribute__ ((noreturn, format(printf, 1, 2)))
void pdftex_fail(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    print_ln();
    tprint("!LuaTeX error");
    if (cur_file_name) {
        tprint(" (file ");
        tprint(cur_file_name);
        tprint(")");
    }
    tprint(": ");
    vsnprintf(print_buf, PRINTF_BUF_SIZE, fmt, args);
    tprint(print_buf);
    va_end(args);
    print_ln();
    remove_pdffile(static_pdf);
    tprint(" ==> Fatal error occurred, no output PDF file produced!");
    print_ln();
    if (kpathsea_debug) {
        abort();
    } else {
        exit(EX_SOFTWARE);
    }
}

@ The output format of this fuction must be the same as |pdf_warn| in
   pdftex.web!
@c
__attribute__ ((format(printf, 1, 2)))
void pdftex_warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    print_ln();
    tex_printf("LuaTeX warning");
    if (cur_file_name)
        tex_printf(" (file %s)", cur_file_name);
    tex_printf(": ");
    vsnprintf(print_buf, PRINTF_BUF_SIZE, fmt, args);
    tprint(print_buf);
    va_end(args);
    print_ln();
}

@ @c
void garbage_warning(void)
{
    pdftex_warn("dangling objects discarded, no output file produced.");
    remove_pdffile(static_pdf);
}

@ @c
char *pdftex_banner = NULL;

@ @c
void make_pdftex_banner(void)
{
    char *s;
    unsigned int slen;
    int i;

    if (pdftex_banner != NULL)
        return;

    slen = (unsigned int) (SMALL_BUF_SIZE +
                           strlen(ptexbanner) +
                           strlen(versionstring) +
                           strlen(kpathsea_version_string));
    s = xtalloc(slen, char);
    /* The Web2c version string starts with a space.  */
    i = snprintf(s, slen,
                 "%s%s %s", ptexbanner, versionstring, kpathsea_version_string);
    check_nprintf(i, slen);
    pdftex_banner = s;
}

@ @c
size_t xfwrite(void *ptr, size_t size, size_t nmemb, FILE * stream)
{
    if (fwrite(ptr, size, nmemb, stream) != nmemb)
        pdftex_fail("fwrite() failed");
    return nmemb;
}

@ @c
int xfflush(FILE * stream)
{
    if (fflush(stream) != 0)
        pdftex_fail("fflush() failed (%s)", strerror(errno));
    return 0;
}

@ @c
int xgetc(FILE * stream)
{
    int c = getc(stream);
    if (c < 0 && c != EOF)
        pdftex_fail("getc() failed (%s)", strerror(errno));
    return c;
}

@ @c
int xputc(int c, FILE * stream)
{
    int i = putc(c, stream);
    if (i < 0)
        pdftex_fail("putc() failed (%s)", strerror(errno));
    return i;
}

@ @c
scaled ext_xn_over_d(scaled x, scaled n, scaled d)
{
    double r = (((double) x) * ((double) n)) / ((double) d);
    if (r > DBL_EPSILON)
        r += 0.5;
    else
        r -= 0.5;
    if (r >= (double) max_integer || r <= -(double) max_integer)
        pdftex_warn("arithmetic: number too big");
    return (scaled) r;
}

@ function strips trailing zeros in string with numbers; 
leading zeros are not stripped (as in real life) 
@c
#if 0
char *stripzeros(char *a)
{
    enum { NONUM, DOTNONUM, INT, DOT, LEADDOT, FRAC } s = NONUM, t = NONUM;
    char *p, *q, *r;
    for (p = q = r = a; *p != '\0';) {
        switch (s) {
        case NONUM:
            if (*p >= '0' && *p <= '9')
                s = INT;
            else if (*p == '.')
                s = LEADDOT;
            break;
        case DOTNONUM:
            if (*p != '.' && (*p < '0' || *p > '9'))
                s = NONUM;
            break;
        case INT:
            if (*p == '.')
                s = DOT;
            else if (*p < '0' || *p > '9')
                s = NONUM;
            break;
        case DOT:
        case LEADDOT:
            if (*p >= '0' && *p <= '9')
                s = FRAC;
            else if (*p == '.')
                s = DOTNONUM;
            else
                s = NONUM;
            break;
        case FRAC:
            if (*p == '.')
                s = DOTNONUM;
            else if (*p < '0' || *p > '9')
                s = NONUM;
            break;
        default:;
        }
        switch (s) {
        case DOT:
            r = q;
            break;
        case LEADDOT:
            r = q + 1;
            break;
        case FRAC:
            if (*p > '0')
                r = q + 1;
            break;
        case NONUM:
            if ((t == FRAC || t == DOT) && r != a) {
                q = r--;
                if (*r == '.')  /* was a LEADDOT */
                    *r = '0';
                r = a;
            }
            break;
        default:;
        }
        *q++ = *p++;
        t = s;
    }
    *q = '\0';
    return a;
}
#endif

@ @c
void initversionstring(char **versions)
{
    (void) asprintf(versions,
                    "Compiled with libpng %s; using libpng %s\n"
                    "Compiled with zlib %s; using zlib %s\n"
                    "Compiled with poppler version %s\n",
                    PNG_LIBPNG_VER_STRING, png_libpng_ver,
                    ZLIB_VERSION, zlib_version, POPPLER_VERSION);
}

@ @c
void check_buffer_overflow(int wsize)
{
    if (wsize > buf_size) {
        int nsize = buf_size + buf_size / 5 + 5;
        if (nsize < wsize) {
            nsize = wsize + 5;
        }
        buffer =
            (unsigned char *) xreallocarray(buffer, char, (unsigned) nsize);
        buf_size = nsize;
    }
}

@  the return value is a decimal number with the point |dd| places from the back,
   |scaled_out| is the number of scaled points corresponding to that.

@c
#define max_integer 0x7FFFFFFF

scaled divide_scaled(scaled s, scaled m, int dd)
{
    register scaled q;
    register scaled r;
    int i;
    int sign = 1;
    if (s < 0) {
        sign = -sign;
        s = -s;
    }
    if (m < 0) {
        sign = -sign;
        m = -m;
    }
    if (m == 0) {
        pdf_error("arithmetic", "divided by zero");
    } else if (m >= (max_integer / 10)) {
        pdf_error("arithmetic", "number too big");
    }
    q = s / m;
    r = s % m;
    for (i = 1; i <= (int) dd; i++) {
        q = 10 * q + (10 * r) / m;
        r = (10 * r) % m;
    }
    /* rounding */
    if (2 * r >= m) {
        q++;
    }
    return sign * q;
}

@ Same function, but using doubles instead of integers (faster) 
@c
scaled divide_scaled_n(double sd, double md, double n)
{
    double dd, di = 0.0;
    dd = sd / md * n;
    if (dd > 0.0)
        di = floor(dd + 0.5);
    else if (dd < 0.0)
        di = -floor((-dd) + 0.5);
    return (scaled) di;
}

@ @c
int do_zround(double r)
{
    int i;

    if (r > 2147483647.0)
        i = 2147483647;
    else if (r < -2147483647.0)
        i = -2147483647;
    else if (r >= 0.0)
        i = (int) (r + 0.5);
    else
        i = (int) (r - 0.5);

    return i;
}


@ MSVC doesn't have |rind|.
@c
#ifdef MSVC

#  include <math.h>
double rint(double x)
{
    double c, f, d1, d2;

    c = ceil(x);
    f = floor(x);
    d1 = fabs(c - x);
    d2 = fabs(x - f);
    if (d1 > d2)
        return f;
    else
        return c;
}

#endif
