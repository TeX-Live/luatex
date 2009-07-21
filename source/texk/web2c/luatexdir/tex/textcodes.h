/* textcodes.h
   
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

#ifndef TEXTCODES_H
#  define TEXTCODES_H


void set_lc_code(integer n, halfword v, quarterword gl);
halfword get_lc_code(integer n);
void set_uc_code(integer n, halfword v, quarterword gl);
halfword get_uc_code(integer n);
void set_sf_code(integer n, halfword v, quarterword gl);
halfword get_sf_code(integer n);
void set_cat_code(integer h, integer n, halfword v, quarterword gl);
halfword get_cat_code(integer h, integer n);
void unsave_cat_codes(integer h, quarterword gl);
int valid_catcode_table(int h);
void initex_cat_codes(int h);
void unsave_text_codes(quarterword grouplevel);
void initialize_text_codes(void);
void dump_text_codes(void);
void undump_text_codes(void);
void copy_cat_codes(int from, int to);
void free_math_codes(void);
void free_text_codes(void);


#endif
