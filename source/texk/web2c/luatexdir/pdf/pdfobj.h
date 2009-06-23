/* pdfobj.h

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

#ifndef PDFOBJ_H
#  define PDFOBJ_H

/* data structure for \.{\\pdfobj} and \.{\\pdfrefobj} */

#  define pdfmem_obj_size          4    /* size of memory in |pdf_mem| which |obj_data_ptr| holds */

#  define obj_obj_data(A)          pdf_mem[obj_data_ptr(A) + 0] /* object data */
#  define obj_obj_is_stream(A)     pdf_mem[obj_data_ptr(A) + 1] /* will this object
                                                                   be written as a stream instead of a dictionary? */
#  define obj_obj_stream_attr(A)   pdf_mem[obj_data_ptr(A) + 2] /* additional
                                                                   object attributes for streams */
#  define obj_obj_is_file(A)       pdf_mem[obj_data_ptr(A) + 3] /* data should be
                                                                   read from an external file? */

#  define set_obj_obj_is_stream(A,B) obj_obj_is_stream(A)=B
#  define set_obj_obj_stream_attr(A,B) obj_obj_stream_attr(A)=B
#  define set_obj_obj_is_file(A,B) obj_obj_is_file(A)=B
#  define set_obj_obj_data(A,B) obj_obj_data(A)=B
#  define set_pdf_obj_objnum(A,B) pdf_obj_objnum(A)=B

extern void pdf_write_obj(PDF pdf, integer n);


#endif
