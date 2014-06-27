/* pdflua.h

   Copyright 2010 Taco Hoekwater <taco@luatex.org>
   Copyright 2010 Hartmut Henkel <hartmut@luatex.org>

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

/* $Id: pdflua.h 5022 2014-06-06 19:22:31Z oneiros $ */

#ifndef PDFLUA_H
#  define PDFLUA_H

typedef struct _zlib_struct {
    uLong uncomprLen;
    uLong comprLen;
    const Byte *compr;
} zlib_struct;

extern const zlib_struct *pdflua_zlib_struct_ptr;

#endif                          /* PDFLUA_H */
