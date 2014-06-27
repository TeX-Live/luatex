% pdfxform.w
%
% Copyright 2009-2011 Taco Hoekwater <taco@@luatex.org>
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
    "$Id: pdfxform.w 5022 2014-06-06 19:22:31Z oneiros $"
    "$URL: https://foundry.supelec.fr/svn/luatex/branches/experimental/source/texk/web2c/luatexdir/pdf/pdfxform.w $";

#include "ptexlib.h"

@ @c
#include "pdf/pdfpage.h"

@ @c
#define box(A) eqtb[box_base+(A)].hh.rh

int pdf_cur_form;               /* the form being output */

void pdf_place_form(PDF pdf, halfword p)
{
    scaled_whd nat, tex;
    scaled x, y;
    pdffloat cm[6];
    pdfstructure *q = pdf->pstruct;
    int r = 6, objnum = pdf_xform_objnum(p);
    nat.wd = obj_xform_width(pdf, objnum);
    nat.ht = obj_xform_height(pdf, objnum);
    nat.dp = obj_xform_depth(pdf, objnum);
    /* no transform yet */
    tex.wd = width(p);
    tex.ht = height(p);
    tex.dp = depth(p);
    if (nat.wd != tex.wd || nat.ht != tex.ht || nat.dp != tex.dp) {
        x = ext_xn_over_d(ten_pow[r], tex.wd, nat.wd);
        y = ext_xn_over_d(ten_pow[r], tex.dp + tex.ht, nat.dp + nat.ht);
    } else
        x = y = ten_pow[r];
    setpdffloat(cm[0], x, r);
    setpdffloat(cm[1], 0, r);
    setpdffloat(cm[2], 0, r);
    setpdffloat(cm[3], y, r);
    pdf_goto_pagemode(pdf);
    (void) calc_pdfpos(q, pdf->posstruct->pos);
    cm[4] = q->cm[4];
    cm[5] = q->cm[5];
    pdf_puts(pdf, "q\n");
    pdf_print_cm(pdf, cm);
    pdf_printf(pdf, "/Fm%d", (int) obj_info(pdf, objnum));
    pdf_print_resname_prefix(pdf);
    pdf_puts(pdf, " Do\nQ\n");
    addto_page_resources(pdf, obj_type_xform, objnum);
}

@ @c
void scan_pdfxform(PDF pdf)
{
    int k;
    halfword p;
    pdf->xform_count++;
    k = pdf_create_obj(pdf, obj_type_xform, pdf->xform_count);
    set_obj_data_ptr(pdf, k, pdf_get_mem(pdf, pdfmem_xform_size));
    if (scan_keyword("attr")) {
        scan_pdf_ext_toks();
        set_obj_xform_attr(pdf, k, def_ref);
    } else {
        set_obj_xform_attr(pdf, k, null);
    }
    if (scan_keyword("resources")) {
        scan_pdf_ext_toks();
        set_obj_xform_resources(pdf, k, def_ref);
    } else {
        set_obj_xform_resources(pdf, k, null);
    }
    scan_int();
    p = box(cur_val);
    if (p == null)
        pdf_error("ext1", "\\pdfxform cannot be used with a void box");
    set_obj_xform_box(pdf, k, p);       /* save pointer to the box */
    set_obj_xform_width(pdf, k, width(p));
    set_obj_xform_height(pdf, k, height(p));
    set_obj_xform_depth(pdf, k, depth(p));
    box(cur_val) = null;
    pdf_last_xform = k;
}

@ @c
#define tail          cur_list.tail_field

void scan_pdfrefxform(PDF pdf)
{
    int transform = 0;
    scaled_whd alt_rule, dim, nat;
    alt_rule = scan_alt_rule(); /* scans |<rule spec>| to |alt_rule| */
    scan_int();
    check_obj_type(pdf, obj_type_xform, cur_val);
    new_whatsit(pdf_refxform_node);
    nat.wd = obj_xform_width(pdf, cur_val);
    nat.ht = obj_xform_height(pdf, cur_val);
    nat.dp = obj_xform_depth(pdf, cur_val);
    if (alt_rule.wd != null_flag || alt_rule.ht != null_flag
        || alt_rule.dp != null_flag) {
        dim = tex_scale(nat, alt_rule);
    } else {
        dim = nat;
    }
    width(tail) = dim.wd;
    height(tail) = dim.ht;
    depth(tail) = dim.dp;
    pdf_xform_transform(tail) = transform;      /* not implemented yet */
    pdf_xform_objnum(tail) = cur_val;
}
