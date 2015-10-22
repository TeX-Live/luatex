/* pdftables.h

   Copyright 2009-2013 Taco Hoekwater <taco@luatex.org>

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


#ifndef PDFTABLES_H
#  define PDFTABLES_H

const char *pdf_obj_typenames[PDF_OBJ_TYPE_MAX + 1];

typedef enum {
    union_type_int,
    union_type_cstring,
} union_type;

typedef struct {
    union {
        int int0;
        char *str0;
    } u;
    union_type u_type;          /* integer or char * in union above */
    int objptr;
} oentry;

/*
The cross-reference table |obj_tab| is an array of |obj_tab_size| of
|obj_entry|. Each entry contains five integer fields and represents an object
in PDF file whose object number is the index of this entry in |obj_tab|.
Objects in |obj_tab| maybe linked into list; objects in such a linked list have
the same type.
*/

/*
The first field contains information representing identifier of this object.
It is usally a number for most of object types, but it may be a string number
for named destination or named thread.

The second field of |obj_entry| contains link to the next
object in |obj_tab| if this object is linked in a list.

The third field holds the byte offset of the object in the output PDF file,
or its byte offset within an object stream. As long as the object is not
written, this field is used for flags about the write status of the object;
then it has a negative value.

The fourth field holds the object number of the object stream, into which
the object is included.

The last field usually represents the pointer to some auxiliary data
structure depending on the object type; however it may be used as a counter as
well.
*/

#  define obj_info(pdf,A)      pdf->obj_tab[(A)].u.int0 /* information representing identifier of this object */
#  define obj_start(pdf,A)     pdf->obj_tab[(A)].u.str0
#  define obj_link(pdf,A)      pdf->obj_tab[(A)].int1   /* link to the next entry in linked list */

#  define obj_offset(pdf,A)    pdf->obj_tab[(A)].int2   /* negative (flags), or byte offset for this object in PDF
                                                           output file, or ... */
#  define obj_os_objnum(pdf,A) pdf->obj_tab[(A)].int2   /* ... object stream number for this object */
#  define obj_os_idx(pdf,A)    pdf->obj_tab[(A)].int3   /* index of this object in object stream */
#  define obj_aux(pdf,A)       pdf->obj_tab[(A)].v.int4 /* auxiliary pointer */
#  define obj_stop(pdf,A)      pdf->obj_tab[(A)].v.str4
#  define obj_type(pdf,A)      pdf->obj_tab[(A)].objtype

#  define obj_data_ptr               obj_aux    /* pointer to |pdf->mem| */

#  define set_obj_link(pdf,A,B)      obj_link(pdf,A)=(B)
#  define set_obj_start(pdf,A,B)     obj_start(pdf,A)=(B)
#  define set_obj_info(pdf,A,B)      obj_info(pdf,A)=(B)
#  define set_obj_offset(pdf,A,B)    obj_offset(pdf,A)=(B)
#  define set_obj_os_objnum(pdf,A,B) obj_offset(pdf,A)=(B)
#  define set_obj_aux(pdf,A,B)       obj_aux(pdf,A)=(B)
#  define set_obj_stop(pdf,A,B)      obj_stop(pdf,A)=(B)
#  define set_obj_data_ptr(pdf,A,B)  obj_data_ptr(pdf,A)=(B)

#  define set_obj_fresh(pdf,A)       obj_offset(pdf,(A))=(off_t)-2
#  define set_obj_scheduled(pdf,A)   if (obj_offset(pdf,A)==(off_t)-2) obj_offset(pdf,A)=(off_t)-1
#  define is_obj_scheduled(pdf,A)    ((obj_offset(pdf,A))>(off_t)-2)
#  define is_obj_written(pdf,A)      ((obj_offset(pdf,A))>(off_t)-1)

/*  NOTE: The data structure definitions for the nodes on the typesetting side are
    inside |nodes.h| */

/* Some constants */
#  define inf_pk_dpi 72         /* min PK pixel density value from \.{texmf.cnf} */
#  define sup_pk_dpi 8000       /* max PK pixel density value from \.{texmf.cnf} */

