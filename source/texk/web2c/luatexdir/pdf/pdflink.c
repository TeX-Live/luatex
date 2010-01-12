/* pdflink.c

   Copyright 2009-2010 Taco Hoekwater <taco@luatex.org>

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

static const char _svn_version[] =
    "$Id$"
    "$URL$";

#include "ptexlib.h"

#define pdf_link_margin          dimen_par(pdf_link_margin_code)

/*
To implement nested link annotations, we need a stack to hold copy of
|pdf_start_link_node|'s that are being written out, together with their box
nesting level.
*/

void push_link_level(PDF pdf, halfword p)
{
    if (pdf->link_stack_ptr >= pdf_max_link_level)
        overflow("pdf link stack size", pdf_max_link_level);
    assert(((type(p) == whatsit_node) && (subtype(p) == pdf_start_link_node)));
    incr(pdf->link_stack_ptr);
    pdf->link_stack[pdf->link_stack_ptr].nesting_level = cur_s;
    pdf->link_stack[pdf->link_stack_ptr].link_node = copy_node_list(p);
    pdf->link_stack[pdf->link_stack_ptr].ref_link_node = p;
}

void pop_link_level(PDF pdf)
{
    assert(pdf->link_stack_ptr > 0);
    flush_node_list(pdf->link_stack[pdf->link_stack_ptr].link_node);
    decr(pdf->link_stack_ptr);
}

void do_link(PDF pdf, halfword p, halfword parent_box, scaledpos cur)
{
    scaled_whd alt_rule;
    if (type(p) == vlist_node)
        pdf_error("ext4", "\\pdfstartlink ended up in vlist");
    if (!is_shipping_page)
        pdf_error("ext4", "link annotations cannot be inside an XForm");
    assert(type(parent_box) == hlist_node);
    if (is_obj_scheduled(pdf, pdf_link_objnum(p)))
        pdf_link_objnum(p) = pdf_new_objnum(pdf);
    push_link_level(pdf, p);
    alt_rule.wd = width(p);
    alt_rule.ht = height(p);
    alt_rule.dp = depth(p);
    set_rect_dimens(pdf, p, parent_box, cur, alt_rule, pdf_link_margin);
    obj_annot_ptr(pdf, pdf_link_objnum(p)) = p; /* the reference for the pdf annot object must be set here */
    addto_page_resources(pdf, obj_type_link, pdf_link_objnum(p));
}

void end_link(PDF pdf, halfword p)
{
    halfword q;
    scaledpos pos = pdf->posstruct->pos;
    if (type(p) == vlist_node)
        pdf_error("ext4", "\\pdfendlink ended up in vlist");
    if (pdf->link_stack_ptr < 1)
        pdf_error("ext4",
                  "pdf link_stack empty, \\pdfendlink used without \\pdfstartlink?");
    if (pdf->link_stack[pdf->link_stack_ptr].nesting_level != cur_s)
        pdf_error("ext4",
                  "\\pdfendlink ended up in different nesting level than \\pdfstartlink");

    /* N.B.: test for running link must be done on |link_node| and not |ref_link_node|,
       as |ref_link_node| can be set by |do_link| or |append_link| already */

    if (is_running(width(pdf->link_stack[pdf->link_stack_ptr].link_node))) {
        q = pdf->link_stack[pdf->link_stack_ptr].ref_link_node;
        if (is_shipping_page && matrixused()) {
            matrixrecalculate(pos.h + pdf_link_margin);
            pdf_ann_left(q) = getllx() - pdf_link_margin;
            pdf_ann_top(q) = cur_page_size.v - getury() - pdf_link_margin;
            pdf_ann_right(q) = geturx() + pdf_link_margin;
            pdf_ann_bottom(q) = cur_page_size.v - getlly() + pdf_link_margin;
        } else {
            switch (pdf->posstruct->dir) {
            case dir_TLT:
                pdf_ann_right(q) = pos.h + pdf_link_margin;
                break;
            case dir_TRT:
                pdf_ann_left(q) = pos.h - pdf_link_margin;
                break;
            case dir_LTL:
            case dir_RTT:
                pdf_ann_bottom(q) = pos.v - pdf_link_margin;
                break;
            }
        }
    }
    pop_link_level(pdf);
}

/*
For ``running'' annotations we must append a new node when the end of
annotation is in other box than its start. The new created node is identical to
corresponding whatsit node representing the start of annotation,  but its
|info| field is |max_halfword|. We set |info| field just before destroying the
node, in order to use |flush_node_list| to do the job.
*/

/* append a new pdf annot to |pdf_link_list| */

void append_link(PDF pdf, halfword parent_box, scaledpos cur, small_number i)
{
    halfword p;
    scaled_whd alt_rule;
    assert(type(parent_box) == hlist_node);
    p = copy_node(pdf->link_stack[(int) i].link_node);
    pdf->link_stack[(int) i].ref_link_node = p;
    subtype(p) = pdf_link_data_node;    /* this node is not a normal link node */
    alt_rule.wd = width(p);
    alt_rule.ht = height(p);
    alt_rule.dp = depth(p);
    set_rect_dimens(pdf, p, parent_box, cur, alt_rule, pdf_link_margin);
    pdf_create_obj(pdf, obj_type_others, 0);
    obj_annot_ptr(pdf, pdf->obj_ptr) = p;
    addto_page_resources(pdf, obj_type_link, pdf->obj_ptr);
}

void scan_startlink(PDF pdf)
{
    int k;
    halfword r;
    if (abs(cur_list.mode_field) == vmode)
        pdf_error("ext1", "\\pdfstartlink cannot be used in vertical mode");
    k = pdf_new_objnum(pdf);
    new_annot_whatsit(pdf_start_link_node);
    set_pdf_link_attr(cur_list.tail_field, null);
    if (scan_keyword("attr")) {
        scan_pdf_ext_toks();
        set_pdf_link_attr(cur_list.tail_field, def_ref);
    }
    r = scan_action(pdf);
    set_pdf_link_action(cur_list.tail_field, r);
    set_pdf_link_objnum(cur_list.tail_field, k);
    pdf_last_link = k;
    /* N.B.: although it is possible to set |obj_annot_ptr(k) := tail| here, it
       is not safe if nodes are later copied/destroyed/moved; a better place
       to do this is inside |do_link|, when the whatsit node is written out */
}
