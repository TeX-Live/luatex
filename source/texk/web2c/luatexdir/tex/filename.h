/* filename.h
   
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

#ifndef FILENAME_H
#  define FILENAME_H

extern void scan_file_name(void);
extern void scan_file_name_toks(void);
extern void pack_job_name(char *s);
extern void prompt_file_name(char *s, char *e);
extern str_number make_name_string(void);
extern void print_file_name(str_number, str_number, str_number);


#endif