extern int find_obj(PDF pdf, int t, int i, boolean byname);
extern void check_obj_exists(PDF pdf, int objnum);
extern void check_obj_type(PDF pdf, int t, int objnum);
extern int pdf_get_obj(PDF pdf, int t, int i, boolean byname);
extern int pdf_create_obj(PDF pdf, int t, int i);

extern void set_rect_dimens(PDF pdf, halfword p, halfword parent_box,
                            scaledpos cur, scaled_whd alt_rule, scaled margin);

extern void libpdffinish(PDF);

extern void dump_pdftex_data(PDF pdf);
extern void undump_pdftex_data(PDF pdf);

#  define set_width(A, B) width(A) = (B)
#  define set_height(A, B) height(A) = (B)
#  define set_depth(A, B) depth(A) = (B)

/* pdf backend definitions */

#  define pdf_compress_level       get_tex_extension_count_register("pdf_compresslevel",        9)
#  define pdf_decimal_digits       get_tex_extension_count_register("pdf_decimaldigits",         3)
#  define pdf_image_resolution     get_tex_extension_count_register("pdf_imageresolution",      72)
#  define pdf_pk_resolution        get_tex_extension_count_register("pdf_pkresolution",         72)
#  define pdf_unique_resname       get_tex_extension_count_register("pdf_uniqueresname",         0)
#  define pdf_minor_version        get_tex_extension_count_register("pdf_minorversion",          4)
#  define pdf_pagebox              get_tex_extension_count_register("pdf_pagebox",               0)
#  define pdf_inclusion_errorlevel get_tex_extension_count_register("pdf_inclusionerrorlevel",   0)
#  define pdf_gamma                get_tex_extension_count_register("pdf_gamma",              1000)
#  define pdf_image_apply_gamma    get_tex_extension_count_register("pdf_imageapplygamma",       0)
#  define pdf_image_gamma          get_tex_extension_count_register("pdf_imagegamma",         2200)
#  define pdf_image_hicolor        get_tex_extension_count_register("pdf_imagehicolor",          1)
#  define pdf_image_addfilename    get_tex_extension_count_register("pdf_imageaddfilename",      1)
#  define pdf_objcompresslevel     get_tex_extension_count_register("pdf_objcompresslevel",      0)
#  define pdf_inclusion_copy_font  get_tex_extension_count_register("pdf_inclusioncopyfonts",    0)
#  define pdf_gen_tounicode        get_tex_extension_count_register("pdf_gentounicode",          0)
#  define pdf_replace_font         get_tex_extension_count_register("pdf_replacefont",           0)

#  define pdf_h_origin             get_tex_extension_dimen_register("pdf_horigin",        one_inch)
#  define pdf_v_origin             get_tex_extension_dimen_register("pdf_vorigin",        one_inch)
#  define pdf_thread_margin        get_tex_extension_dimen_register("pdf_threadmargin",          0)
#  define pdf_dest_margin          get_tex_extension_dimen_register("pdf_destmargin",            0)
#  define pdf_link_margin          get_tex_extension_dimen_register("pdf_linkmargin",            0)

#  define pdf_page_attr            get_tex_extension_toks_register("pdf_pageattr")
#  define pdf_page_resources       get_tex_extension_toks_register("pdf_pageresources")
#  define pdf_pages_attr           get_tex_extension_toks_register("pdf_pagesattr")
#  define pdf_xform_attr           get_tex_extension_toks_register("pdf_xformattr")
#  define pdf_xform_resources      get_tex_extension_toks_register("pdf_xformresources")
#  define pdf_pk_mode              get_tex_extension_toks_register("pdf_pkmode")

#  define set_pdf_minor_version(i)    set_tex_extension_count_register("pdf_minorversion",i)
#  define set_pdf_compress_level(i)   set_tex_extension_count_register("pdf_compresslevel",i)
#  define set_pdf_objcompresslevel(i) set_tex_extension_count_register("pdf_objcompresslevel",i)

#endif                          /* PDFTABLES_H */
