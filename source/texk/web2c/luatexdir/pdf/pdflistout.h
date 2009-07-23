/* pdflistout.h

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

#ifndef PDFLISTOUT_H
#  define PDFLISTOUT_H

/* a few small helpers */
#  define pos_right(A) pdf->posstruct->pos.h = pdf->posstruct->pos.h + (A)
#  define pos_left(A)  pdf->posstruct->pos.h = pdf->posstruct->pos.h - (A)
#  define pos_up(A)    pdf->posstruct->pos.v = pdf->posstruct->pos.v + (A)
#  define pos_down(A)  pdf->posstruct->pos.v = pdf->posstruct->pos.v - (A)

extern void init_pdf_output_functions(PDF pdf);
extern void init_dvi_output_functions(PDF pdf);
extern scaled simple_advance_width(halfword p);
extern void calculate_width_to_enddir(halfword p, real cur_glue, scaled cur_g,
                                      halfword this_box, scaled * setw,
                                      halfword * settemp_ptr);
extern void hlist_out(PDF pdf);
extern void vlist_out(PDF pdf);

/**********************************************************************/

typedef void (*node_output_function) ();
typedef void (*whatsit_output_function) ();

extern node_output_function backend_out[];
extern whatsit_output_function backend_out_whatsit[];

#endif
