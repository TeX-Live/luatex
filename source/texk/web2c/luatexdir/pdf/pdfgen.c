/* pdfgen.c
   
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
#include <ctype.h>
#include "commands.h"
#include "md5.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#define check_nprintf(size_get, size_want) \
    if ((unsigned)(size_get) >= (unsigned)(size_want)) \
        pdftex_fail ("snprintf failed: file %s, line %d", __FILE__, __LINE__);

static char *jobname_cstr = NULL;


/*
Sometimes it is neccesary to allocate memory for PDF output that cannot
be deallocated then, so we use |pdf_mem| for this purpose.
*/

integer pdf_mem_size = inf_pdf_mem_size;        /* allocated size of |pdf_mem| array */
integer *pdf_mem;
/* the first word is not used so we can use zero as a value for testing
   whether a pointer to |pdf_mem| is valid  */
integer pdf_mem_ptr = 1;

byte_file pdf_file;             /* the PDF output file */
real_eight_bits *pdf_buf;       /* pointer to the PDF output buffer or PDF object stream buffer */
integer pdf_buf_size = pdf_op_buf_size; /* end of PDF output buffer or PDF object stream buffer */
longinteger pdf_ptr = 0;        /* pointer to the first unused byte in the PDF buffer or object stream buffer */
real_eight_bits *pdf_op_buf;    /* the PDF output buffer */
real_eight_bits *pdf_os_buf;    /* the PDF object stream buffer */
integer pdf_os_buf_size = inf_pdf_os_buf_size;  /* current size of the PDF object stream buffer, grows dynamically */
integer *pdf_os_objnum;         /* array of object numbers within object stream */
integer *pdf_os_objoff;         /* array of object offsets within object stream */
halfword pdf_os_objidx;         /* pointer into |pdf_os_objnum| and |pdf_os_objoff| */
integer pdf_os_cntr = 0;        /* counter for object stream objects */
integer pdf_op_ptr = 0;         /* store for PDF buffer |pdf_ptr| while inside object streams */
integer pdf_os_ptr = 0;         /* store for object stream |pdf_ptr| while outside object streams */
boolean pdf_os_mode = false;    /* true if producing object stream */
boolean pdf_os_enable;          /* true if object streams are globally enabled */
integer pdf_os_cur_objnum = 0;  /* number of current object stream object */
longinteger pdf_gone = 0;       /* number of bytes that were flushed to output */
longinteger pdf_save_offset;    /* to save |pdf_offset| */
integer zip_write_state = no_zip;       /* which state of compression we are in */
integer fixed_pdf_minor_version;        /* fixed minor part of the PDF version */
boolean fixed_pdf_minor_version_set = false;    /* flag if the PDF version has been set */
integer fixed_pdf_objcompresslevel;     /* fixed level for activating PDF object streams */
integer fixed_pdfoutput;        /* fixed output format */
boolean fixed_pdfoutput_set = false;    /* |fixed_pdfoutput| has been set? */
integer fixed_gamma;
integer fixed_image_gamma;
boolean fixed_image_hicolor;
integer fixed_image_apply_gamma;
integer fixed_pdf_draftmode;    /* fixed \\pdfdraftmode */
integer pdf_page_group_val = -1;
integer epochseconds;
integer microseconds;
integer page_divert_val = 0;

void initialize_pdfgen(void)
{
    pdf_buf = pdf_op_buf;
}

void initialize_pdf_output(void)
{
    if ((pdf_minor_version < 0) || (pdf_minor_version > 9)) {
        char *hlp[] = { "The pdfminorversion must be between 0 and 9.",
            "I changed this to 4.", NULL
        };
        char msg[256];
        (void) snprintf(msg, 255, "LuaTeX error (illegal pdfminorversion %d)",
                        (int) pdf_minor_version);
        tex_error(msg, hlp);
        pdf_minor_version = 4;
    }
    fixed_pdf_minor_version = pdf_minor_version;
    fixed_decimal_digits = fix_int(pdf_decimal_digits, 0, 4);
    fixed_gamma = fix_int(pdf_gamma, 0, 1000000);
    fixed_image_gamma = fix_int(pdf_image_gamma, 0, 1000000);
    fixed_image_hicolor = fix_int(pdf_image_hicolor, 0, 1);
    fixed_image_apply_gamma = fix_int(pdf_image_apply_gamma, 0, 1);
    fixed_pdf_objcompresslevel = fix_int(pdf_objcompresslevel, 0, 3);
    fixed_pdf_draftmode = fix_int(pdf_draftmode, 0, 1);
    fixed_inclusion_copy_font = fix_int(pdf_inclusion_copy_font, 0, 1);
    fixed_replace_font = fix_int(pdf_replace_font, 0, 1);
    fixed_pk_resolution = fix_int(pdf_pk_resolution, 72, 8000);
    if ((fixed_pdf_minor_version >= 5) && (fixed_pdf_objcompresslevel > 0)) {
        pdf_os_enable = true;
    } else {
        if (fixed_pdf_objcompresslevel > 0) {
            pdf_warning(maketexstring("Object streams"),
                        maketexstring
                        ("\\pdfobjcompresslevel > 0 requires \\pdfminorversion > 4. Object streams disabled now."),
                        true, true);
            fixed_pdf_objcompresslevel = 0;
        }
        pdf_os_enable = false;
    }
    if (pdf_pk_resolution == 0) /* if not set from format file or by user */
        pdf_pk_resolution = pk_dpi;     /* take it from \.{texmf.cnf} */
    pk_scale_factor =
        divide_scaled(72, fixed_pk_resolution, 5 + fixed_decimal_digits);
    if (!callback_defined(read_pk_file_callback)) {
        if (pdf_pk_mode != null) {
            kpseinitprog("PDFTEX", fixed_pk_resolution,
                         makecstring(tokens_to_string(pdf_pk_mode)), nil);
            flush_string();
        } else {
            kpseinitprog("PDFTEX", fixed_pk_resolution, nil, nil);
        }
        if (!kpsevarvalue("MKTEXPK"))
            kpsesetprogramenabled(kpsepkformat, 1, kpsesrccmdline);
    }
    set_job_id(int_par(param_year_code),
               int_par(param_month_code),
               int_par(param_day_code), int_par(param_time_code));

    if ((pdf_unique_resname > 0) && (pdf_resname_prefix == 0))
        pdf_resname_prefix = get_resname_prefix();
    pdf_page_init();
}

