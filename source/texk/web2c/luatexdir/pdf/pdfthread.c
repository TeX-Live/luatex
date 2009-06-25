/* pdfthread.c
   
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

#define info(A) fixmem[(A)].hhlh
#undef link
#define link(A) fixmem[(A)].hhrh

#define pdf_thread_margin        dimen_par(param_pdf_thread_margin_code)
#define page_width dimen_par(param_page_width_code)
#define page_height dimen_par(param_page_height_code)

static halfword last_thread = 0;        /* pointer to the last thread */
static scaled pdf_thread_ht = 0;
static scaled pdf_thread_dp = 0;
static scaled pdf_thread_wd = 0;        /* dimensions of the last thread */
static halfword pdf_last_thread_id = 0; /* identifier of the last thread */
static boolean pdf_last_thread_named_id = false;        /* is identifier of the last thread named */
static integer pdf_thread_level = 0;    /* depth of nesting of box containing the last thread */
static halfword pdf_bead_list = 0;      /* list of thread beads in the current page */

/* Threads are handled in similar way as link annotations */

void append_bead(PDF pdf, halfword p)
{
    integer a, b, c, t;
    if (!is_shipping_page)
        pdf_error(maketexstring("ext4"),
                  maketexstring("threads cannot be inside an XForm"));
    t = get_obj(obj_type_thread, pdf_thread_id(p), pdf_thread_named_id(p));
    b = pdf_new_objnum();
    obj_bead_ptr(b) = pdf_get_mem(pdf, pdfmem_bead_size);
    set_obj_bead_page(pdf,b,pdf_last_page);
    set_obj_bead_data(pdf, b, p);
    if (pdf_thread_attr(p) != null)
        set_obj_bead_attr(pdf, b, tokens_to_string(pdf_thread_attr(p)));
    else
        set_obj_bead_attr(pdf, b, 0);
    if (obj_thread_first(t) == 0) {
        obj_thread_first(t) = b;
        set_obj_bead_next(pdf,b, b);
        set_obj_bead_prev(pdf,b, b);
    } else {
        a = obj_thread_first(t);
        c = obj_bead_prev(pdf, a);
        set_obj_bead_prev(pdf, b, c);
        set_obj_bead_next(pdf, b, a);
        set_obj_bead_prev(pdf, a, b);
        set_obj_bead_next(pdf, c, b);
    }
    pdf_append_list(b, pdf_bead_list);
}

void do_thread(PDF pdf, halfword parent_box, halfword p, scaled x, scaled y)
{
    if (doing_leaders)
        return;
    if (subtype(p) == pdf_start_thread_node) {
        pdf_thread_wd = pdf_width(p);
        pdf_thread_ht = pdf_height(p);
        pdf_thread_dp = pdf_depth(p);
        pdf_last_thread_id = pdf_thread_id(p);
        pdf_last_thread_named_id = (pdf_thread_named_id(p) > 0);
        if (pdf_last_thread_named_id)
            add_token_ref(pdf_thread_id(p));
        pdf_thread_level = cur_s;
    }
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p),
                    pdf_thread_margin);
    append_bead(pdf, p);
    last_thread = p;
}

void append_thread(PDF pdf, halfword parent_box, scaled x, scaled y)
{
    halfword p;
    p = new_node(whatsit_node, pdf_thread_data_node);
    pdf_width(p) = pdf_thread_wd;
    pdf_height(p) = pdf_thread_ht;
    pdf_depth(p) = pdf_thread_dp;
    pdf_thread_attr(p) = null;
    pdf_thread_id(p) = pdf_last_thread_id;
    if (pdf_last_thread_named_id) {
        add_token_ref(pdf_thread_id(p));
        pdf_thread_named_id(p) = 1;
    } else {
        pdf_thread_named_id(p) = 0;
    }
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p),
                    pdf_thread_margin);
    append_bead(pdf,p);
    last_thread = p;
}

void end_thread(PDF pdf)
{
    scaledpos tmp1, tmp2;
    (void)pdf; /* for later */
    if (pdf_thread_level != cur_s)
        pdf_error(maketexstring("ext4"),
                  maketexstring
                  ("\\pdfendthread ended up in different nesting level than \\pdfstartthread"));
    if (is_running(pdf_thread_dp) && (last_thread != null)) {
        tmp1.h = cur.h;
        tmp1.v = cur.v;
        tmp2 = synch_p_with_c(tmp1);
        switch (box_direction(dvi_direction)) {
        case dir_TL_:
        case dir_TR_:
            pdf_ann_bottom(last_thread) = tmp2.v - pdf_thread_margin;
            break;
        case dir_BL_:
        case dir_BR_:
            pdf_ann_top(last_thread) = tmp2.v + pdf_thread_margin;
            break;
        case dir_LT_:
        case dir_LB_:
            pdf_ann_right(last_thread) = tmp2.h + pdf_thread_margin;
            break;
        case dir_RT_:
        case dir_RB_:
            pdf_ann_left(last_thread) = tmp2.h - pdf_thread_margin;
            break;
        }
    }
    if (pdf_last_thread_named_id)
        delete_token_ref(pdf_last_thread_id);
    last_thread = null;
}

/* The following function are needed for outputing article thread. */

void thread_title(PDF pdf, integer t)
{
    pdf_printf(pdf, "/Title (");
    if (obj_info(t) < 0)
        pdf_print(pdf, -obj_info(t));
    else
        pdf_print_int(pdf, obj_info(t));
    pdf_printf(pdf, ")\n");
}

