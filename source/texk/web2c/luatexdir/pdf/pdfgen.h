/* pdfgen.h

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

#ifndef PDFGEN_H
#  define PDFGEN_H

#  define inf_pdf_mem_size 10000        /* min size of the |mem| array */
#  define sup_pdf_mem_size 10000000     /* max size of the |mem| array */

extern PDF static_pdf;

extern void initialize_pdfgen(void);
extern PDF initialize_pdf(void);

extern integer pdf_get_mem(PDF pdf, integer s);

/*
We use the similiar subroutines to handle the output buffer for
PDF output. When compress is used, the state of writing to buffer
is held in |zip_write_state|. We must write the header of PDF
output file in initialization to ensure that it will be the first
written bytes.
*/

#  define inf_pdf_op_buf_size 16384     /* size of the PDF output buffer */
#  define sup_pdf_op_buf_size 16384     /* size of the PDF output buffer */
#  define inf_pdf_os_buf_size 1 /* initial value of |pdf_os_buf_size| */
#  define sup_pdf_os_buf_size 5000000   /* arbitrary upper hard limit of |pdf_os_buf_size| */
#  define pdf_os_max_objs 100   /* maximum number of objects in object stream */

#  define inf_obj_tab_size 1000 /* min size of the cross-reference table for PDF output */
#  define sup_obj_tab_size 8388607      /* max size of the cross-reference table for PDF output */

/* The following macros are similar as for \.{DVI} buffer handling */

#  define pdf_offset(pdf) (pdf->gone + pdf->ptr)
                                        /* the file offset of last byte in PDF
                                           buffer that |pdf_ptr| points to */
#  define pdf_save_offset(pdf) pdf->save_offset=(pdf->gone + pdf->ptr)
#  define pdf_saved_offset(pdf) pdf->save_offset

#  define set_ff(A)  do {                       \
        if (pdf_font_num(A) < 0)                \
            ff = -pdf_font_num(A);              \
        else                                    \
            ff = A;                             \
    } while (0)

typedef enum {
    no_zip = 0,                 /* no \.{ZIP} compression */
    zip_writing = 1,            /* \.{ZIP} compression being used */
    zip_finish = 2              /* finish \.{ZIP} compression */
} zip_write_states;

extern integer pdf_output_option;
extern integer pdf_output_value;
extern integer pdf_draftmode_option;
extern integer pdf_draftmode_value;

extern integer fixed_pdfoutput;
extern boolean fixed_pdfoutput_set;

extern scaled one_hundred_inch;
extern scaled one_inch;
extern scaled one_true_inch;
extern scaled one_hundred_bp;
extern scaled one_bp;
extern integer ten_pow[10];

extern void pdf_flush(PDF);
extern void pdf_room(PDF, integer);

#  define check_pdfminorversion(A) do {          \
    if (A==NULL) {                             \
      static_pdf = initialize_pdf();           \
      A = static_pdf;                          \
    }                                          \
    do_check_pdfminorversion(A);               \
  } while (0)

extern void do_check_pdfminorversion(PDF);

extern void ensure_pdf_open(PDF);

 /* output a byte to PDF buffer without checking of overflow */
#  define pdf_quick_out(pdf,A) pdf->buf[pdf->ptr++]=A

/* do the same as |pdf_quick_out| and flush the PDF buffer if necessary */
#  define pdf_out(pdf,A) do { pdf_room(pdf,1); pdf_quick_out(pdf,A); } while (0)

/*
Basic printing procedures for PDF output are very similiar to \TeX\ basic
printing ones but the output is going to PDF buffer. Subroutines with
suffix |_ln| append a new-line character to the PDF output.
*/

#  define pdf_new_line_char 10  /* new-line character for UNIX platforms */

/* output a new-line character to PDF buffer */
#  define pdf_print_nl(pdf) pdf_out(pdf,pdf_new_line_char)

/* print out a string to PDF buffer followed by a new-line character */
#  define pdf_print_ln(pdf,A) do {                 \
        pdf_print(pdf,A);                          \
        pdf_print_nl(pdf);                         \
    } while (0)