/*
  We use |pdf_get_mem| to allocate memory in |pdf_mem|
*/

integer pdf_get_mem(integer s)
{                               /* allocate |s| words in |pdf_mem| */
    integer a;
    integer ret;
    if (s > sup_pdf_mem_size - pdf_mem_ptr)
        overflow(maketexstring("PDF memory size (pdf_mem_size)"), pdf_mem_size);
    if (pdf_mem_ptr + s > pdf_mem_size) {
        a = 0.2 * pdf_mem_size;
        if (pdf_mem_ptr + s > pdf_mem_size + a) {
            pdf_mem_size = pdf_mem_ptr + s;
        } else if (pdf_mem_size < sup_pdf_mem_size - a) {
            pdf_mem_size = pdf_mem_size + a;
        } else {
            pdf_mem_size = sup_pdf_mem_size;
        }
        pdf_mem = xreallocarray(pdf_mem, integer, pdf_mem_size);
    }
    ret = pdf_mem_ptr;
    pdf_mem_ptr = pdf_mem_ptr + s;
    return ret;
}


/*
This ensures that |pdfminorversion| is set only before any bytes have
been written to the generated \.{PDF} file. Here also all variables for
\.{PDF} output are initialized, the \.{PDF} file is opened by |ensure_pdf_open|,
and the \.{PDF} header is written.
*/

void check_pdfminorversion(void)
{
    fix_pdfoutput();
    assert(fixed_pdfoutput > 0);
    if (!fixed_pdf_minor_version_set) {
        fixed_pdf_minor_version_set = true;
        /* Initialize variables for \.{PDF} output */
        prepare_mag();
        initialize_pdf_output();
        /* Write \.{PDF} header */
        ensure_pdf_open();
        pdf_printf("%%PDF-1.%d\n", (int) fixed_pdf_minor_version);
        pdf_out('%');
        pdf_out('P' + 128);
        pdf_out('T' + 128);
        pdf_out('E' + 128);
        pdf_out('X' + 128);
        pdf_print_nl();

    } else {
        /* Check that variables for \.{PDF} output are unchanged */
        if (fixed_pdf_minor_version != pdf_minor_version)
            pdf_error(maketexstring("setup"),
                      maketexstring
                      ("\\pdfminorversion cannot be changed after data is written to the PDF file"));
        if (fixed_pdf_draftmode != pdf_draftmode)
            pdf_error(maketexstring("setup"),
                      maketexstring
                      ("\\pdfdraftmode cannot be changed after data is written to the PDF file"));

    }
    if (fixed_pdf_draftmode != 0) {
        pdf_compress_level = 0; /* re-fix it, might have been changed inbetween */
        fixed_pdf_objcompresslevel = 0;
    }
}

/* Checks that we have a name for the generated PDF file and that it's open. */

void ensure_pdf_open(void)
{
    if (output_file_name != 0)
        return;
    if (job_name == 0)
        open_log_file();
    pack_job_name(".pdf");
    if (fixed_pdf_draftmode == 0) {
        while (!lua_b_open_out(pdf_file))
            prompt_file_name("file name for output", ".pdf");
    }
    pdf_file = name_file_pointer;
    output_file_name = make_name_string();
}

/*
The PDF buffer is flushed by calling |pdf_flush|, which checks the
variable |zip_write_state| and will compress the buffer before flushing if
neccesary. We call |pdf_begin_stream| to begin a stream  and |pdf_end_stream|
to finish it. The stream contents will be compressed if compression is turn on.
*/

void pdf_flush(void)
{                               /* flush out the |pdf_buf| */
    longinteger saved_pdf_gone;
    if (!pdf_os_mode) {
        saved_pdf_gone = pdf_gone;
        switch (zip_write_state) {
        case no_zip:
            if (pdf_ptr > 0) {
                if (fixed_pdf_draftmode == 0)
                    write_pdf(0, pdf_ptr - 1);
                pdf_gone = pdf_gone + pdf_ptr;
                pdf_last_byte = pdf_buf[pdf_ptr - 1];
            }
            break;
        case zip_writing:
            if (fixed_pdf_draftmode == 0)
                write_zip(false);
            break;
        case zip_finish:
            if (fixed_pdf_draftmode == 0)
                write_zip(true);
            zip_write_state = no_zip;
            break;
        }
        pdf_ptr = 0;
        if (saved_pdf_gone > pdf_gone)
            pdf_error(maketexstring("file size"),
                      maketexstring
                      ("File size exceeds architectural limits (pdf_gone wraps around)"));
    }
}

/* low-level buffer checkers */

