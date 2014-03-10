/* luatexcallbackids.h

   Copyright 2012 Taco Hoekwater <taco@luatex.org>

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

/* $Id: luatexcallbackids.h 4544 2012-12-25 14:07:44Z oneiros $ */

#ifndef LUATEXCALLBACKIDS_H
#define LUATEXCALLBACKIDS_H

typedef enum {
    find_write_file_callback = 1,
    find_output_file_callback,
    find_image_file_callback,
    find_format_file_callback,
    find_read_file_callback, open_read_file_callback,
    find_vf_file_callback, read_vf_file_callback,
    find_data_file_callback, read_data_file_callback,
    find_font_file_callback, read_font_file_callback,
    find_map_file_callback, read_map_file_callback,
    find_enc_file_callback, read_enc_file_callback,
    find_type1_file_callback, read_type1_file_callback,
    find_truetype_file_callback, read_truetype_file_callback,
    find_opentype_file_callback, read_opentype_file_callback,
    find_sfd_file_callback, read_sfd_file_callback,
    find_cidmap_file_callback, read_cidmap_file_callback,
    find_pk_file_callback, read_pk_file_callback,
    show_error_hook_callback,
    process_input_buffer_callback, process_output_buffer_callback,
    process_jobname_callback,
    start_page_number_callback, stop_page_number_callback,
    start_run_callback, stop_run_callback,
    define_font_callback,
    token_filter_callback,
    pre_output_filter_callback,
    buildpage_filter_callback,
    hpack_filter_callback, vpack_filter_callback,
    char_exists_callback,
    hyphenate_callback,
    ligaturing_callback,
    kerning_callback,
    pre_linebreak_filter_callback,
    linebreak_filter_callback,
    post_linebreak_filter_callback,
    mlist_to_hlist_callback,
    finish_pdffile_callback,
    finish_pdfpage_callback,
    pre_dump_callback,
    total_callbacks
} callback_callback_types;

/* lcallbacklib.c */

extern int callback_set[];

#  define callback_defined(a) callback_set[a]
/* #  define callback_defined(a) debug_callback_defined(a) */

extern int lua_active;

extern int debug_callback_defined(int i);

extern int run_callback(int i, const char *values, ...);
extern int run_saved_callback(int i, const char *name, const char *values, ...);
extern int run_and_save_callback(int i, const char *values, ...);
extern void destroy_saved_callback(int i);

extern void get_saved_lua_boolean(int i, const char *name, boolean * target);
extern void get_saved_lua_number(int i, const char *name, int *target);
extern void get_saved_lua_string(int i, const char *name, char **target);

extern void get_lua_boolean(const char *table, const char *name,
                            boolean * target);
extern void get_lua_number(const char *table, const char *name, int *target);
extern void get_lua_string(const char *table, const char *name, char **target);

extern char *get_lua_name(int i);

/* texfileio.c */
extern char *luatex_find_file(const char *s, int callback_index);
extern int readbinfile(FILE * f, unsigned char **b, int *s);


#endif
