% writetype0.w
%
% Copyright 2006-2008 Taco Hoekwater <taco@@luatex.org>
%
% This file is part of LuaTeX.
%
% LuaTeX is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free
% Software Foundation; either version 2 of the License, or (at your
% option) any later version.
%
% LuaTeX is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
% FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License for more details.
%
% You should have received a copy of the GNU General Public License along
% with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

@ @c
static const char _svn_version[] =
    "$Id: writetype0.w 5016 2014-06-05 14:06:34Z oneiros $"
    "$URL: https://foundry.supelec.fr/svn/luatex/branches/experimental/source/texk/web2c/luatexdir/font/writetype0.w $";

#include "ptexlib.h"
#include "font/writettf.h"
#include "font/writecff.h"

@ @c
void writetype0(PDF pdf, fd_entry * fd)
{
    int callback_id;
    int file_opened = 0;
    long i;
    dirtab_entry *tab;
    cff_font *cff;

    dir_tab = NULL;
    glyph_tab = NULL;

    fd_cur = fd;                /* |fd_cur| is global inside \.{writettf.w} */
    assert(fd_cur->fm != NULL);
    assert(is_opentype(fd_cur->fm));
    assert(is_included(fd_cur->fm));

    ttf_curbyte = 0;
    ttf_size = 0;
    cur_file_name =
        luatex_find_file(fd_cur->fm->ff_name, find_opentype_file_callback);
    if (cur_file_name == NULL) {
        luatex_fail("cannot find OpenType font file for reading (%s)",
                    fd_cur->fm->ff_name);
    }
    callback_id = callback_defined(read_opentype_file_callback);
    if (callback_id > 0) {
        if (run_callback(callback_id, "S->bSd", cur_file_name,
                         &file_opened, &ttf_buffer, &ttf_size) &&
            file_opened && ttf_size > 0) {
        } else {
            luatex_fail("cannot open OpenType font file for reading (%s)",
                        cur_file_name);
        }
    } else {
        if (!otf_open(cur_file_name)) {
            luatex_fail("cannot open OpenType font file for reading (%s)",
                        cur_file_name);
        }
        ttf_read_file();
        ttf_close();
    }

    fd_cur->ff_found = true;

    if (is_subsetted(fd_cur->fm)) {
        report_start_file(filetype_subset, cur_file_name);
    } else {
        report_start_file(filetype_font, cur_file_name);
    }
    ttf_read_tabdir();
    /* read font parameters */
    if (ttf_name_lookup("head", false) != NULL)
        ttf_read_head();
    if (ttf_name_lookup("hhea", false) != NULL)
        ttf_read_hhea();
    if (ttf_name_lookup("PCLT", false) != NULL)
        ttf_read_pclt();
    if (ttf_name_lookup("post", false) != NULL)
        ttf_read_post();

    /* copy font file */
    tab = ttf_seek_tab("CFF ", 0);

    /* TODO the next 0 is a subfont index */
    cff = read_cff(ttf_buffer + ttf_curbyte, (long) tab->length, 0);
    if (!is_subsetted(fd_cur->fm)) {
        /* not subsetted, just do a copy */
        for (i = (long) tab->length; i > 0; i--)
            strbuf_putchar(pdf->fb, (unsigned char) ttf_getnum(1));
    } else {
        if (cff != NULL) {
            if (cff_is_cidfont(cff)) {
                write_cid_cff(pdf, cff, fd_cur);
#if 0
                for (i = tab->length; i > 0; i--)
                    strbuf_putchar(pdf->fb, (unsigned char) ttf_getnum(1));
#endif
            } else {
                write_cff(pdf, cff, fd_cur);
            }
        } else {
            /* not understood, just do a copy */
            for (i = (long) tab->length; i > 0; i--)
                strbuf_putchar(pdf->fb, (unsigned char) ttf_getnum(1));
        }
    }
    xfree(dir_tab);
    xfree(ttf_buffer);
    if (is_subsetted(fd_cur->fm)) {
        report_stop_file(filetype_subset);
    } else {
        report_stop_file(filetype_font);
    }
    cur_file_name = NULL;
}