/* check that |s| bytes more fit into |pdf_os_buf|; increase it if required */
void pdf_os_get_os_buf(integer s)
{
    integer a;
    if (s > sup_pdf_os_buf_size - pdf_ptr)
        overflow(maketexstring("PDF object stream buffer"), pdf_os_buf_size);
    if (pdf_ptr + s > pdf_os_buf_size) {
        a = 0.2 * pdf_os_buf_size;
        if (pdf_ptr + s > pdf_os_buf_size + a)
            pdf_os_buf_size = pdf_ptr + s;
        else if (pdf_os_buf_size < sup_pdf_os_buf_size - a)
            pdf_os_buf_size = pdf_os_buf_size + a;
        else
            pdf_os_buf_size = sup_pdf_os_buf_size;
        pdf_os_buf =
            xreallocarray(pdf_os_buf, real_eight_bits, pdf_os_buf_size);
        pdf_buf = pdf_os_buf;
        pdf_buf_size = pdf_os_buf_size;
    }
}

/* make sure that there are at least |n| bytes free in PDF buffer */
void pdf_room(integer n)
{
    if (pdf_os_mode && (n + pdf_ptr > pdf_buf_size))
        pdf_os_get_os_buf(n);
    else if ((!pdf_os_mode) && (n > pdf_buf_size))
        overflow(maketexstring("PDF output buffer"), pdf_op_buf_size);
    else if ((!pdf_os_mode) && (n + pdf_ptr > pdf_buf_size))
        pdf_flush();
}


#define is_hex_char isxdigit

/* print out a character to PDF buffer; the character will be printed in octal
 * form in the following cases: chars <= 32, backslash (92), left parenthesis
 * (40) and  right parenthesis (41) 
 */

#define pdf_print_escaped(c)                                            \
  if ((c)<=32||(c)=='\\'||(c)=='('||(c)==')'||(c)>127) {                \
    pdf_room(4);                                                        \
    pdf_quick_out('\\');                                                \
    pdf_quick_out('0' + (((c)>>6) & 0x3));                              \
    pdf_quick_out('0' + (((c)>>3) & 0x7));                              \
    pdf_quick_out('0' + ( (c)     & 0x7));                              \
  } else {                                                              \
    pdf_out((c));                                                       \
  }

void pdf_print_char(internal_font_number f, integer cc)
{
    register int c;
    pdf_mark_char(f, cc);
    if (font_encodingbytes(f) == 2) {
        register int chari;
        chari = char_index(f, cc);
        c = chari >> 8;
        pdf_print_escaped(c);
        c = chari & 0xFF;
    } else {
        if (cc > 255)
            return;
        c = cc;
    }
    pdf_print_escaped(c);
}

void pdf_puts(const char *s)
{
    pdf_room(strlen(s) + 1);
    while (*s)
        pdf_buf[pdf_ptr++] = *s++;
}

static char pdf_printf_buf[PRINTF_BUF_SIZE];

__attribute__ ((format(printf, 1, 2)))
void pdf_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(pdf_printf_buf, PRINTF_BUF_SIZE, fmt, args);
    pdf_puts(pdf_printf_buf);
    va_end(args);
}


/* print out a string to PDF buffer */

void pdf_print(str_number s)
{
    if (s < number_chars) {
        assert(s < 256);
        pdf_out(s);
    } else {
        register pool_pointer j = str_start_macro(s);
        while (j < str_start_macro(s + 1)) {
            pdf_out(str_pool[j++]);
        }
    }
}

/* print out a integer to PDF buffer */

void pdf_print_int(longinteger n)
{
    register integer k = 0;     /*  current digit; we assume that $|n|<10^{23}$ */
    if (n < 0) {
        pdf_out('-');
        if (n < -0x7FFFFFFF) {  /* need to negate |n| more carefully */
            register longinteger m;
            k++;
            m = -1 - n;
            n = m / 10;
            m = (m % 10) + 1;
            if (m < 10) {
                dig[0] = m;
            } else {
                dig[0] = 0;
                incr(n);
            }
        } else {
            n = -n;
        }
    }
    do {
        dig[k++] = n % 10;
        n = n / 10;
    } while (n != 0);
    pdf_room(k);
    while (k-- > 0) {
        pdf_quick_out('0' + dig[k]);
    }
}


/* print $m/10^d$ as real */
void pdf_print_real(integer m, integer d)
{
    if (m < 0) {
        pdf_out('-');
        m = -m;
    };
    pdf_print_int(m / ten_pow[d]);
    m = m % ten_pow[d];
    if (m > 0) {
        pdf_out('.');
        d--;
        while (m < ten_pow[d]) {
            pdf_out('0');
            d--;
        }
        while (m % 10 == 0)
            m = m / 10;
        pdf_print_int(m);
    }
}

/* print out |s| as string in PDF output */

void pdf_print_str(str_number s)
{
    pool_pointer i, j;
    i = str_start_macro(s);
    j = str_start_macro(s + 1) - 1;
    if (i > j) {
        pdf_room(2);
        pdf_quick_out('(');
        pdf_quick_out(')');
        return;
    }
    /* the next is not really safe, the string could be "(a)xx(b)" */
    if ((str_pool[i] == '(') && (str_pool[j] == ')')) {
        pdf_print(s);
        return;
    }
    if ((str_pool[i] != '<') || (str_pool[j] != '>') || odd(str_length(s))) {
        pdf_out('(');
        pdf_print(s);
        pdf_out(')');
        return;
    }
    i++;
    j--;
    while (i < j) {
        if (!is_hex_char(str_pool[i++])) {
            pdf_out('(');
            pdf_print(s);
            pdf_out(')');
            return;
        }
    }
    pdf_print(s);               /* it was a hex string after all  */
}


