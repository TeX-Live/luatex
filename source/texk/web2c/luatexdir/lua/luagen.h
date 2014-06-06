/* luagen.h
   
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

/* $Id$ */

extern void lua_begin_page(PDF pdf);
extern void lua_end_page(PDF pdf);

extern void lua_place_glyph(PDF pdf, internal_font_number f, int c, int ex);
extern void lua_place_rule(PDF pdf, scaledpos size);
extern void finish_lua_file(PDF pdf);