void pdf_fix_thread(PDF pdf, integer t)
{
    halfword a;
    pdf_warning(maketexstring("thread"),
                maketexstring("destination "), false, false);
    if (obj_info(t) < 0) {
        tprint("name{");
        print(-obj_info(t));
        tprint("}");
    } else {
        tprint("num");
        print_int(obj_info(t));
    }
    tprint(" has been referenced but does not exist, replaced by a fixed one");
    print_ln();
    print_ln();
    pdf_new_dict(pdf, obj_type_others, 0, 0);
    a = obj_ptr;
    pdf_indirect_ln(pdf, "T", t);
    pdf_indirect_ln(pdf, "V", a);
    pdf_indirect_ln(pdf, "N", a);
    pdf_indirect_ln(pdf, "P", head_tab[obj_type_page]);
    pdf_printf(pdf, "/R [0 0 ");
    pdf_print_bp(pdf, page_width);
    pdf_out(pdf, ' ');
    pdf_print_bp(pdf, page_height);
    pdf_printf(pdf, "]\n");
    pdf_end_dict(pdf);
    pdf_begin_dict(pdf, t, 1);
    pdf_printf(pdf, "/I << \n");
    thread_title(pdf, t);
    pdf_printf(pdf, ">>\n");
    pdf_indirect_ln(pdf, "F", a);
    pdf_end_dict(pdf);
}

void out_thread(PDF pdf, integer t)
{
    halfword a, b;
    integer last_attr;
    if (obj_thread_first(t) == 0) {
        pdf_fix_thread(pdf, t);
        return;
    }
    pdf_begin_dict(pdf, t, 1);
    a = obj_thread_first(t);
    b = a;
    last_attr = 0;
    do {
        if (obj_bead_attr(pdf, a) != 0)
            last_attr = obj_bead_attr(pdf, a);
        a = obj_bead_next(pdf, a);
    } while (a != b);
    if (last_attr != 0) {
        pdf_print_ln(pdf, last_attr);
    } else {
        pdf_printf(pdf, "/I << \n");
        thread_title(pdf, t);
        pdf_printf(pdf, ">>\n");
    }
    pdf_indirect_ln(pdf, "F", a);
    pdf_end_dict(pdf);
    do {
        pdf_begin_dict(pdf, a, 1);
        if (a == b)
            pdf_indirect_ln(pdf, "T", t);
        pdf_indirect_ln(pdf, "V", obj_bead_prev(pdf, a));
        pdf_indirect_ln(pdf, "N", obj_bead_next(pdf, a));
        pdf_indirect_ln(pdf, "P", obj_bead_page(pdf, a));
        pdf_indirect_ln(pdf, "R", obj_bead_rect(pdf, a));
        pdf_end_dict(pdf);
        a = obj_bead_next(pdf, a);
    } while (a != b);
}


void scan_thread_id(void)
{
    if (scan_keyword("num")) {
        scan_int();
        if (cur_val <= 0)
            pdf_error(maketexstring("ext1"),
                      maketexstring("num identifier must be positive"));
        if (cur_val > max_halfword)
            pdf_error(maketexstring("ext1"), maketexstring("number too big"));
        set_pdf_thread_id(cur_list.tail_field, cur_val);
        set_pdf_thread_named_id(cur_list.tail_field, 0);
    } else if (scan_keyword("name")) {
        scan_pdf_ext_toks();
        set_pdf_thread_id(cur_list.tail_field, def_ref);
        set_pdf_thread_named_id(cur_list.tail_field, 1);
    } else {
        pdf_error(maketexstring("ext1"),
                  maketexstring("identifier type missing"));
    }
}

void check_running_thread(PDF pdf, halfword this_box, scaledpos cur)
{
    if ((last_thread != null) && is_running(pdf_thread_dp)
        && (pdf_thread_level == cur_s))
        append_thread(pdf, this_box, cur.h, cur.v + height(this_box));
}

void reset_thread_lists(void)
{
    pdf_bead_list = null;
    last_thread = null;
}

void print_beads_list(PDF pdf)
{
    halfword k;
    if (pdf_bead_list != null) {
        k = pdf_bead_list;
        pdf_printf(pdf, "/B [ ");
        while (k != null) {
            pdf_print_int(pdf, info(k));
            pdf_printf(pdf, " 0 R ");
            k = link(k);
        }
        pdf_printf(pdf, "]\n");
    }
}

void print_bead_rectangles(PDF pdf)
{
    halfword i, k;
    if (pdf_bead_list != null) {
        k = pdf_bead_list;
        while (k != null) {
            pdf_new_obj(pdf, obj_type_others, 0, 1);
            pdf_out(pdf, '[');
            i = obj_bead_data(pdf, info(k)); /* pointer to a whatsit or whatsit-like node */
            pdf_print_rect_spec(pdf, i);
            if (subtype(i) == pdf_thread_data_node)     /* thanh says it mis be destroyed here */
                flush_node(i);
            pdf_printf(pdf, "]\n");
            set_obj_bead_rect(pdf, info(k), obj_ptr);        /* rewrite |obj_bead_data| */
            pdf_end_obj(pdf);
            k = link(k);
        }
    }
}

void flush_beads_list(void)
{
    flush_list(pdf_bead_list);
}