/* begin a stream */
void pdf_begin_stream(void)
{
    assert(pdf_os_mode == false);
    pdf_printf("/Length           \n");
    pdf_seek_write_length = true;       /* fill in length at |pdf_end_stream| call */
    pdf_stream_length_offset = pdf_offset - 11;
    pdf_stream_length = 0;
    pdf_last_byte = 0;
    if (pdf_compress_level > 0) {
        pdf_printf("/Filter /FlateDecode\n");
        pdf_printf(">>\n");
        pdf_printf("stream\n");
        pdf_flush();
        zip_write_state = zip_writing;
    } else {
        pdf_printf(">>\n");
        pdf_printf("stream\n");
        pdf_save_offset = pdf_offset;
    }
}

/* end a stream */
void pdf_end_stream(void)
{
    if (zip_write_state == zip_writing)
        zip_write_state = zip_finish;
    else
        pdf_stream_length = pdf_offset - pdf_save_offset;
    pdf_flush();
    if (pdf_seek_write_length)
        write_stream_length(pdf_stream_length, pdf_stream_length_offset);
    pdf_seek_write_length = false;
    if (pdf_last_byte != pdf_new_line_char)
        pdf_out(pdf_new_line_char);
    pdf_printf("endstream\n");
    pdf_end_obj();
}

void pdf_remove_last_space(void)
{
    if ((pdf_ptr > 0) && (pdf_buf[pdf_ptr - 1] == ' '))
        decr(pdf_ptr);
}


/*
To print |scaled| value to PDF output we need some subroutines to ensure
accurary.
*/

#define max_integer 0x7FFFFFFF  /* $2^{31}-1$ */

/* scaled value corresponds to 100in, exact, 473628672 */
scaled one_hundred_inch = 7227 * 65536;

/* scaled value corresponds to 1in (rounded to 4736287) */
scaled one_inch = (7227 * 65536 + 50) / 100;

    /* scaled value corresponds to 1truein (rounded!) */
scaled one_true_inch = (7227 * 65536 + 50) / 100;

/* scaled value corresponds to 100bp */
scaled one_hundred_bp = (7227 * 65536) / 72;

/* scaled value corresponds to 1bp (rounded to 65782) */
scaled one_bp = ((7227 * 65536) / 72 + 50) / 100;

/* $10^0..10^9$ */
integer ten_pow[10] =
    { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000,
    1000000000
};

/*
The function |divide_scaled| divides |s| by |m| using |dd| decimal
digits of precision. It is defined in C because it is a good candidate
for optimizations that are not possible in pascal.
*/

scaled round_xn_over_d(scaled x, integer n, integer d)
{
    boolean positive;           /* was |x>=0|? */
    nonnegative_integer t, u, v;        /* intermediate quantities */
    if (x >= 0) {
        positive = true;
    } else {
        x = -(x);
        positive = false;
    }
    t = (x % 0100000) * n;
    u = (x / 0100000) * n + (t / 0100000);
    v = (u % d) * 0100000 + (t % 0100000);
    if (u / d >= 0100000)
        arith_error = true;
    else
        u = 0100000 * (u / d) + (v / d);
    v = v % d;
    if (2 * v >= d)
        u++;
    if (positive)
        return u;
    else
        return (-u);
}

#define lround(a) (long) floor((a) + 0.5)

void pdf_print_bp(scaled s)
{                               /* print scaled as |bp| */
    pdffloat a;
    assert(pstruct != NULL);
    a.m = lround(s * pstruct->k1);
    a.e = fixed_decimal_digits;
    print_pdffloat(&a);
}

void pdf_print_mag_bp(scaled s)
{                               /* take |mag| into account */
    pdffloat a;
    prepare_mag();
    if (int_par(param_mag_code) != 1000)
        a.m = lround(s * (long) int_par(param_mag_code) / 1000.0 * pstruct->k1);
    else
        a.m = lround(s * pstruct->k1);
    a.e = fixed_decimal_digits;
    print_pdffloat(&a);
}

integer fixed_pk_resolution;
integer fixed_decimal_digits;
integer fixed_gen_tounicode;
integer fixed_inclusion_copy_font;
integer fixed_replace_font;
integer pk_scale_factor;
integer pdf_output_option;
integer pdf_output_value;
integer pdf_draftmode_option;
integer pdf_draftmode_value;

/* mark |f| as a used font; set |font_used(f)|, |pdf_font_size(f)| and |pdf_font_num(f)| */
void pdf_use_font(internal_font_number f, integer fontnum)
{
    set_pdf_font_size(f, font_size(f));
    set_font_used(f, true);
    assert((fontnum > 0) || ((fontnum < 0) && (pdf_font_num(-fontnum) > 0)));
    set_pdf_font_num(f, fontnum);
    if (pdf_move_chars > 0) {
        pdf_warning(0, maketexstring("Primitive \\pdfmovechars is obsolete."),
                    true, true);
        pdf_move_chars = 0;     /* warn only once */
    }
}

/*
To set PDF font we need to find out fonts with the same name, because \TeX\
can load the same font several times for various sizes. For such fonts we
define only one font resource. The array |pdf_font_num| holds the object
number of font resource. A negative value of an entry of |pdf_font_num|
indicates that the corresponding font shares the font resource with the font
*/