/* print out an integer to PDF buffer followed by a new-line character */
#  define pdf_print_int_ln(pdf,A) do {            \
        pdf_print_int(pdf,A);                     \
        pdf_print_nl(pdf);                        \
    } while (0)

extern void pdf_puts(PDF, const char *);
extern __attribute__ ((format(printf, 2, 3)))
void pdf_printf(PDF, const char *, ...);

extern void pdf_print_char(PDF, int);
extern void pdf_print_wide_char(PDF, int);
extern void pdf_print(PDF, str_number);
extern void pdf_print_int(PDF, longinteger);
extern void pdf_print_real(PDF, integer, integer);
extern void pdf_print_str(PDF, char *);

extern void pdf_begin_stream(PDF);
extern void pdf_end_stream(PDF);
extern void pdf_remove_last_space(PDF);

extern void pdf_print_bp(PDF, scaled);
extern void pdf_print_mag_bp(PDF, scaled);


/* This is for the resource lists */

extern void append_object_list(PDF pdf, pdf_obj_type t, integer f);
extern void flush_object_list(PDF pdf, pdf_obj_type t);
extern pdf_object_list *lookup_object_list(PDF pdf, pdf_obj_type t, integer f);

#  define set_ff(A)  do {                         \
        if (pdf_font_num(A) < 0)                \
            ff = -pdf_font_num(A);              \
        else                                    \
            ff = A;                             \
    } while (0)

#  define pdf_print_resname_prefix(pdf) do {        \
        if (pdf->resname_prefix != NULL)            \
            pdf_puts(pdf,pdf->resname_prefix);      \
    } while (0)

extern void pdf_print_fw_int(PDF, longinteger, integer);
extern void pdf_out_bytes(PDF, longinteger, integer);
extern void pdf_int_entry(PDF, char *, integer);
extern void pdf_int_entry_ln(PDF, char *, integer);
extern void pdf_indirect(PDF, char *, integer);
extern void pdf_indirect_ln(PDF, char *, integer);
extern void pdf_print_str_ln(PDF, char *);
extern void pdf_str_entry(PDF, char *, char *);
extern void pdf_str_entry_ln(PDF, char *, char *);

extern void pdf_print_toks(PDF, halfword);
extern void pdf_print_toks_ln(PDF, halfword);

extern void pdf_print_rect_spec(PDF, halfword);
extern void pdf_rectangle(PDF, halfword);

extern void pdf_begin_obj(PDF, integer, integer);
extern void pdf_new_obj(PDF, integer, integer, integer);
extern void pdf_end_obj(PDF);

extern void pdf_begin_dict(PDF, integer, integer);
extern void pdf_new_dict(PDF, integer, integer, integer);
extern void pdf_end_dict(PDF);

extern void pdf_os_switch(PDF pdf, boolean pdf_os);
extern void pdf_os_prepare_obj(PDF pdf, integer i, integer pdf_os_level);
extern void pdf_os_write_objstream(PDF);

extern void write_stream_length(PDF, integer, longinteger);

extern void print_creation_date(PDF);
extern void print_mod_date(PDF);
extern void print_ID(PDF, char *);

extern void remove_pdffile(PDF);

extern integer fb_offset(PDF);
extern void fb_flush(PDF);
extern void fb_putchar(PDF, eight_bits);
extern void fb_seek(PDF, integer);
extern void fb_free(PDF);

extern void write_zip(PDF, boolean);
extern void zip_free(PDF);

/* functions that do not output stuff */

extern scaled round_xn_over_d(scaled x, integer n, integer d);
extern char *convertStringToPDFString(const char *in, int len);

extern void init_start_time(PDF);
extern char *getcreationdate(PDF);

extern void pdf_error(char *t, char *p);
extern void pdf_warning(char *t, char *p, boolean pr, boolean ap);
extern void check_pdfoutput(char *s, boolean is_error);

extern void set_job_id(PDF, int, int, int, int);
extern char *get_resname_prefix(PDF);

#endif
