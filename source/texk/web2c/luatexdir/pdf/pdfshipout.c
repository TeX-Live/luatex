/* pdfshipout.c
   
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
#include "luatex-api.h"         /* for tokenlist_to_cstring */
#include "tokens.h"             /* for link and info */

#define count(A) zeqtb[count_base+(A)].cint


#define pdf_output int_par(param_pdf_output_code)
#define pdf_gen_tounicode int_par(param_pdf_gen_tounicode_code)
#define mag int_par(param_mag_code)
#define tracing_output int_par(param_tracing_output_code)
#define tracing_stats int_par(param_tracing_stats_code)
#define page_direction int_par(param_page_direction_code)       /* really direction */
#define page_width dimen_par(param_page_width_code)
#define page_height dimen_par(param_page_height_code)
#define page_left_offset dimen_par(param_page_left_offset_code)
#define page_right_offset dimen_par(param_page_right_offset_code)
#define page_top_offset dimen_par(param_page_top_offset_code)
#define page_bottom_offset dimen_par(param_page_bottom_offset_code)
#define h_offset dimen_par(param_h_offset_code)
#define v_offset dimen_par(param_v_offset_code)
#define pdf_h_origin dimen_par(param_pdf_h_origin_code)
#define pdf_v_origin dimen_par(param_pdf_v_origin_code)
#define pdf_page_attr loc_par(param_pdf_page_attr_code)
#define pdf_pages_attr loc_par(param_pdf_pages_attr_code)
#define pdf_page_resources loc_par(param_pdf_page_resources_code)

static const char __svn_version[] =
    "$Id$"
    "$URL$";

/*
|fix_pdfoutput| freezes |fixed_pdfoutput| as soon as anything has been written
to the output file, be it \.{PDF} or \.{DVI}.
*/

void fix_pdfoutput(void)
{
    if (!fixed_pdfoutput_set) {
        fixed_pdfoutput = pdf_output;
        fixed_pdfoutput_set = true;
    } else if (fixed_pdfoutput != pdf_output) {
        pdf_error(maketexstring("setup"),
                  maketexstring
                  ("\\pdfoutput can only be changed before anything is written to the output"));
    }
}


void reset_resource_lists(void)
{
    pdf_font_list = null;
    pdf_obj_list = null;
    pdf_xform_list = null;
    pdf_ximage_list = null;
    pdf_text_procset = false;
    pdf_image_procset = 0;
}



/*
|pdf_ship_out| is used instead of |ship_out| to shipout a box to PDF
 output. If |shipping_page| is not set then the output will be a Form object,
otherwise it will be a Page object.
*/