/* create a font object */
void pdf_init_font(internal_font_number f)
{
    internal_font_number k, b;
    integer i;
    assert(!font_used(f));

    /* if |f| is auto expanded then ensure the base font is initialized */
    if (pdf_font_auto_expand(f) && (pdf_font_blink(f) != null_font)) {
        b = pdf_font_blink(f);
        /* TODO: reinstate this check. disabled because wide fonts font have fmentries */
        if (false && (!hasfmentry(b)))
            pdf_error(maketexstring("font expansion"),
                      maketexstring
                      ("auto expansion is only possible with scalable fonts"));
        if (!font_used(b))
            pdf_init_font(b);
        set_font_map(f, font_map(b));
    }
    /* check whether |f| can share the font object with some |k|: we have 2 cases
       here: 1) |f| and |k| have the same tfm name (so they have been loaded at
       different sizes, eg 'cmr10' and 'cmr10 at 11pt'); 2) |f| has been auto
       expanded from |k|
     */
    if (hasfmentry(f) || true) {
        i = head_tab[obj_type_font];
        while (i != 0) {
            k = obj_info(i);
            if (font_shareable(f, k)) {
                assert(pdf_font_num(k) != 0);
                if (pdf_font_num(k) < 0)
                    pdf_use_font(f, pdf_font_num(k));
                else
                    pdf_use_font(f, -k);
                return;
            }
            i = obj_link(i);
        }
    }
    /* create a new font object for |f| */
    pdf_create_obj(obj_type_font, f);
    pdf_use_font(f, obj_ptr);
}


/* set the actual font on PDF page */
internal_font_number pdf_set_font(internal_font_number f)
{
    pointer p;
    internal_font_number k;
    if (!font_used(f))
        pdf_init_font(f);
    set_ff(f);                  /* set |ff| to the tfm number of the font sharing the font object
                                   with |f|; |ff| is either |f| or some font with the same tfm name
                                   at different size and/or expansion */
    k = ff;
    p = pdf_font_list;
    while (p != null) {
        set_ff(fixmem[p].hhlh); /* info(p) */
        if (ff == k)
            goto FOUND;
        p = fixmem[p].hhrh;     /* link(p) */
    }
    pdf_append_list(f, pdf_font_list);  /* |f| not found in |pdf_font_list|, append it now */
  FOUND:
    return k;
}

/* Subroutines to print out various PDF objects */

/* print out an integer with fixed width; used for outputting cross-reference table */
void pdf_print_fw_int(longinteger n, integer w)
{
    integer k;                  /* $0\le k\le23$ */
    k = 0;
    do {
        dig[k] = n % 10;
        n = n / 10;
        k++;
    } while (k != w);
    pdf_room(k);
    while (k-- > 0)
        pdf_quick_out('0' + dig[k]);
}

/* print out an integer as a number of bytes; used for outputting \.{/XRef} cross-reference stream */
void pdf_out_bytes(longinteger n, integer w)
{
    integer k;
    integer bytes[8];           /* digits in a number being output */
    k = 0;
    do {
        bytes[k] = n % 256;
        n = n / 256;
        k++;
    } while (k != w);
    pdf_room(k);
    while (k-- > 0)
        pdf_quick_out(bytes[k]);
}

/* print out an entry in dictionary with integer value to PDF buffer */

void pdf_int_entry(str_number s, integer v)
{
    pdf_out('/');
    pdf_print(s);
    pdf_out(' ');
    pdf_print_int(v);
}

void pdf_int_entry_ln(str_number s, integer v)
{
    pdf_int_entry(s, v);
    pdf_print_nl();
}


/* print out an indirect entry in dictionary */
void pdf_indirect(str_number s, integer o)
{
    pdf_out('/');
    pdf_print(s);
    pdf_printf(" %d 0 R", (int) o);
}

void pdf_indirect_ln(str_number s, integer o)
{
    pdf_indirect(s, o);
    pdf_print_nl();
}

/* print out |s| as string in PDF output */

void pdf_print_str_ln(str_number s)
{
    pdf_print_str(s);
    pdf_print_nl();
}

/* print out an entry in dictionary with string value to PDF buffer */

void pdf_str_entry(str_number s, str_number v)
{
    if (v == 0)
        return;
    pdf_out('/');
    pdf_print(s);
    pdf_out(' ');
    pdf_print_str(v);
}

void pdf_str_entry_ln(str_number s, str_number v)
{
    if (v == 0)
        return;
    pdf_str_entry(s, v);
    pdf_print_nl();
}

void pdf_print_toks(halfword p)
{
    str_number s = tokens_to_string(p);
    if (str_length(s) > 0)
        pdf_print(s);
    flush_str(s);
}


void pdf_print_toks_ln(halfword p)
{
    str_number s = tokens_to_string(p);
    if (str_length(s) > 0)
        pdf_print_ln(s);
    flush_str(s);
}

/* prints a rect spec */
void pdf_print_rect_spec(halfword r)
{
    pdf_print_mag_bp(pdf_ann_left(r));
    pdf_out(' ');
    pdf_print_mag_bp(pdf_ann_bottom(r));
    pdf_out(' ');
    pdf_print_mag_bp(pdf_ann_right(r));
    pdf_out(' ');
    pdf_print_mag_bp(pdf_ann_top(r));
}

/* output a rectangle specification to PDF file */
void pdf_rectangle(halfword r)
{
    prepare_mag();
    pdf_printf("/Rect [");
    pdf_print_rect_spec(r);
    pdf_printf("]\n");
}


/* begin a PDF dictionary object */
void pdf_begin_dict(integer i, integer pdf_os_level)
{
    check_pdfminorversion();
    pdf_os_prepare_obj(i, pdf_os_level);
    if (!pdf_os_mode) {
        pdf_printf("%d 0 obj <<\n", (int) i);
    } else {
        if (pdf_compress_level == 0)
            pdf_printf("%% %d 0 obj\n", (int) i);       /* debugging help */
        pdf_printf("<<\n");
    }
}

/* begin a new PDF dictionary object */
void pdf_new_dict(integer t, integer i, integer pdf_os)
{
    pdf_create_obj(t, i);
    pdf_begin_dict(obj_ptr, pdf_os);
}

