/* pdfdest.c
   
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

#include "ptexlib.h"

#include "commands.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#define pdf_dest_margin          dimen_par(param_pdf_dest_margin_code)

integer pdf_dest_names_ptr;     /* first unused position in |dest_names| */
dest_name_entry *dest_names;

integer dest_names_size = inf_dest_names_size;  /* maximum number of names in name tree of PDF output file */

/*
Here we implement subroutines for work with objects and related things.
Some of them are used in former parts too, so we need to declare them
forward.
*/

void init_dest_names(void)
{
    dest_names = xmallocarray(dest_name_entry, inf_dest_names_size);    /* will grow dynamically */
}

void append_dest_name(str_number s, integer n)
{
    integer a;
    if (pdf_dest_names_ptr == sup_dest_names_size)
        overflow("number of destination names (dest_names_size)",
                 dest_names_size);
    if (pdf_dest_names_ptr == dest_names_size) {
        a = 0.2 * dest_names_size;
        if (dest_names_size < sup_dest_names_size - a)
            dest_names_size = dest_names_size + a;
        else
            dest_names_size = sup_dest_names_size;
        dest_names =
            xreallocarray(dest_names, dest_name_entry, dest_names_size);
    }
    dest_names[pdf_dest_names_ptr].objname = s;
    dest_names[pdf_dest_names_ptr].objnum = n;
    incr(pdf_dest_names_ptr);
}

/*
When a destination is created we need to check whether another destination
with the same identifier already exists and give a warning if needed.
*/

void warn_dest_dup(integer id, small_number byname, char *s1, char *s2)
{
    pdf_warning(s1, "destination with the same identifier (", false, false);
    if (byname > 0) {
        tprint("name");
        print_mark(id);
    } else {
        tprint("num");
        print_int(id);
    }
    tprint(") ");
    tprint(s2);
    print_ln();
    show_context();
}


void do_dest(PDF pdf, halfword p, halfword parent_box, scaledpos cur_orig)
{
    scaledpos pos = pdf->posstruct->pos;
    scaled_whd alt_rule;
    integer k;
    if (!is_shipping_page)
        pdf_error("ext4", "destinations cannot be inside an XForm");
    if (doing_leaders)
        return;
    k = get_obj(pdf, obj_type_dest, pdf_dest_id(p), pdf_dest_named_id(p));
    if (obj_dest_ptr(pdf, k) != null) {
        warn_dest_dup(pdf_dest_id(p), pdf_dest_named_id(p),
                      "ext4", "has been already used, duplicate ignored");
        return;
    }
    obj_dest_ptr(pdf, k) = p;
    append_object_list(pdf, obj_type_dest, k);
    alt_rule.wd = pdf_width(p);
    alt_rule.ht = pdf_height(p);
    alt_rule.dp = pdf_depth(p);
    switch (pdf_dest_type(p)) {
    case pdf_dest_xyz:
        if (matrixused())
            set_rect_dimens(p, parent_box, cur_orig, alt_rule, pdf_dest_margin);
        else {
            pdf_ann_left(p) = pos.h;
            pdf_ann_top(p) = pos.v;
        }
        break;
    case pdf_dest_fith:
    case pdf_dest_fitbh:
        if (matrixused())
            set_rect_dimens(p, parent_box, cur_orig, alt_rule, pdf_dest_margin);
        else
            pdf_ann_top(p) = pos.v;
        break;
    case pdf_dest_fitv:
    case pdf_dest_fitbv:
        if (matrixused())
            set_rect_dimens(p, parent_box, cur_orig, alt_rule, pdf_dest_margin);
        else
            pdf_ann_left(p) = pos.h;
        break;
    case pdf_dest_fit:
    case pdf_dest_fitb:
        break;
    case pdf_dest_fitr:
        set_rect_dimens(p, parent_box, cur_orig, alt_rule, pdf_dest_margin);
    }
}

void write_out_pdf_mark_destinations(PDF pdf)
{
    pdf_object_list *k;
    if (pdf->dest_list != NULL) {
        k = pdf->dest_list;
        while (k != NULL) {
            if (is_obj_written(pdf, k->info)) {
                pdf_error("ext5",
                          "destination has been already written (this shouldn't happen)");
            } else {
                integer i;
                i = obj_dest_ptr(pdf, k->info);
                if (pdf_dest_named_id(i) > 0) {
                    pdf_begin_dict(pdf, k->info, 1);
                    pdf_printf(pdf, "/D ");
                } else {
                    pdf_begin_obj(pdf, k->info, 1);
                }
                pdf_out(pdf, '[');
                pdf_print_int(pdf, pdf->last_page);
                pdf_printf(pdf, " 0 R ");
                switch (pdf_dest_type(i)) {
                case pdf_dest_xyz:
                    pdf_printf(pdf, "/XYZ ");
                    pdf_print_mag_bp(pdf, pdf_ann_left(i));
                    pdf_out(pdf, ' ');
                    pdf_print_mag_bp(pdf, pdf_ann_top(i));
                    pdf_out(pdf, ' ');
                    if (pdf_dest_xyz_zoom(i) == null) {
                        pdf_printf(pdf, "null");
                    } else {
                        pdf_print_int(pdf, pdf_dest_xyz_zoom(i) / 1000);
                        pdf_out(pdf, '.');
                        pdf_print_int(pdf, (pdf_dest_xyz_zoom(i) % 1000));
                    }
                    break;
                case pdf_dest_fit:
                    pdf_printf(pdf, "/Fit");
                    break;
                case pdf_dest_fith:
                    pdf_printf(pdf, "/FitH ");
                    pdf_print_mag_bp(pdf, pdf_ann_top(i));
                    break;
                case pdf_dest_fitv:
                    pdf_printf(pdf, "/FitV ");
                    pdf_print_mag_bp(pdf, pdf_ann_left(i));
                    break;
                case pdf_dest_fitb:
                    pdf_printf(pdf, "/FitB");
                    break;
                case pdf_dest_fitbh:
                    pdf_printf(pdf, "/FitBH ");
                    pdf_print_mag_bp(pdf, pdf_ann_top(i));
                    break;
                case pdf_dest_fitbv:
                    pdf_printf(pdf, "/FitBV ");
                    pdf_print_mag_bp(pdf, pdf_ann_left(i));
                    break;
                case pdf_dest_fitr:
                    pdf_printf(pdf, "/FitR ");
                    pdf_print_rect_spec(pdf, i);
                    break;
                default:
                    pdf_error("ext5", "unknown dest type");
                    break;
                }
                pdf_printf(pdf, "]\n");
                if (pdf_dest_named_id(i) > 0)
                    pdf_end_dict(pdf);
                else
                    pdf_end_obj(pdf);
            }
            k = k->link;
        }
    }
}