void pdf_ship_out(halfword p, boolean shipping_page)
{                               /* output the box |p| */
    integer i, j, k;            /* general purpose accumulators */
    halfword save_font_list;    /* to save |pdf_font_list| during flushing pending forms */
    halfword save_obj_list;     /* to save |pdf_obj_list| */
    halfword save_ximage_list;  /* to save |pdf_ximage_list| */
    halfword save_xform_list;   /* to save |pdf_xform_list| */
    integer save_image_procset; /* to save |pdf_image_procset| */
    integer save_text_procset;  /* to save |pdf_text_procset| */
    scaledpos save_cur_page_size;       /* to save |cur_page_size| during flushing pending forms */
    scaled form_margin;
    integer pdf_last_resources; /* halfword to most recently generated Resources object */
    integer pre_callback_id;
    integer post_callback_id;
    boolean ret;
    integer count_base = get_count_base();
    /* Start sheet {\sl Sync\TeX} information record */
    pdf_output_value = pdf_output;      /* {\sl Sync\TeX}: we assume that |pdf_output| is properly set up */
    synctex_sheet(mag);

    form_margin = one_bp;
    pre_callback_id = callback_defined(start_page_number_callback);
    post_callback_id = callback_defined(stop_page_number_callback);
    if ((tracing_output > 0) && (pre_callback_id == 0)) {
        tprint_nl("");
        print_ln();
        tprint("Completed box being shipped out");
    }
    check_pdfminorversion();
    is_shipping_page = shipping_page;
    if (shipping_page) {
        if (pre_callback_id > 0)
            ret = run_callback(pre_callback_id, "->");
        else if (pre_callback_id == 0) {
            if (term_offset > max_print_line - 9)
                print_ln();
            else if ((term_offset > 0) || (file_offset > 0))
                print_char(' ');
            print_char('[');
            j = 9;
            while ((count(j) == 0) && (j > 0))
                decr(j);
            for (k = 0; k <= j; k++) {
                print_int(count(k));
                if (k < j)
                    print_char('.');
            }
        }
    }
    if ((tracing_output > 0) && (post_callback_id == 0) && shipping_page) {
        print_char(']');
        begin_diagnostic();
        show_box(p);
        end_diagnostic(true);
    }

    /* (\pdfTeX) Ship box |p| out */
    if (shipping_page && (box_dir(p) != page_direction))
        pdf_warning(maketexstring("\\shipout"),
                    maketexstring
                    ("\\pagedir != \\bodydir; \\box255 may be placed wrongly on the page."),
                    true, true);
    /* Update the values of |max_h| and |max_v|; but if the page is too large, |goto done| */
    /* todo: this can be shared with dvi output */
    if ((height(p) > max_dimen) || (depth(p) > max_dimen) ||
        (height(p) + depth(p) + v_offset > max_dimen) ||
        (width(p) + h_offset > max_dimen)) {
        char *hlp[] = { "The page just created is more than 18 feet tall or",
            "more than 18 feet wide, so I suspect something went wrong.",
            NULL
        };
        tex_error("Huge page cannot be shipped out", hlp);
        if (tracing_output <= 0) {
            begin_diagnostic();
            tprint_nl("The following box has been deleted:");
            show_box(p);
            end_diagnostic(true);
        }
        goto DONE;
    }
    if (height(p) + depth(p) + v_offset > max_v)
        max_v = height(p) + depth(p) + v_offset;
    if (width(p) + h_offset > max_h)
        max_h = width(p) + h_offset;

    /* Initialize variables as |pdf_ship_out| begins */
    prepare_mag();
    temp_ptr = p;
    pdf_last_resources = pdf_new_objnum();
    reset_resource_lists();
    dvi_direction = page_direction;

    /* Calculate PDF page dimensions and margins */
    if (is_shipping_page) {
        /* Calculate DVI page dimensions and margins */
        /* todo: this can be shared with dvi */
        if (page_width > 0) {
            cur_page_size.h = page_width;
        } else {
            switch (box_direction(dvi_direction)) {
            case dir_TL_:
            case dir_BL_:
                cur_page_size.h = width(p) + 2 * page_left_offset;
                break;
            case dir_TR_:
            case dir_BR_:
                cur_page_size.h = width(p) + 2 * page_right_offset;
                break;
            case dir_LT_:
            case dir_LB_:
                cur_page_size.h = height(p) + depth(p) + 2 * page_left_offset;
                break;
            case dir_RT_:
            case dir_RB_:
                cur_page_size.h = height(p) + depth(p) + 2 * page_right_offset;
                break;
            }
        }
        if (page_height > 0) {
            cur_page_size.v = page_height;
        } else {
            switch (box_direction(dvi_direction)) {
            case dir_TL_:
            case dir_TR_:
                cur_page_size.v = height(p) + depth(p) + 2 * page_top_offset;
                break;
            case dir_BL_:
            case dir_BR_:
                cur_page_size.v = height(p) + depth(p) + 2 * page_bottom_offset;
                break;
            case dir_LT_:
            case dir_RT_:
                cur_page_size.v = width(p) + 2 * page_top_offset;
                break;
            case dir_LB_:
            case dir_RB_:
                cur_page_size.v = width(p) + 2 * page_bottom_offset;
                break;
            }
        }

        /* Think in upright page/paper coordinates: First preset |pos.h| and |pos.v| to the DVI origin. */
        pos.h = pdf_h_origin;
        pos.v = cur_page_size.v - pdf_v_origin;
        box_pos = pos;
        /* Then calculate |cur.h| and |cur.v| within the upright coordinate system
           for the DVI origin depending on the |page_direction|. */
        dvi_direction = page_direction;
        switch (box_direction(dvi_direction)) {
        case dir_TL_:
        case dir_LT_:
            cur.h = h_offset;
            cur.v = v_offset;
            break;
        case dir_TR_:
        case dir_RT_:
            cur.h = cur_page_size.h - page_right_offset - one_true_inch;
            cur.v = v_offset;
            break;
        case dir_BL_:
        case dir_LB_:
            cur.h = h_offset;
            cur.v = cur_page_size.v - page_bottom_offset - one_true_inch;
            break;
        case dir_RB_:
        case dir_BR_:
            cur.h = cur_page_size.h - page_right_offset - one_true_inch;
            cur.v = cur_page_size.v - page_bottom_offset - one_true_inch;
            break;
        }
        /* The movement is actually done within the upright page coordinate system. */
        dvi_direction = dir_TL_;        /* only temporarily for this adjustment */
        pos = synch_p_with_c(cur);
        box_pos = pos;          /* keep track on page */
        /* Then switch to page box coordinate system; do |height(p)| movement. */
        dvi_direction = page_direction;
        cur.h = 0;
        cur.v = height(p);
        pos = synch_p_with_c(cur);
        /* Now we are at the point on the page where the origin of the page box should go. */
    } else {
        dvi_direction = box_dir(p);
        switch (box_direction(dvi_direction)) {
        case dir_TL_:
        case dir_TR_:
        case dir_BL_:
        case dir_BR_:
            cur_page_size.h = width(p);
            cur_page_size.v = height(p) + depth(p);
            break;
        case dir_LT_:
        case dir_RT_:
        case dir_LB_:
        case dir_RB_:
            cur_page_size.h = height(p) + depth(p);
            cur_page_size.v = width(p);
            break;
        }
        switch (box_direction(dvi_direction)) {
        case dir_TL_:
            pos.h = 0;
            pos.v = depth(p);
            break;
        case dir_TR_:
            pos.h = width(p);
            pos.v = depth(p);
            break;
        case dir_BL_:
            pos.h = 0;
            pos.v = height(p);
            break;
        case dir_BR_:
            pos.h = width(p);
            pos.v = height(p);
            break;
        case dir_LT_:
            pos.h = height(p);
            pos.v = width(p);
            break;
        case dir_RT_:
            pos.h = depth(p);
            pos.v = width(p);
            break;
        case dir_LB_:
            pos.h = height(p);
            pos.v = 0;
            break;
        case dir_RB_:
            pos.h = depth(p);
            pos.v = 0;
            break;
        }
    }
    pdf_page_init();

    if (!shipping_page) {
        pdf_begin_dict(pdf_cur_form, 0);
        pdf_last_stream = pdf_cur_form;

        /* Write out Form stream header */
        pdf_printf("/Type /XObject\n");
        pdf_printf("/Subtype /Form\n");
        if (obj_xform_attr(pdf_cur_form) != null) {
            pdf_print_toks_ln(obj_xform_attr(pdf_cur_form));
            delete_token_ref(obj_xform_attr(pdf_cur_form));
            set_obj_xform_attr(pdf_cur_form, null);
        }
        pdf_printf("/BBox [");
        pdf_print_bp(-form_margin);
        pdf_out(' ');
        pdf_print_bp(-form_margin);
        pdf_out(' ');
        pdf_print_bp(cur_page_size.h + form_margin);
        pdf_out(' ');
        pdf_print_bp(cur_page_size.v + form_margin);
        pdf_printf("]\n");
        pdf_printf("/FormType 1\n");
        pdf_printf("/Matrix [1 0 0 1 0 0]\n");
        pdf_indirect_ln("Resources", pdf_last_resources);

    } else {
        pdf_last_page = get_obj(obj_type_page, total_pages + 1, 0);
        set_obj_aux(pdf_last_page, 1);  /* mark that this page has been created */
        pdf_new_dict(obj_type_others, 0, 0);
        pdf_last_stream = obj_ptr;
        /* Reset PDF mark lists */
        pdf_annot_list = null;
        pdf_link_list = null;
        pdf_dest_list = null;
        pdf_bead_list = null;
        last_thread = null;
    }
    /* Start stream of page/form contents */
    pdf_begin_stream();
    if (shipping_page) {
        /* Adjust transformation matrix for the magnification ratio */
        prepare_mag();
        if (mag != 1000) {
            pdf_print_real(mag, 3);
            pdf_printf(" 0 0 ");
            pdf_print_real(mag, 3);
            pdf_printf(" 0 0 cm\n");
        }
    }
    pdfshipoutbegin(shipping_page);
    if (shipping_page)
        pdf_out_colorstack_startpage();


    if (type(p) == vlist_node)
        pdf_vlist_out();
    else
        pdf_hlist_out();
    if (shipping_page)
        incr(total_pages);
    cur_s = -1;

    /* Finish shipping */
    /* Finish stream of page/form contents */
    pdf_goto_pagemode();
    pdfshipoutend(shipping_page);
    pdf_end_stream();

    if (shipping_page) {
        /* Write out page object */

        pdf_begin_dict(pdf_last_page, 1);
        pdf_last_pages = pdf_do_page_divert(pdf_last_page, page_divert_val);
        pdf_printf("/Type /Page\n");
        pdf_indirect_ln("Contents", pdf_last_stream);
        pdf_indirect_ln("Resources", pdf_last_resources);
        pdf_printf("/MediaBox [0 0 ");
        pdf_print_mag_bp(cur_page_size.h);
        pdf_out(' ');
        pdf_print_mag_bp(cur_page_size.v);
        pdf_printf("]\n");
        if (pdf_page_attr != null)
            pdf_print_toks_ln(pdf_page_attr);
        pdf_indirect_ln("Parent", pdf_last_pages);
        if (pdf_page_group_val > 0) {
            pdf_printf("/Group %d 0 R\n", (int) pdf_page_group_val);
            pdf_page_group_val = -1;
        }
        /* Generate array of annotations or beads in page */
        if ((pdf_annot_list != null) || (pdf_link_list != null)) {
            pdf_printf("/Annots [ ");
            k = pdf_annot_list;
            while (k != null) {
                pdf_print_int(info(k));
                pdf_printf(" 0 R ");
                k = link(k);
            }
            k = pdf_link_list;
            while (k != null) {
                pdf_print_int(info(k));
                pdf_printf(" 0 R ");
                k = link(k);
            }
            pdf_printf("]\n");
        }
        if (pdf_bead_list != null) {
            k = pdf_bead_list;
            pdf_printf("/B [ ");
            while (k != null) {
                pdf_print_int(info(k));
                pdf_printf(" 0 R ");
                k = link(k);
            }
            pdf_printf("]\n");
        }

        pdf_end_dict();

    }
    /* Write out resource lists */
    /* Write out pending raw objects */
    if (pdf_obj_list != null) {
        k = pdf_obj_list;
        while (k != null) {
            if (!is_obj_written(info(k)))
                pdf_write_obj(info(k));
            k = link(k);
        }
    }

    /* Write out pending forms */
    /* When flushing pending forms we need to save and restore resource lists
       (|pdf_font_list|, |pdf_obj_list|, |pdf_xform_list| and |pdf_ximage_list|),
       which are also used by page shipping.
       Saving and restoring |cur_page_size| is needed for proper
       writing out pending PDF marks. */
    if (pdf_xform_list != null) {
        k = pdf_xform_list;
        while (k != null) {
            if (!is_obj_written(info(k))) {
                pdf_cur_form = info(k);
                /* Save resource lists */
                save_font_list = pdf_font_list;
                save_obj_list = pdf_obj_list;
                save_xform_list = pdf_xform_list;
                save_ximage_list = pdf_ximage_list;
                save_text_procset = pdf_text_procset;
                save_image_procset = pdf_image_procset;

                reset_resource_lists();
                save_cur_page_size = cur_page_size;
                pdf_ship_out(obj_xform_box(pdf_cur_form), false);
                cur_page_size = save_cur_page_size;

                /* Restore resource lists */
                pdf_font_list = save_font_list;
                pdf_obj_list = save_obj_list;
                pdf_xform_list = save_xform_list;
                pdf_ximage_list = save_ximage_list;
                pdf_text_procset = save_text_procset;
                pdf_image_procset = save_image_procset;

            }
            k = link(k);
        }
    }

    /* Write out pending images */
    if (pdf_ximage_list != null) {
        k = pdf_ximage_list;
        while (k != null) {
            if (!is_obj_written(info(k)))
                pdf_write_image(info(k));
            k = link(k);
        }
    }

    if (shipping_page) {
        /* Write out pending PDF marks */
        /* Write out PDF annotations */
        if (pdf_annot_list != null) {
            k = pdf_annot_list;
            while (k != null) {
                i = obj_annot_ptr(info(k));     /* |i| points to |pdf_annot_node| */
                pdf_begin_dict(info(k), 1);
                pdf_printf("/Type /Annot\n");
                pdf_print_toks_ln(pdf_annot_data(i));
                pdf_rectangle(i);
                pdf_end_dict();
                k = link(k);
            }
        }

        /* Write out PDF link annotations */
        if (pdf_link_list != null) {
            k = pdf_link_list;
            while (k != null) {
                i = obj_annot_ptr(info(k));
                pdf_begin_dict(info(k), 1);
                pdf_printf("/Type /Annot\n");
                if (pdf_action_type(pdf_link_action(i)) != pdf_action_user)
                    pdf_printf("/Subtype /Link\n");
                if (pdf_link_attr(i) != null)
                    pdf_print_toks_ln(pdf_link_attr(i));
                pdf_rectangle(i);
                if (pdf_action_type(pdf_link_action(i)) != pdf_action_user)
                    pdf_printf("/A ");
                write_action(pdf_link_action(i));
                pdf_end_dict();
                k = link(k);
            }
            /* Flush |pdf_start_link_node|'s created by |append_link| */
            k = pdf_link_list;
            while (k != null) {
                i = obj_annot_ptr(info(k));
                /* nodes with |subtype = pdf_link_data_node| were created by |append_link| and
                   must be flushed here, as they are not linked in any list */
                if (subtype(i) == pdf_link_data_node)
                    flush_node(i);
                k = link(k);
            }
        }

        /* Write out PDF mark destinations */
        write_out_pdf_mark_destinations();

        /* Write out PDF bead rectangle specifications */
        if (pdf_bead_list != null) {
            k = pdf_bead_list;
            while (k != null) {
                pdf_new_obj(obj_type_others, 0, 1);
                pdf_out('[');
                i = obj_bead_data(info(k));     /* pointer to a whatsit or whatsit-like node */
                pdf_print_rect_spec(i);
                if (subtype(i) == pdf_thread_data_node) /* thanh says it mis be destroyed here */
                    flush_node(i);
                pdf_printf("]\n");
                set_obj_bead_rect(info(k), obj_ptr);    /* rewrite |obj_bead_data| */
                pdf_end_obj();
                k = link(k);
            }
        }

    }
    /* Write out resources dictionary */
    pdf_begin_dict(pdf_last_resources, 1);
    /* Print additional resources */
    if (shipping_page) {
        if (pdf_page_resources != null)
            pdf_print_toks_ln(pdf_page_resources);
    } else {
        if (obj_xform_resources(pdf_cur_form) != null) {
            pdf_print_toks_ln(obj_xform_resources(pdf_cur_form));
            delete_token_ref(obj_xform_resources(pdf_cur_form));
            set_obj_xform_resources(pdf_cur_form, null);
        }
    }

    /* Generate font resources */
    if (pdf_font_list != null) {
        pdf_printf("/Font << ");
        k = pdf_font_list;
        while (k != null) {
            pdf_printf("/F");
            set_ff(info(k));
            pdf_print_int(ff);
            pdf_print_resname_prefix();
            pdf_out(' ');
            pdf_print_int(pdf_font_num(ff));
            pdf_printf(" 0 R ");
            k = link(k);
        }
        pdf_printf(">>\n");
        pdf_text_procset = true;
    }

    /* Generate XObject resources */
    if ((pdf_xform_list != null) || (pdf_ximage_list != null)) {
        pdf_printf("/XObject << ");
        k = pdf_xform_list;
        while (k != null) {
            pdf_printf("/Fm");
            pdf_print_int(obj_info(info(k)));
            pdf_print_resname_prefix();
            pdf_out(' ');
            pdf_print_int(info(k));
            pdf_printf(" 0 R ");
            k = link(k);
        }
        k = pdf_ximage_list;
        while (k != null) {
            pdf_printf("/Im");
            pdf_print_int(image_index(obj_data_ptr(info(k))));
            pdf_print_resname_prefix();
            pdf_out(' ');
            pdf_print_int(info(k));
            pdf_printf(" 0 R ");
            update_image_procset(obj_data_ptr(info(k)));
            k = link(k);
        }
        pdf_printf(">>\n");
    }

    /* Generate ProcSet */
    pdf_printf("/ProcSet [ /PDF");
    if (pdf_text_procset)
        pdf_printf(" /Text");
    if (check_image_b(pdf_image_procset))
        pdf_printf(" /ImageB");
    if (check_image_c(pdf_image_procset))
        pdf_printf(" /ImageC");
    if (check_image_i(pdf_image_procset))
        pdf_printf(" /ImageI");
    pdf_printf(" ]\n");

    pdf_end_dict();


    /* In the end of shipping out a page we reset all the lists holding objects
       have been created during the page shipping. */
    /* Flush resource lists */
    flush_list(pdf_font_list);
    flush_list(pdf_obj_list);
    flush_list(pdf_xform_list);
    flush_list(pdf_ximage_list);

    if (shipping_page) {
        /* Flush PDF mark lists */
        flush_list(pdf_annot_list);
        flush_list(pdf_link_list);
        flush_list(pdf_dest_list);
        flush_list(pdf_bead_list);
    }

  DONE:
    if ((tracing_output <= 0) && (post_callback_id == 0) && shipping_page)
        print_char(']');
    dead_cycles = 0;

    /* Flush the box from memory, showing statistics if requested */
    /* todo: this can be shared with dvi */
    if ((tracing_stats > 1) && (pre_callback_id == 0)) {
        tprint_nl("Memory usage before: ");
        print_int(var_used);
        print_char('&');
        print_int(dyn_used);
        print_char(';');
    }
    flush_node_list(p);
    if ((tracing_stats > 1) && (post_callback_id == 0)) {
        tprint(" after: ");
        print_int(var_used);
        print_char('&');
        print_int(dyn_used);
        print_ln();
    }

    if (shipping_page && post_callback_id > 0)
        ret = run_callback(post_callback_id, "->");

    /* Finish sheet {\sl Sync\TeX} information record */
    synctex_teehs();
}