/* end a PDF dictionary object */
void pdf_end_dict(void)
{
    if (pdf_os_mode) {
        pdf_printf(">>\n");
        if (pdf_os_objidx == pdf_os_max_objs - 1)
            pdf_os_write_objstream();
    } else {
        pdf_printf(">> endobj\n");
    }
}


/*
Write out an accumulated object stream.
First the object number and byte offset pairs are generated
and appended to the ready buffered object stream.
By this the value of \.{/First} can be calculated.
Then a new \.{/ObjStm} object is generated, and everything is
copied to the PDF output buffer, where also compression is done.
When calling this procedure, |pdf_os_mode| must be |true|.
*/

void pdf_os_write_objstream(void)
{
    halfword i, j, p, q;
    if (pdf_os_cur_objnum == 0) /* no object stream started */
        return;
    p = pdf_ptr;
    i = 0;
    j = 0;
    while (i <= pdf_os_objidx) {        /* assemble object number and byte offset pairs */
        pdf_printf("%d %d", (int) pdf_os_objnum[i], (int) pdf_os_objoff[i]);
        if (j == 9) {           /* print out in groups of ten for better readability */
            pdf_out(pdf_new_line_char);
            j = 0;
        } else {
            pdf_printf(" ");
            incr(j);
        }
        incr(i);
    }
    pdf_buf[pdf_ptr - 1] = pdf_new_line_char;   /* no risk of flush, as we are in |pdf_os_mode| */
    q = pdf_ptr;
    pdf_begin_dict(pdf_os_cur_objnum, 0);       /* switch to PDF stream writing */
    pdf_printf("/Type /ObjStm\n");
    pdf_printf("/N %d\n", (int) (pdf_os_objidx + 1));
    pdf_printf("/First %d\n", (int) (q - p));
    pdf_begin_stream();
    pdf_room(q - p);            /* should always fit into the PDF output buffer */
    i = p;
    while (i < q) {             /* write object number and byte offset pairs */
        pdf_quick_out(pdf_os_buf[i]);
        incr(i);
    }
    i = 0;
    while (i < p) {
        q = i + pdf_buf_size;
        if (q > p)
            q = p;
        pdf_room(q - i);
        while (i < q) {         /* write the buffered objects */
            pdf_quick_out(pdf_os_buf[i]);
            incr(i);
        }
    }
    pdf_end_stream();
    pdf_os_cur_objnum = 0;      /* to force object stream generation next time */
}

/* begin a PDF object */
void pdf_begin_obj(integer i, integer pdf_os_level)
{
    check_pdfminorversion();
    pdf_os_prepare_obj(i, pdf_os_level);
    if (!pdf_os_mode) {
        pdf_printf("%d 0 obj\n", (int) i);
    } else if (pdf_compress_level == 0) {
        pdf_printf("%% %d 0 obj\n", (int) i);   /* debugging help */
    }
}

/* begin a new PDF object */
void pdf_new_obj(integer t, integer i, integer pdf_os)
{
    pdf_create_obj(t, i);
    pdf_begin_obj(obj_ptr, pdf_os);
}


/* end a PDF object */
void pdf_end_obj(void)
{
    if (pdf_os_mode) {
        if (pdf_os_objidx == pdf_os_max_objs - 1)
            pdf_os_write_objstream();
    } else {
        pdf_printf("endobj\n"); /* end a PDF object */
    }
}


void write_stream_length(integer length, longinteger offset)
{
    if (jobname_cstr == NULL)
        jobname_cstr = xstrdup(makecstring(job_name));
    if (fixed_pdf_draftmode == 0) {
        xfseeko(pdf_file, (off_t) offset, SEEK_SET, jobname_cstr);
        fprintf(pdf_file, "%li", (long int) length);
        xfseeko(pdf_file, (off_t) pdf_offset, SEEK_SET, jobname_cstr);
    }
}


/* Converts any string given in in in an allowed PDF string which can be
 * handled by printf et.al.: \ is escaped to \\, parenthesis are escaped and
 * control characters are octal encoded.
 * This assumes that the string does not contain any already escaped
 * characters!
 */
char *convertStringToPDFString(const char *in, int len)
{
    static char pstrbuf[MAX_PSTRING_LEN];
    char *out = pstrbuf;
    int i, j, k;
    char buf[5];
    j = 0;
    for (i = 0; i < len; i++) {
        check_buf(j + sizeof(buf), MAX_PSTRING_LEN);
        if (((unsigned char) in[i] < '!') || ((unsigned char) in[i] > '~')) {
            /* convert control characters into oct */
            k = snprintf(buf, sizeof(buf),
                         "\\%03o", (unsigned int) (unsigned char) in[i]);
            check_nprintf(k, sizeof(buf));
            out[j++] = buf[0];
            out[j++] = buf[1];
            out[j++] = buf[2];
            out[j++] = buf[3];
        } else if ((in[i] == '(') || (in[i] == ')')) {
            /* escape paranthesis */
            out[j++] = '\\';
            out[j++] = in[i];
        } else if (in[i] == '\\') {
            /* escape backslash */
            out[j++] = '\\';
            out[j++] = '\\';
        } else {
            /* copy char :-) */
            out[j++] = in[i];
        }
    }
    out[j] = '\0';
    return pstrbuf;
}

/* Converts any string given in in in an allowed PDF string which is
 * hexadecimal encoded;
 * sizeof(out) should be at least lin*2+1.
 */