void scan_pdfdest(PDF pdf)
{
    halfword q;
    integer k;
    str_number i;
    scaled_whd alt_rule;
    q = cur_list.tail_field;
    new_whatsit(pdf_dest_node);
    if (scan_keyword("num")) {
        scan_int();
        if (cur_val <= 0)
            pdf_error("ext1", "num identifier must be positive");
        if (cur_val > max_halfword)
            pdf_error("ext1", "number too big");
        set_pdf_dest_id(cur_list.tail_field, cur_val);
        set_pdf_dest_named_id(cur_list.tail_field, 0);
    } else if (scan_keyword("name")) {
        scan_pdf_ext_toks();
        set_pdf_dest_id(cur_list.tail_field, def_ref);
        set_pdf_dest_named_id(cur_list.tail_field, 1);
    } else {
        pdf_error("ext1", "identifier type missing");
    }
    if (scan_keyword("xyz")) {
        set_pdf_dest_type(cur_list.tail_field, pdf_dest_xyz);
        if (scan_keyword("zoom")) {
            scan_int();
            if (cur_val > max_halfword)
                pdf_error("ext1", "number too big");
            set_pdf_dest_xyz_zoom(cur_list.tail_field, cur_val);
        } else {
            set_pdf_dest_xyz_zoom(cur_list.tail_field, null);
        }
    } else if (scan_keyword("fitbh")) {
        set_pdf_dest_type(cur_list.tail_field, pdf_dest_fitbh);
    } else if (scan_keyword("fitbv")) {
        set_pdf_dest_type(cur_list.tail_field, pdf_dest_fitbv);
    } else if (scan_keyword("fitb")) {
        set_pdf_dest_type(cur_list.tail_field, pdf_dest_fitb);
    } else if (scan_keyword("fith")) {
        set_pdf_dest_type(cur_list.tail_field, pdf_dest_fith);
    } else if (scan_keyword("fitv")) {
        set_pdf_dest_type(cur_list.tail_field, pdf_dest_fitv);
    } else if (scan_keyword("fitr")) {
        set_pdf_dest_type(cur_list.tail_field, pdf_dest_fitr);
    } else if (scan_keyword("fit")) {
        set_pdf_dest_type(cur_list.tail_field, pdf_dest_fit);
    } else {
        pdf_error("ext1", "destination type missing");
    }
    /* Scan an optional space */
    get_x_token();
    if (cur_cmd != spacer_cmd)
        back_input();

    if (pdf_dest_type(cur_list.tail_field) == pdf_dest_fitr) {
        alt_rule = scan_alt_rule();     /* scans |<rule spec>| to |alt_rule| */
        set_pdf_width(cur_list.tail_field, alt_rule.wd);
        set_pdf_height(cur_list.tail_field, alt_rule.ht);
        set_pdf_depth(cur_list.tail_field, alt_rule.dp);
    }
    if (pdf_dest_named_id(cur_list.tail_field) != 0) {
        i = tokens_to_string(pdf_dest_id(cur_list.tail_field));
        k = find_obj(pdf, obj_type_dest, i, true);
        flush_str(i);
    } else {
        k = find_obj(pdf, obj_type_dest, pdf_dest_id(cur_list.tail_field),
                     false);
    }
    if ((k != 0) && (obj_dest_ptr(pdf, k) != null)) {
        warn_dest_dup(pdf_dest_id(cur_list.tail_field),
                      pdf_dest_named_id(cur_list.tail_field),
                      "ext4", "has been already used, duplicate ignored");
        flush_node_list(cur_list.tail_field);
        cur_list.tail_field = q;
        vlink(q) = null;
    }
}

void sort_dest_names(integer l, integer r)
{                               /* sorts |dest_names| by names */
    integer i, j;
    str_number s;
    dest_name_entry e;
    i = l;
    j = r;
    s = dest_names[(l + r) / 2].objname;
    do {
        while (str_less_str(dest_names[i].objname, s))
            incr(i);
        while (str_less_str(s, dest_names[j].objname))
            decr(j);
        if (i <= j) {
            e = dest_names[i];
            dest_names[i] = dest_names[j];
            dest_names[j] = e;
            incr(i);
            decr(j);
        }
    } while (i <= j);
    if (l < j)
        sort_dest_names(l, j);
    if (i < r)
        sort_dest_names(i, r);
}
