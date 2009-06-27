/* pdfxform.c
   
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
#include "pdfpage.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

halfword pdf_xform_list;        /* list of forms in the current page */
integer pdf_xform_count;        /* counter of forms */
integer pdf_cur_form;           /* the form being output */

void output_form(PDF pdf, halfword p, scaledpos pos)
{
    pdf_goto_pagemode(pdf);
    pdf_place_form(pdf, obj_info(pdf_xform_objnum(p)), pos);
    if (pdf_lookup_list(pdf_xform_list, pdf_xform_objnum(p)) == null)
        pdf_append_list(pdf_xform_objnum(p), pdf_xform_list);
}

static void place_form(PDF pdf, pdfstructure * p, integer i, scaledpos pos)
{
    pdf_goto_pagemode(pdf);
    pdf_printf(pdf, "q\n");
    pdf_set_pos_temp(pdf, pos);
    pdf_printf(pdf, "/Fm%d", (int) i);
    pdf_print_resname_prefix(pdf);
    pdf_printf(pdf, " Do\nQ\n");
}

void pdf_place_form(PDF pdf, integer i, scaledpos pos)
{
    place_form(pdf, pstruct, i, pos);
}