static void convertStringToHexString(const char *in, char *out, int lin)
{
    int i, j, k;
    char buf[3];
    j = 0;
    for (i = 0; i < lin; i++) {
        k = snprintf(buf, sizeof(buf),
                     "%02X", (unsigned int) (unsigned char) in[i]);
        check_nprintf(k, sizeof(buf));
        out[j++] = buf[0];
        out[j++] = buf[1];
    }
    out[j] = '\0';
}


/* Converts any string given in in in an allowed PDF string which can be
 * handled by printf et.al.: \ is escaped to \\, parenthesis are escaped and
 * control characters are octal encoded.
 * This assumes that the string does not contain any already escaped
 * characters!
 *
 * See escapename for parameter description.
 */
void escapestring(poolpointer in)
{
    const poolpointer out = pool_ptr;
    unsigned char ch;
    int i;
    while (in < out) {
        if (pool_ptr + 4 >= pool_size) {
            pool_ptr = pool_size;
            /* error by str_toks that calls str_room(1) */
            return;
        }

        ch = (unsigned char) str_pool[in++];

        if ((ch < '!') || (ch > '~')) {
            /* convert control characters into oct */
            i = snprintf((char *) &str_pool[pool_ptr], 5,
                         "\\%.3o", (unsigned int) ch);
            check_nprintf(i, 5);
            pool_ptr += i;
            continue;
        }
        if ((ch == '(') || (ch == ')') || (ch == '\\')) {
            /* escape parenthesis and backslash */
            str_pool[pool_ptr++] = '\\';
        }
        /* copy char :-) */
        str_pool[pool_ptr++] = ch;
    }
}

/* Convert any given string in a PDF name using escaping mechanism
   of PDF 1.2. The result does not include the leading slash.

   PDF specification 1.6, section 3.2.6 "Name Objects" explains:
   <blockquote>
    Beginning with PDF 1.2, any character except null (character code 0) may
    be included in a name by writing its 2-digit hexadecimal code, preceded
    by the number sign character (#); see implementation notes 3 and 4 in
    Appendix H. This syntax is required to represent any of the delimiter or
    white-space characters or the number sign character itself; it is
    recommended but not required for characters whose codes are outside the
    range 33 (!) to 126 (~).
   </blockquote>
   The following table shows the conversion that are done by this
   function:
     code      result   reason
     -----------------------------------
     0         ignored  not allowed
     1..32     escaped  must for white-space:
                          9 (tab), 10 (lf), 12 (ff), 13 (cr), 32 (space)
                        recommended for the other control characters
     35        escaped  escape char "#"
     37        escaped  delimiter "%"
     40..41    escaped  delimiters "(" and ")"
     47        escaped  delimiter "/"
     60        escaped  delimiter "<"
     62        escaped  delimiter ">"
     91        escaped  delimiter "["
     93        escaped  delimiter "]"
     123       escaped  delimiter "{"
     125       escaped  delimiter "}"
     127..255  escaped  recommended
     else      copy     regular characters

   Parameter "in" is a pointer into the string pool where
   the input string is located. The output string is written
   as temporary string right after the input string.
   Thus at the begin of the procedure the global variable
   "pool_ptr" points to the start of the output string and
   after the end when the procedure returns.
*/
void escapename(poolpointer in)
{
    const poolpointer out = pool_ptr;
    unsigned char ch;
    int i;

    while (in < out) {
        if (pool_ptr + 3 >= pool_size) {
            pool_ptr = pool_size;
            /* error by str_toks that calls str_room(1) */
            return;
        }

        ch = (unsigned char) str_pool[in++];

        if ((ch >= 1 && ch <= 32) || ch >= 127) {
            /* escape */
            i = snprintf((char *) &str_pool[pool_ptr], 4,
                         "#%.2X", (unsigned int) ch);
            check_nprintf(i, 4);
            pool_ptr += i;
            continue;
        }
        switch (ch) {
        case 0:
            /* ignore */
            break;
        case 35:
        case 37:
        case 40:
        case 41:
        case 47:
        case 60:
        case 62:
        case 91:
        case 93:
        case 123:
        case 125:
            /* escape */
            i = snprintf((char *) &str_pool[pool_ptr], 4,
                         "#%.2X", (unsigned int) ch);
            check_nprintf(i, 4);
            pool_ptr += i;
            break;
        default:
            /* copy */
            str_pool[pool_ptr++] = ch;
        }
    }
}

/* Compute the ID string as per PDF1.4 9.3:
  <blockquote>
    File identifers are defined by the optional ID entry in a PDF file's
    trailer dictionary (see Section 3.4.4, "File Trailer"; see also
    implementation note 105 in Appendix H). The value of this entry is an
    array of two strings. The first string is a permanent identifier based
    on the contents of the file at the time it was originally created, and
    does not change when the file is incrementally updated. The second
    string is a changing identifier based on the file's contents at the
    time it was last updated. When a file is first written, both
    identifiers are set to the same value. If both identifiers match when a
    file reference is resolved, it is very likely that the correct file has
    been found; if only the first identifier matches, then a different
    version of the correct file has been found.
        To help ensure the uniqueness of file identifiers, it is recommend
    that they be computed using a message digest algorithm such as MD5
    (described in Internet RFC 1321, The MD5 Message-Digest Algorithm; see
    the Bibliography), using the following information (see implementation
    note 106 in Appendix H):
    - The current time
    - A string representation of the file's location, usually a pathname
    - The size of the file in bytes
    - The values of all entries in the file's document information
      dictionary (see Section 9.2.1,  Document Information Dictionary )
  </blockquote>
  This stipulates only that the two IDs must be identical when the file is
  created and that they should be reasonably unique. Since it's difficult
  to get the file size at this point in the execution of pdfTeX and
  scanning the info dict is also difficult, we start with a simpler
  implementation using just the first two items.
 */
