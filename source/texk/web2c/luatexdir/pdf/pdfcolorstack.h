/* pdfcolorstack.h

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

#ifndef PDFCOLORSTACK_H
#  define PDFCOLORSTACK_H

#  define set_pdf_colorstack_stack(A,B) pdf_colorstack_stack(A)=B
#  define set_pdf_colorstack_cmd(A,B) pdf_colorstack_cmd(A)=B
#  define set_pdf_colorstack_data(A,B) pdf_colorstack_data(A)=B

/* remember shipout mode: page/form */
extern boolean page_mode;

#  define STACK_INCREMENT 8

int newcolorstack(integer s, integer literal_mode, boolean pagestart);
int colorstackused();
integer colorstackset(int colstack_no, integer s);
integer colorstackpush(int colstack_no, integer s);
integer colorstackpop(int colstack_no);
integer colorstackcurrent(int colstack_no);
integer colorstackskippagestart(int colstack_no);
void colorstackpagestart(void);

extern void pdf_out_colorstack(halfword p);
extern void pdf_out_colorstack_startpage(void);

#endif
