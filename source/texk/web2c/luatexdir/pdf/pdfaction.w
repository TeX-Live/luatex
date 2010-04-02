% pdfaction.w
% 
% Copyright 2009-2010 Taco Hoekwater <taco@@luatex.org>

% This file is part of LuaTeX.

% LuaTeX is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free
% Software Foundation; either version 2 of the License, or (at your
% option) any later version.

% LuaTeX is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
% FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License for more details.

% You should have received a copy of the GNU General Public License along
% with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

@ @c
#include "ptexlib.h"

static const char _svn_version[] =
    "$Id$"
    "$URL$";


@ @c
halfword new_action_node(void)
{
    return new_node(action_node, 0);
}

@ @c
void delete_action_node(halfword a)
{
    if (pdf_action_type(a) == pdf_action_user) {
        delete_token_ref(pdf_action_tokens(a));
    } else {
        if (pdf_action_file(a) != null)
            delete_token_ref(pdf_action_file(a));
        if (pdf_action_type(a) == pdf_action_page)
            delete_token_ref(pdf_action_tokens(a));
        else if (pdf_action_named_id(a) > 0)
            delete_token_ref(pdf_action_id(a));
    }
    free_node(a, pdf_action_size);
}

@ read an action specification 
@c
halfword scan_action(PDF pdf)
{
    int p;
    (void) pdf;
    p = new_action_node();
    if (scan_keyword("user"))
        set_pdf_action_type(p, pdf_action_user);
    else if (scan_keyword("goto"))
        set_pdf_action_type(p, pdf_action_goto);
    else if (scan_keyword("thread"))
        set_pdf_action_type(p, pdf_action_thread);
    else
        pdf_error("ext1", "action type missing");
    if (pdf_action_type(p) == pdf_action_user) {
        scan_pdf_ext_toks();
        set_pdf_action_tokens(p, def_ref);
        return p;
    }
    if (scan_keyword("file")) {
        scan_pdf_ext_toks();
        set_pdf_action_file(p, def_ref);
    }
    if (scan_keyword("page")) {
        if (pdf_action_type(p) != pdf_action_goto)
            pdf_error("ext1", "only GoTo action can be used with `page'");
        set_pdf_action_type(p, pdf_action_page);
        scan_int();
        if (cur_val <= 0)
            pdf_error("ext1", "page number must be positive");
        set_pdf_action_id(p, cur_val);
        set_pdf_action_named_id(p, 0);
        scan_pdf_ext_toks();
        set_pdf_action_tokens(p, def_ref);
    } else if (scan_keyword("name")) {
        scan_pdf_ext_toks();
        set_pdf_action_named_id(p, 1);
        set_pdf_action_id(p, def_ref);
    } else if (scan_keyword("num")) {
        if ((pdf_action_type(p) == pdf_action_goto) &&
            (pdf_action_file(p) != null))
            pdf_error("ext1",
                      "`goto' option cannot be used with both `file' and `num'");
        scan_int();
        if (cur_val <= 0)
            pdf_error("ext1", "num identifier must be positive");
        set_pdf_action_named_id(p, 0);
        set_pdf_action_id(p, cur_val);
    } else {
        pdf_error("ext1", "identifier type missing");
    }
    if (scan_keyword("newwindow")) {
        set_pdf_action_new_window(p, pdf_window_new);
        /* Scan an optional space */
        get_x_token();
        if (cur_cmd != spacer_cmd)
            back_input();
    } else if (scan_keyword("nonewwindow")) {
        set_pdf_action_new_window(p, pdf_window_nonew);
        /* Scan an optional space */
        get_x_token();
        if (cur_cmd != spacer_cmd)
            back_input();
    } else {
        set_pdf_action_new_window(p, pdf_window_notset);
    }
    if ((pdf_action_new_window(p) > pdf_window_notset) &&
        (((pdf_action_type(p) != pdf_action_goto) &&
          (pdf_action_type(p) != pdf_action_page)) ||
         (pdf_action_file(p) == null)))
        pdf_error("ext1",
                  "`newwindow'/`nonewwindow' must be used with `goto' and `file' option");
    return p;
}

@ write an action specification
@c
void write_action(PDF pdf, halfword p)
{
    char *s;
    int d = 0;
    if (pdf_action_type(p) == pdf_action_user) {
        pdf_print_toks_ln(pdf, pdf_action_tokens(p));
        return;
    }
    pdf_printf(pdf, "<< ");
    if (pdf_action_file(p) != null) {
        pdf_printf(pdf, "/F ");
        s = tokenlist_to_cstring(pdf_action_file(p), true, NULL);
        pdf_print_str(pdf, s);
        xfree(s);
        pdf_printf(pdf, " ");
        if (pdf_action_new_window(p) > pdf_window_notset) {
            pdf_printf(pdf, "/NewWindow ");
            if (pdf_action_new_window(p) == pdf_window_new)
                pdf_printf(pdf, "true ");
            else
                pdf_printf(pdf, "false ");
        }
    }
    switch (pdf_action_type(p)) {
    case pdf_action_page:
        if (pdf_action_file(p) == null) {
            pdf_printf(pdf, "/S /GoTo /D [");
            pdf_print_int(pdf,
                          get_obj(pdf, obj_type_page, pdf_action_id(p), false));
            pdf_printf(pdf, " 0 R");
        } else {
            pdf_printf(pdf, "/S /GoToR /D [");
            pdf_print_int(pdf, pdf_action_id(p) - 1);
        }
        {
            char *tokstr =
                tokenlist_to_cstring(pdf_action_tokens(p), true, NULL);
            pdf_printf(pdf, " %s]", tokstr);
            xfree(tokstr);
        }
        break;
    case pdf_action_goto:
        if (pdf_action_file(p) == null) {
            pdf_printf(pdf, "/S /GoTo ");
            d = get_obj(pdf, obj_type_dest, pdf_action_id(p),
                        pdf_action_named_id(p));
        } else {
            pdf_printf(pdf, "/S /GoToR ");
        }
        if (pdf_action_named_id(p) > 0) {
            char *tokstr = tokenlist_to_cstring(pdf_action_id(p), true, NULL);
            pdf_str_entry(pdf, "D", tokstr);
            xfree(tokstr);
        } else if (pdf_action_file(p) == null) {
            pdf_indirect(pdf, "D", d);
        } else {
            pdf_error("ext4",
                      "`goto' option cannot be used with both `file' and `num'");
        }
        break;
    case pdf_action_thread:
        pdf_printf(pdf, "/S /Thread ");
        if (pdf_action_file(p) == null) {
            d = get_obj(pdf, obj_type_thread, pdf_action_id(p),
                        pdf_action_named_id(p));
            if (pdf_action_named_id(p) > 0) {
                char *tokstr =
                    tokenlist_to_cstring(pdf_action_id(p), true, NULL);
                pdf_str_entry(pdf, "D", tokstr);
                xfree(tokstr);
            } else if (pdf_action_file(p) == null) {
                pdf_indirect(pdf, "D", d);
            } else {
                pdf_int_entry(pdf, "D", pdf_action_id(p));
            }
        }
        break;
    }
    pdf_printf(pdf, " >>\n");
}