void print_ID(str_number filename)
{
    time_t t;
    size_t size;
    char time_str[32];
    md5_state_t state;
    md5_byte_t digest[16];
    char id[64];
    char *file_name;
    char pwd[4096];
    /* start md5 */
    md5_init(&state);
    /* get the time */
    t = time(NULL);
    size = strftime(time_str, sizeof(time_str), "%Y%m%dT%H%M%SZ", gmtime(&t));
    md5_append(&state, (const md5_byte_t *) time_str, size);
    /* get the file name */
    if (getcwd(pwd, sizeof(pwd)) == NULL)
        pdftex_fail("getcwd() failed (%s), (path too long?)", strerror(errno));
    file_name = makecstring(filename);
    md5_append(&state, (const md5_byte_t *) pwd, strlen(pwd));
    md5_append(&state, (const md5_byte_t *) "/", 1);
    md5_append(&state, (const md5_byte_t *) file_name, strlen(file_name));
    /* finish md5 */
    md5_finish(&state, digest);
    /* write the IDs */
    convertStringToHexString((char *) digest, id, 16);
    pdf_printf("/ID [<%s> <%s>]", id, id);
}

/* Print the /CreationDate entry.

  PDF Reference, third edition says about the expected date format:
  <blockquote>
    3.8.2 Dates

      PDF defines a standard date format, which closely follows that of
      the international standard ASN.1 (Abstract Syntax Notation One),
      defined in ISO/IEC 8824 (see the Bibliography). A date is a string
      of the form

        (D:YYYYMMDDHHmmSSOHH'mm')

      where

        YYYY is the year
        MM is the month
        DD is the day (01-31)
        HH is the hour (00-23)
        mm is the minute (00-59)
        SS is the second (00-59)
        O is the relationship of local time to Universal Time (UT),
          denoted by one of the characters +, -, or Z (see below)
        HH followed by ' is the absolute value of the offset from UT
          in hours (00-23)
        mm followed by ' is the absolute value of the offset from UT
          in minutes (00-59)

      The apostrophe character (') after HH and mm is part of the syntax.
      All fields after the year are optional. (The prefix D:, although also
      optional, is strongly recommended.) The default values for MM and DD
      are both 01; all other numerical fields default to zero values.  A plus
      sign (+) as the value of the O field signifies that local time is
      later than UT, a minus sign (-) that local time is earlier than UT,
      and the letter Z that local time is equal to UT. If no UT information
      is specified, the relationship of the specified time to UT is
      considered to be unknown. Whether or not the time zone is known, the
      rest of the date should be specified in local time.

      For example, December 23, 1998, at 7:52 PM, U.S. Pacific Standard
      Time, is represented by the string

        D:199812231952-08'00'
  </blockquote>

  The main difficulty is get the time zone offset. strftime() does this in ISO
  C99 (e.g. newer glibc) with %z, but we have to work with other systems (e.g.
  Solaris 2.5).
*/

static time_t start_time = 0;
#define TIME_STR_SIZE 30
static char start_time_str[TIME_STR_SIZE];      /* minimum size for time_str is 24: "D:YYYYmmddHHMMSS+HH'MM'" */

static void makepdftime(time_t t, char *time_str)
{
    struct tm lt, gmt;
    size_t size;
    int i, off, off_hours, off_mins;

    /* get the time */
    lt = *localtime(&t);
    size = strftime(time_str, TIME_STR_SIZE, "D:%Y%m%d%H%M%S", &lt);
    /* expected format: "YYYYmmddHHMMSS" */
    if (size == 0) {
        /* unexpected, contents of time_str is undefined */
        time_str[0] = '\0';
        return;
    }

    /* correction for seconds: %S can be in range 00..61,
       the PDF reference expects 00..59,
       therefore we map "60" and "61" to "59" */
    if (time_str[14] == '6') {
        time_str[14] = '5';
        time_str[15] = '9';
        time_str[16] = '\0';    /* for safety */
    }

    /* get the time zone offset */
    gmt = *gmtime(&t);

    /* this calculation method was found in exim's tod.c */
    off = 60 * (lt.tm_hour - gmt.tm_hour) + lt.tm_min - gmt.tm_min;
    if (lt.tm_year != gmt.tm_year) {
        off += (lt.tm_year > gmt.tm_year) ? 1440 : -1440;
    } else if (lt.tm_yday != gmt.tm_yday) {
        off += (lt.tm_yday > gmt.tm_yday) ? 1440 : -1440;
    }

    if (off == 0) {
        time_str[size++] = 'Z';
        time_str[size] = 0;
    } else {
        off_hours = off / 60;
        off_mins = abs(off - off_hours * 60);
        i = snprintf(&time_str[size], 9, "%+03d'%02d'", off_hours, off_mins);
        check_nprintf(i, 9);
    }
}

void init_start_time()
{
    if (start_time == 0) {
        start_time = time((time_t *) NULL);
        makepdftime(start_time, start_time_str);
    }
}

void print_creation_date()
{
    init_start_time();
    pdf_printf("/CreationDate (%s)\n", start_time_str);
}

void print_mod_date()
{
    init_start_time();
    pdf_printf("/ModDate (%s)\n", start_time_str);
}

void getcreationdate()
{
    /* put creation date on top of string pool and update pool_ptr */
    size_t len = strlen(start_time_str);

    init_start_time();

    if ((unsigned) (pool_ptr + len) >= (unsigned) pool_size) {
        pool_ptr = pool_size;
        /* error by str_toks that calls str_room(1) */
        return;
    }

    memcpy(&str_pool[pool_ptr], start_time_str, len);
    pool_ptr += len;
}
