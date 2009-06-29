/* pdfxform.h

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

#ifndef PDFXFORM_H
#  define PDFXFORM_H

/* data structure for \.{\\pdfxform} and \.{\\pdfrefxform} */

#  define pdfmem_xform_size        6    /* size of memory in |pdf->mem| which |obj_data_ptr| holds */

#  define set_pdf_xform_objnum(A,B) pdf_xform_objnum(A)=B

#  define obj_xform_width(pdf,A)       pdf->mem[obj_data_ptr(pdf,A) + 0]
#  define obj_xform_height(pdf,A)      pdf->mem[obj_data_ptr(pdf,A) + 1]
#  define obj_xform_depth(pdf,A)       pdf->mem[obj_data_ptr(pdf,A) + 2]
#  define obj_xform_box(pdf,A)         pdf->mem[obj_data_ptr(pdf,A) + 3]    /* this field holds
                                                                           pointer to the corresponding box */
#  define obj_xform_attr(pdf,A)        pdf->mem[obj_data_ptr(pdf,A) + 4]    /* additional xform
                                                                           attributes */
#  define obj_xform_resources(pdf,A)   pdf->mem[obj_data_ptr(pdf,A) + 5]    /* additional xform
                                                                           Resources */


#  define set_obj_xform_width(pdf,A,B) obj_xform_width(pdf,A)=B
#  define set_obj_xform_height(pdf,A,B) obj_xform_height(pdf,A)=B
#  define set_obj_xform_depth(pdf,A,B) obj_xform_depth(pdf,A)=B
#  define set_obj_xform_box(pdf,A,B) obj_xform_box(pdf,A)=B
#  define set_obj_xform_attr(pdf,A,B) obj_xform_attr(pdf,A)=B
#  define set_obj_xform_resources(pdf,A,B) obj_xform_resources(pdf,A)=B

extern halfword pdf_xform_list; /* list of forms in the current page */
extern integer pdf_xform_count; /* counter of forms */
extern integer pdf_cur_form;    /* the form being output */

void pdf_place_form(PDF pdf, integer i, scaledpos pos);

void scan_pdfxform(PDF pdf, integer box_base);
void scan_pdfrefxform(PDF pdf);

#endif
