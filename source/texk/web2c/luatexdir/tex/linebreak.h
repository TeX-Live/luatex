/* linebreak.h
   
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

#ifndef LINEBREAK_H
#  define LINEBREAK_H

#  define left_side 0
#  define right_side 1

extern halfword just_box;       /* the |hlist_node| for the last line of the new paragraph */

extern void line_break(boolean d, int line_break_context);

#  define inf_bad 10000         /* infinitely bad value */
#  define awful_bad 07777777777 /* more than a billion demerits */

extern void initialize_active(void);

extern void ext_do_line_break(boolean d,
                              int paragraph_dir,
                              int line_break_dir,
                              int pretolerance,
                              int tracing_paragraphs,
                              int tolerance,
                              scaled emergency_stretch,
                              int looseness,
                              int hyphen_penalty,
                              int ex_hyphen_penalty,
                              int pdf_adjust_spacing,
                              halfword par_shape_ptr,
                              int adj_demerits,
                              int pdf_protrude_chars,
                              int line_penalty,
                              int last_line_fit,
                              int double_hyphen_demerits,
                              int final_hyphen_demerits,
                              int hang_indent,
                              int hsize,
                              int hang_after,
                              halfword left_skip,
                              halfword right_skip,
                              int pdf_each_line_height,
                              int pdf_each_line_depth,
                              int pdf_first_line_height,
                              int pdf_last_line_depth,
                              halfword inter_line_penalties_ptr,
                              int inter_line_penalty,
                              int club_penalty,
                              halfword club_penalties_ptr,
                              halfword display_widow_penalties_ptr,
                              halfword widow_penalties_ptr,
                              int display_widow_penalty,
                              int widow_penalty,
                              int broken_penalty, halfword final_par_glue,
                              halfword pdf_ignored_dimen);

halfword find_protchar_left(halfword l, boolean d);
halfword find_protchar_right(halfword l, halfword r);

#endif