/* Finishing the PDF output file. */

/* The following procedures sort the table of destination names */

boolean str_less_str(str_number s1, str_number s2)
{                               /* compare two strings */
    pool_pointer c1, c2;
    integer l, i;
    c1 = str_start_macro(s1);
    c2 = str_start_macro(s2);
    if (str_length(s1) < str_length(s2))
        l = str_length(s1);
    else
        l = str_length(s2);
    i = 0;
    while ((i < l) && (str_pool[c1 + i] == str_pool[c2 + i]))
        incr(i);
    if (((i < l) && (str_pool[c1 + i] < str_pool[c2 + i])) ||
        ((i == l) && (str_length(s1) < str_length(s2))))
        return true;
    else
        return false;
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



/*
Destinations that have been referenced but don't exists have
|obj_dest_ptr=null|. Leaving them undefined might cause troubles for
PDF browsers, so we need to fix them.
*/

void pdf_fix_dest(integer k)
{
    if (obj_dest_ptr(k) != null)
        return;
    pdf_warning(maketexstring("dest"), get_nullstr(), false, false);
    if (obj_info(k) < 0) {
        tprint("name{");
        print(-obj_info(k));
        tprint("}");
    } else {
        tprint("num");
        print_int(obj_info(k));
    }
    tprint(" has been referenced but does not exist, replaced by a fixed one");
    print_ln();
    print_ln();
    pdf_begin_obj(k, 1);
    pdf_out('[');
    pdf_print_int(head_tab[obj_type_page]);
    pdf_printf(" 0 R /Fit]\n");
    pdf_end_obj();
}

void print_pdf_pages_attr(void)
{
    if (pdf_pages_attr != null)
        pdf_print_toks_ln(pdf_pages_attr);
}

/*
If the same keys in a dictionary are given several times, then it is not
defined which value is choosen by an application.  Therefore the keys
|/Producer| and |/Creator| are only set if the token list
|pdf_info_toks| converted to a string does not contain these key strings.
*/

boolean substr_of_str(char *s, char *t)
{
    if (strstr(t, s) == NULL)
        return false;
    return true;
}

void pdf_print_info(integer luatex_version, str_number luatex_revision)
{                               /* print info object */
    boolean creator_given, producer_given, creationdate_given, moddate_given,
        trapped_given;
    char *s = NULL;
    integer len = 0;
    pdf_new_dict(obj_type_others, 0, 3);        /* keep Info readable unless explicitely forced */
    creator_given = false;
    producer_given = false;
    creationdate_given = false;
    moddate_given = false;
    trapped_given = false;
    if (pdf_info_toks != 0) {
        s = tokenlist_to_cstring(pdf_info_toks, true, &len);
        creator_given = substr_of_str("/Creator", s);
        producer_given = substr_of_str("/Producer", s);
        creationdate_given = substr_of_str("/CreationDate", s);
        moddate_given = substr_of_str("/ModDate", s);
        trapped_given = substr_of_str("/Trapped", s);
    }
    if (!producer_given) {
        /* Print the Producer key */
        pdf_printf("/Producer (LuaTeX-");
        pdf_print_int(luatex_version / 100);
        pdf_out('.');
        pdf_print_int(luatex_version % 100);
        pdf_out('.');
        pdf_print(luatex_revision);
        pdf_printf(")\n");

    }
    if (pdf_info_toks != null) {
        if (len > 0) {
            pdf_printf("%s\n", s);
            xfree(s);
        }
        delete_token_ref(pdf_info_toks);
        pdf_info_toks = null;
    }
    if (!creator_given)
        pdf_str_entry_ln("Creator", "TeX");
    if (!creationdate_given) {
        print_creation_date();
    }
    if (!moddate_given) {
        print_mod_date();
    }
    if (!trapped_given) {
        pdf_printf("/Trapped /False\n");
    }
    pdf_str_entry_ln("PTEX.Fullbanner", makecstring(pdftex_banner));
    pdf_end_dict();
}


void build_free_object_list(void)
{
    integer k, l;
    l = 0;
    set_obj_fresh(l);           /* null object at begin of list of free objects */
    for (k = 1; k <= sys_obj_ptr; k++) {
        if (!is_obj_written(k)) {
            set_obj_link(l, k);
            l = k;
        }
    }
    set_obj_link(l, 0);
}

/*
Now the finish of PDF output file. At this moment all Page objects
are already written completely to PDF output file.
*/

void finish_pdf_file(integer luatex_version, str_number luatex_revision)
{
    boolean is_names;           /* flag for name tree output: is it Names or Kids? */
    boolean res;
    integer a, b, i, j, k, l;
    integer root, outlines, threads, names_tree, dests;
    integer xref_offset_width, names_head, names_tail;
    integer callback_id = callback_defined(stop_run_callback);

    if (total_pages == 0) {
        if (callback_id == 0) {
            tprint_nl("No pages of output.");
        } else if (callback_id > 0) {
            res = run_callback(callback_id, "->");
        }
        if (pdf_gone > 0)
            garbage_warning();
    } else {
        if (fixed_pdf_draftmode == 0) {
            pdf_flush();        /* to make sure that the output file name has been already created */
            flush_jbig2_page0_objects();        /* flush page 0 objects from JBIG2 images, if any */
            /* Check for non-existing pages */
            k = head_tab[obj_type_page];
            while (obj_aux(k) == 0) {
                pdf_warning(maketexstring("dest"), maketexstring("Page "),
                            false, false);
                print_int(obj_info(k));
                tprint(" has been referenced but does not exist!");
                print_ln();
                print_ln();
                k = obj_link(k);
            }
            head_tab[obj_type_page] = k;

            /* Check for non-existing destinations */
            k = head_tab[obj_type_dest];
            while (k != 0) {
                pdf_fix_dest(k);
                k = obj_link(k);
            }

            /* Output fonts definition */
            for (k = 1; k <= max_font_id(); k++) {
                if (font_used(k) && (pdf_font_num(k) < 0)) {
                    i = -pdf_font_num(k);
                    assert(pdf_font_num(i) > 0);
                    for (j = font_bc(k); j <= font_ec(k); j++)
                        if (pdf_char_marked(k, j))
                            pdf_mark_char(i, j);
                    if ((pdf_font_attr(i) == 0) && (pdf_font_attr(k) != 0)) {
                        set_pdf_font_attr(i, pdf_font_attr(k));
                    } else if ((pdf_font_attr(k) == 0)
                               && (pdf_font_attr(i) != 0)) {
                        set_pdf_font_attr(k, pdf_font_attr(i));
                    } else if ((pdf_font_attr(i) != 0)
                               && (pdf_font_attr(k) != 0)
                               &&
                               (!str_eq_str
                                (pdf_font_attr(i), pdf_font_attr(k)))) {
                        pdf_warning(maketexstring("\\pdffontattr"),
                                    maketexstring("fonts "), false, false);
                        print_font_identifier(i);
                        tprint(" and ");
                        print_font_identifier(k);
                        tprint
                            (" have conflicting attributes; I will ignore the attributes assigned to ");
                        print_font_identifier(i);
                        print_ln();
                        print_ln();
                    }
                }
            }
            fixed_gen_tounicode = pdf_gen_tounicode;
            k = head_tab[obj_type_font];
            while (k != 0) {
                f = obj_info(k);
                assert(pdf_font_num(f) > 0);
                do_pdf_font(k, f);
                k = obj_link(k);
            }
            write_fontstuff();

            pdf_last_pages = output_pages_tree();
            /* Output outlines */
            /* In the end we must flush PDF objects that cannot be written out
               immediately after shipping out pages. */

            if (pdf_first_outline != 0) {
                pdf_new_dict(obj_type_others, 0, 1);
                outlines = obj_ptr;
                l = pdf_first_outline;
                k = 0;
                do {
                    incr(k);
                    a = open_subentries(l);
                    if (obj_outline_count(l) > 0)
                        k = k + a;
                    set_obj_outline_parent(l, obj_ptr);
                    l = obj_outline_next(l);
                } while (l != 0);
                pdf_printf("/Type /Outlines\n");
                pdf_indirect_ln("First", pdf_first_outline);
                pdf_indirect_ln("Last", pdf_last_outline);
                pdf_int_entry_ln("Count", k);
                pdf_end_dict();
                /* Output PDF outline entries */

                k = head_tab[obj_type_outline];
                while (k != 0) {
                    if (obj_outline_parent(k) == pdf_parent_outline) {
                        if (obj_outline_prev(k) == 0)
                            pdf_first_outline = k;
                        if (obj_outline_next(k) == 0)
                            pdf_last_outline = k;
                    }
                    pdf_begin_dict(k, 1);
                    pdf_indirect_ln("Title", obj_outline_title(k));
                    pdf_indirect_ln("A", obj_outline_action_objnum(k));
                    if (obj_outline_parent(k) != 0)
                        pdf_indirect_ln("Parent", obj_outline_parent(k));
                    if (obj_outline_prev(k) != 0)
                        pdf_indirect_ln("Prev", obj_outline_prev(k));
                    if (obj_outline_next(k) != 0)
                        pdf_indirect_ln("Next", obj_outline_next(k));
                    if (obj_outline_first(k) != 0)
                        pdf_indirect_ln("First", obj_outline_first(k));
                    if (obj_outline_last(k) != 0)
                        pdf_indirect_ln("Last", obj_outline_last(k));
                    if (obj_outline_count(k) != 0)
                        pdf_int_entry_ln("Count", obj_outline_count(k));
                    if (obj_outline_attr(k) != 0) {
                        pdf_print_toks_ln(obj_outline_attr(k));
                        delete_token_ref(obj_outline_attr(k));
                        set_obj_outline_attr(k, null);
                    }
                    pdf_end_dict();
                    k = obj_link(k);
                }

            } else {
                outlines = 0;
            }

            /* Output name tree */
            /* The name tree is very similiar to Pages tree so its construction should be
               certain from Pages tree construction. For intermediate node |obj_info| will be
               the first name and |obj_link| will be the last name in \.{\\Limits} array.
               Note that |pdf_dest_names_ptr| will be less than |obj_ptr|, so we test if
               |k < pdf_dest_names_ptr| then |k| is index of leaf in |dest_names|; else
               |k| will be index in |obj_tab| of some intermediate node.
             */
            if (pdf_dest_names_ptr == 0) {
                dests = 0;
                goto DONE1;
            }
            sort_dest_names(0, pdf_dest_names_ptr - 1);
            names_head = 0;
            names_tail = 0;
            k = 0;              /* index of current child of |l|; if |k < pdf_dest_names_ptr|
                                   then this is pointer to |dest_names| array;
                                   otherwise it is the pointer to |obj_tab| (object number) */
            is_names = true;    /* flag whether Names or Kids */
            b = 0;
            do {
                do {
                    pdf_create_obj(obj_type_others, 0); /* create a new node */
                    l = obj_ptr;
                    if (b == 0)
                        b = l;  /* first in this level */
                    if (names_head == 0) {
                        names_head = l;
                        names_tail = l;
                    } else {
                        set_obj_link(names_tail, l);
                        names_tail = l;
                    }
                    set_obj_link(names_tail, 0);
                    /* Output the current node in this level */
                    pdf_begin_dict(l, 1);
                    j = 0;
                    if (is_names) {
                        set_obj_info(l, dest_names[k].objname);
                        pdf_printf("/Names [");
                        do {
                            pdf_print_str(makecstring(dest_names[k].objname));
                            pdf_out(' ');
                            pdf_print_int(dest_names[k].objnum);
                            pdf_printf(" 0 R ");
                            incr(j);
                            incr(k);
                        } while (!((j == name_tree_kids_max)
                                   || (k == pdf_dest_names_ptr)));
                        pdf_remove_last_space();
                        pdf_printf("]\n");
                        set_obj_aux(l, dest_names[k - 1].objname);
                        if (k == pdf_dest_names_ptr) {
                            is_names = false;
                            k = names_head;
                            b = 0;
                        }
                    } else {
                        set_obj_info(l, obj_info(k));
                        pdf_printf("/Kids [");
                        do {
                            pdf_print_int(k);
                            pdf_printf(" 0 R ");
                            incr(j);
                            set_obj_aux(l, obj_aux(k));
                            k = obj_link(k);
                        } while (!((j == name_tree_kids_max) || (k == b)
                                   || (obj_link(k) == 0)));
                        pdf_remove_last_space();
                        pdf_printf("]\n");
                        if (k == b)
                            b = 0;
                    }
                    pdf_printf("/Limits [");
                    pdf_print_str(makecstring(obj_info(l)));
                    pdf_out(' ');
                    pdf_print_str(makecstring(obj_aux(l)));
                    pdf_printf("]\n");
                    pdf_end_dict();

                } while (b != 0);
                if (k == l) {
                    dests = l;
                    goto DONE1;
                }
            } while (true);
          DONE1:
            if ((dests != 0) || (pdf_names_toks != null)) {
                pdf_new_dict(obj_type_others, 0, 1);
                if (dests != 0)
                    pdf_indirect_ln("Dests", dests);
                if (pdf_names_toks != null) {
                    pdf_print_toks_ln(pdf_names_toks);
                    delete_token_ref(pdf_names_toks);
                    pdf_names_toks = null;
                }
                pdf_end_dict();
                names_tree = obj_ptr;
            } else {
                names_tree = 0;
            }

            /* Output article threads */
            if (head_tab[obj_type_thread] != 0) {
                pdf_new_obj(obj_type_others, 0, 1);
                threads = obj_ptr;
                pdf_out('[');
                k = head_tab[obj_type_thread];
                while (k != 0) {
                    pdf_print_int(k);
                    pdf_printf(" 0 R ");
                    k = obj_link(k);
                }
                pdf_remove_last_space();
                pdf_printf("]\n");
                pdf_end_obj();
                k = head_tab[obj_type_thread];
                while (k != 0) {
                    out_thread(k);
                    k = obj_link(k);
                }
            } else {
                threads = 0;
            }

            /* Output the catalog object */
            pdf_new_dict(obj_type_others, 0, 1);
            root = obj_ptr;
            pdf_printf("/Type /Catalog\n");
            pdf_indirect_ln("Pages", pdf_last_pages);
            if (threads != 0)
                pdf_indirect_ln("Threads", threads);
            if (outlines != 0)
                pdf_indirect_ln("Outlines", outlines);
            if (names_tree != 0)
                pdf_indirect_ln("Names", names_tree);
            if (pdf_catalog_toks != null) {
                pdf_print_toks_ln(pdf_catalog_toks);
                delete_token_ref(pdf_catalog_toks);
                pdf_catalog_toks = null;
            }
            if (pdf_catalog_openaction != 0)
                pdf_indirect_ln("OpenAction", pdf_catalog_openaction);
            pdf_end_dict();

            pdf_print_info(luatex_version, luatex_revision);    /* last candidate for object stream */

            if (pdf_os_enable) {
                pdf_os_switch(true);
                pdf_os_write_objstream();
                pdf_flush();
                pdf_os_switch(false);
                /* Output the cross-reference stream dictionary */
                pdf_new_dict(obj_type_others, 0, 0);
                if ((obj_offset(sys_obj_ptr) / 256) > 16777215)
                    xref_offset_width = 5;
                else if (obj_offset(sys_obj_ptr) > 16777215)
                    xref_offset_width = 4;
                else if (obj_offset(sys_obj_ptr) > 65535)
                    xref_offset_width = 3;
                else
                    xref_offset_width = 2;
                /* Build a linked list of free objects */
                build_free_object_list();
                pdf_printf("/Type /XRef\n");
                pdf_printf("/Index [0 ");
                pdf_print_int(obj_ptr);
                pdf_printf("]\n");
                pdf_int_entry_ln("Size", obj_ptr);
                pdf_printf("/W [1 ");
                pdf_print_int(xref_offset_width);
                pdf_printf(" 1]\n");
                pdf_indirect_ln("Root", root);
                pdf_indirect_ln("Info", obj_ptr - 1);
                if (pdf_trailer_toks != null) {
                    pdf_print_toks_ln(pdf_trailer_toks);
                    delete_token_ref(pdf_trailer_toks);
                    pdf_trailer_toks = null;
                }
                print_ID(output_file_name);
                pdf_print_nl();
                pdf_begin_stream();
                for (k = 0; k <= sys_obj_ptr; k++) {
                    if (!is_obj_written(k)) {   /* a free object */
                        pdf_out(0);
                        pdf_out_bytes(obj_link(k), xref_offset_width);
                        pdf_out(255);
                    } else if (obj_os_idx(k) == -1) {   /* object not in object stream */
                        pdf_out(1);
                        pdf_out_bytes(obj_offset(k), xref_offset_width);
                        pdf_out(0);
                    } else {    /* object in object stream */
                        pdf_out(2);
                        pdf_out_bytes(obj_offset(k), xref_offset_width);
                        pdf_out(obj_os_idx(k));
                    }
                }
                pdf_end_stream();

                pdf_flush();
            } else {
                /* Output the |obj_tab| */
                /* Build a linked list of free objects */
                build_free_object_list();

                pdf_save_offset = pdf_offset;
                pdf_printf("xref\n");
                pdf_printf("0 ");
                pdf_print_int_ln(obj_ptr + 1);
                pdf_print_fw_int(obj_link(0), 10);
                pdf_printf(" 65535 f \n");
                for (k = 1; k <= obj_ptr; k++) {
                    if (!is_obj_written(k)) {
                        pdf_print_fw_int(obj_link(k), 10);
                        pdf_printf(" 00000 f \n");
                    } else {
                        pdf_print_fw_int(obj_offset(k), 10);
                        pdf_printf(" 00000 n \n");
                    }
                }

            }

            /* Output the trailer */
            if (!pdf_os_enable) {
                pdf_printf("trailer\n");
                pdf_printf("<< ");
                pdf_int_entry_ln("Size", sys_obj_ptr + 1);
                pdf_indirect_ln("Root", root);
                pdf_indirect_ln("Info", sys_obj_ptr);
                if (pdf_trailer_toks != null) {
                    pdf_print_toks_ln(pdf_trailer_toks);
                    delete_token_ref(pdf_trailer_toks);
                    pdf_trailer_toks = null;
                }
                print_ID(output_file_name);
                pdf_printf(" >>\n");
            }
            pdf_printf("startxref\n");
            if (pdf_os_enable)
                pdf_print_int_ln(obj_offset(sys_obj_ptr));
            else
                pdf_print_int_ln(pdf_save_offset);
            pdf_printf("%%%%EOF\n");

            pdf_flush();
            if (callback_id == 0) {
                tprint_nl("Output written on ");
                print_file_name(0, output_file_name, 0);
                tprint(" (");
                print_int(total_pages);
                tprint(" page");
                if (total_pages != 1)
                    print_char('s');
                tprint(", ");
                print_int(pdf_offset);
                tprint(" bytes).");
            } else if (callback_id > 0) {
                res = run_callback(callback_id, "->");
            }
        }
        libpdffinish();
        if (fixed_pdf_draftmode == 0)
            b_close(pdf_file);
        else
            pdf_warning(0,
                        maketexstring
                        ("\\pdfdraftmode enabled, not changing output pdf"),
                        true, true);
    }

    if (callback_id == 0) {
        if (log_opened) {
            fprintf(log_file,
                    "\nPDF statistics: %d PDF objects out of %d (max. %d)\n",
                    obj_ptr, obj_tab_size, sup_obj_tab_size);
            if (pdf_os_cntr > 0) {
                fprintf(log_file,
                        " %d compressed objects within %d object stream%s\n",
                        ((pdf_os_cntr - 1) * pdf_os_max_objs + pdf_os_objidx +
                         1), pdf_os_cntr, (pdf_os_cntr > 1 ? "s" : ""));
            }
            fprintf(log_file, " %d named destinations out of %d (max. %d)\n",
                    pdf_dest_names_ptr, dest_names_size, sup_dest_names_size);
            fprintf(log_file,
                    " %d words of extra memory for PDF output out of %d (max. %d)\n",
                    pdf_mem_ptr, pdf_mem_size, sup_pdf_mem_size);
        }
    }
}
