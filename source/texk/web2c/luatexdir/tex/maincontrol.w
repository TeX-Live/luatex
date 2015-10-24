% maincontrol.w
%
% Copyright 2009-2010 Taco Hoekwater <taco@@luatex.org>
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


#include "ptexlib.h"
#include "lua/luatex-api.h"

/* these will move to equivalents.h */

@ @c
#define explicit 1
#define acc_kern 2
#define lp_code_base 2
#define rp_code_base 3
#define ef_code_base 4
#define tag_code 5
#define auto_kern explicit
#define no_lig_code 6

#define prev_depth cur_list.prev_depth_field
#define space_factor cur_list.space_factor_field
#define par_shape_ptr  equiv(par_shape_loc)

#define cur_lang int_par(cur_lang_code)
#define global_defs int_par(global_defs_code)
#define output_box int_par(output_box_code)
#define end_line_char int_par(end_line_char_code)
#define new_line_char int_par(new_line_char_code)
#define tracing_online int_par(tracing_online_code)
#define no_local_whatsits int_par(no_local_whatsits_code)
#define no_local_dirs int_par(no_local_dirs_code)
#define err_help equiv(err_help_loc)
#define text_direction int_par(text_direction_code)
#define every_par equiv(every_par_loc)
#define par_direction int_par(par_direction_code)

#define page_left_offset dimen_par(page_left_offset_code)
#define page_top_offset dimen_par(page_top_offset_code)
#define page_right_offset dimen_par(page_right_offset_code)
#define page_bottom_offset dimen_par(page_bottom_offset_code)
#define px_dimen dimen_par(px_dimen_code)

#define math_eqno_gap_step int_par(math_eqno_gap_step_code)

#define escape_char int_par(escape_char_code)
#define max_dead_cycles int_par(max_dead_cycles_code)
#define tolerance int_par(tolerance_code)
#define mag int_par(mag_code)
#define cat_code_table int_par(cat_code_table_code)

#define par_indent dimen_par(par_indent_code)
#define looseness int_par(looseness_code)
#define space_skip glue_par(space_skip_code)
#define xspace_skip glue_par(xspace_skip_code)
#define math_skip glue_par(math_skip_code)
#define every_vbox equiv(every_vbox_loc)

#define split_top_skip glue_par(split_top_skip_code)
#define split_max_depth dimen_par(split_max_depth_code)

#define hang_indent dimen_par(hang_indent_code)
#define hang_after int_par(hang_after_code)
#define inter_line_penalties_ptr equiv(inter_line_penalties_loc)

#define box(A) eqtb[box_base+(A)].hh.rh
#define cur_font equiv(cur_font_loc)
#define hsize dimen_par(hsize_code)
#define ex_hyphen_char int_par(ex_hyphen_char_code)
#define floating_penalty int_par(floating_penalty_code)

#define mode          cur_list.mode_field
#define tail          cur_list.tail_field
#define head          cur_list.head_field
#define prev_graf     cur_list.pg_field
#define dir_save      cur_list.dirs_field

#define check_filter(A) if (!output_active) lua_node_filter_s(buildpage_filter_callback,lua_key_index(A))

#define var_code 7              /* math code meaning ``use the current family'' */

@ We come now to the |main_control| routine, which contains the master
switch that causes all the various pieces of \TeX\ to do their things,
in the right order.

In a sense, this is the grand climax of the program: It applies all the
tools that we have worked so hard to construct. In another sense, this is
the messiest part of the program: It necessarily refers to other pieces
of code all over the place, so that a person can't fully understand what is
going on without paging back and forth to be reminded of conventions that
are defined elsewhere. We are now at the hub of the web, the central nervous
system that touches most of the other parts and ties them together.
@^brain@>

The structure of |main_control| itself is quite simple. There's a label
called |big_switch|, at which point the next token of input is fetched
using |get_x_token|. Then the program branches at high speed into one of
about 100 possible directions, based on the value of the current
mode and the newly fetched command code; the sum |abs(mode)+cur_cmd|
indicates what to do next. For example, the case `|vmode+letter|' arises
when a letter occurs in vertical mode (or internal vertical mode); this
case leads to instructions that initialize a new paragraph and enter
horizontal mode.

The big |case| statement that contains this multiway switch has been labeled
|reswitch|, so that the program can |goto reswitch| when the next token
has already been fetched. Most of the cases are quite short; they call
an ``action procedure'' that does the work for that case, and then they
either |goto reswitch| or they ``fall through'' to the end of the |case|
statement, which returns control back to |big_switch|. Thus, |main_control|
is not an extremely large procedure, in spite of the multiplicity of things
it must do; it is small enough to be handled by PASCAL compilers that put
severe restrictions on procedure size.
@!@^action procedure@>

One case is singled out for special treatment, because it accounts for most
of \TeX's activities in typical applications. The process of reading simple
text and converting it into |char_node| records, while looking for ligatures
and kerns, is part of \TeX's ``inner loop''; the whole program runs
efficiently when its inner loop is fast, so this part has been written
with particular care.

@c
static halfword main_p;         /* temporary register for list manipulation */
static halfword main_s;         /* space factor value */


@ We leave the |space_factor| unchanged if |sf_code(cur_chr)=0|; otherwise we
set it equal to |sf_code(cur_chr)|, except that it should never change
from a value less than 1000 to a value exceeding 1000. The most common
case is |sf_code(cur_chr)=1000|, so we want that case to be fast.

@c
void adjust_space_factor(void)
{
    main_s = get_sf_code(cur_chr);
    if (main_s == 1000) {
        space_factor = 1000;
    } else if (main_s < 1000) {
        if (main_s > 0)
            space_factor = main_s;
    } else if (space_factor < 1000) {
        space_factor = 1000;
    } else {
        space_factor = main_s;
    }
}


@ From Knuth: ``Having |font_glue| allocated for each text font saves
both time and memory.''  That may be true, but it also punches through
the API wall for fonts, so I removed that -- Taco. But a bit of caching
is very welcome, which is why I need to have the next two globals:

@c
internal_font_number space_spec_font;
halfword space_spec_cache;

@ To handle the execution state of |main_control|'s eternal loop,
an extra global variable is used, along with a macro to define
its values.

@c
#define goto_next 0
#define goto_skip_token 1
#define goto_return 2

static int main_control_state;


@* Main control helpers.

Here are all the functions that are called from |main_control| that
are not already defined elsewhere. For the moment, this list simply
in the order that the appear in |init_main_control|, below.

@
@c
static void run_char_num (void) {
    scan_char_num();
    cur_chr = cur_val;
    adjust_space_factor();
    tail_append(new_char(cur_font, cur_chr));
}

static void run_char (void) {
    adjust_space_factor();
    tail_append(new_char(cur_font, cur_chr));
}

@
The occurrence of blank spaces is almost part of \TeX's inner loop,
since we usually encounter about one space for every five non-blank characters.
Therefore |main_control| gives second-highest priority to ordinary spaces.

When a glue parameter like \.{\\spaceskip} is set to `\.{0pt}', we will
see to it later that the corresponding glue specification is precisely
|zero_glue|, not merely a pointer to some specification that happens
to be full of zeroes. Therefore it is simple to test whether a glue parameter
is zero or~not.


@c
static void run_app_space (void) {
    if ((abs(mode) + cur_cmd == hmode + spacer_cmd)
        && (!(space_factor == 1000))) {
        app_space();
    } else {
        /* Append a normal inter-word space to the current list */
        if (space_skip == zero_glue) {
            /* Find the glue specification, |main_p|, for
               text spaces in the current font */
            if (cur_font != space_spec_font) {
                if (space_spec_cache != zero_glue)
                    delete_glue_ref(space_spec_cache);
                space_spec_cache = new_spec(zero_glue);
                width(space_spec_cache) = space(cur_font);
                stretch(space_spec_cache) = space_stretch(cur_font);
                shrink(space_spec_cache) = space_shrink(cur_font);
                space_spec_font = cur_font;
            }
            main_p = space_spec_cache;

            temp_ptr = new_glue(main_p);
        } else {
            temp_ptr = new_param_glue(space_skip_code);
        }
        couple_nodes(tail,temp_ptr);
        tail = temp_ptr;

    }
}

@ Append a |boundary_node|
@c
static void run_no_boundary (void) {
    halfword n ;
    n = new_node(boundary_node,cancel_boundary);
    couple_nodes(tail, n);
    tail = n;
}
static void run_boundary (void) {
    halfword n ;
    n = new_node(boundary_node,user_boundary);
    scan_int();
    boundary_value(n) = cur_val;
    couple_nodes(tail, n);
    tail = n;
}


@ @c
static void run_char_ghost (void) {
    int t;
    t = cur_chr;
    get_x_token();
    if ((cur_cmd == letter_cmd) || (cur_cmd == other_char_cmd)
        || (cur_cmd == char_given_cmd) || (cur_cmd == char_num_cmd)) {
        halfword p = new_glyph(get_cur_font(), cur_chr);
        if (t == 0) {
            set_is_leftghost(p);
        } else {
            set_is_rightghost(p);
        }
        tail_append(p);
    }
}

@ @c
static void run_relax (void) {
    return;
}

@ |ignore_spaces| is a special case: after it has acted, |get_x_token| has already
fetched the next token from the input, so that operation in |main_control|
should be skipped.

@c
static void run_ignore_spaces (void) {
    if (cur_chr == 0) {
        /* Get the next non-blank non-call... */
        do {
            get_x_token();
        } while (cur_cmd == spacer_cmd);

        main_control_state = goto_skip_token;
    } else {
        int t = scanner_status;
        scanner_status = normal;
        get_next(); /* get_token_lua(); */
        scanner_status = t;
        cur_cs = prim_lookup(cs_text(cur_cs));
        if (cur_cs != undefined_primitive) {
            cur_cmd = get_prim_eq_type(cur_cs);
            cur_chr = get_prim_equiv(cur_cs);
            cur_tok = (cur_cmd * STRING_OFFSET) + cur_chr;
            main_control_state = goto_skip_token;
        }
    }
}

@ |stop| is the second special case. We want |main_control| to return to its caller
if there is nothing left to do.

@c
static void run_stop (void) {
    if (its_all_over())
       main_control_state= goto_return; /* this is the only way out */
}

@ @c
static void run_non_math_math (void) {
    back_input();
    new_graf(true);
}

@ @c
static void run_math_char_num (void) {
    mathcodeval mval;           /* to build up an argument to |set_math_char| */
    if (cur_chr == 0)
        mval = scan_mathchar(tex_mathcode);
    else if (cur_chr == 1)
        mval = scan_mathchar(umath_mathcode);
    else
        mval = scan_mathchar(umathnum_mathcode);
    math_char_in_text(mval);
}

@ @c
static void run_math_given (void) {
    mathcodeval mval;           /* to build up an argument to |set_math_char| */
    mval = mathchar_from_integer(cur_chr, tex_mathcode);
    math_char_in_text(mval);
}

static void run_xmath_given (void) {
    mathcodeval mval;           /* to build up an argument to |set_math_char| */
    mval = mathchar_from_integer(cur_chr, umath_mathcode);
    math_char_in_text(mval);
}

@  The most important parts of |main_control| are concerned with \TeX's
chief mission of box-making. We need to control the activities that put
entries on vlists and hlists, as well as the activities that convert
those lists into boxes. All of the necessary machinery has already been
developed; it remains for us to ``push the buttons'' at the right times.

As an introduction to these routines, let's consider one of the simplest
cases: What happens when `\.{\\hrule}' occurs in vertical mode, or
`\.{\\vrule}' in horizontal mode or math mode? The code in |main_control|
is short, since the |scan_rule_spec| routine already does most of what is
required; thus, there is no need for a special action procedure.

Note that baselineskip calculations are disabled after a rule in vertical
mode, by setting |prev_depth:=ignore_depth|.

@c
static void run_rule (void) {
    tail_append(scan_rule_spec());
    if (abs(mode) == vmode)
        prev_depth = ignore_depth;
    else if (abs(mode) == hmode)
        space_factor = 1000;
}

@
Many of the actions related to box-making are triggered by the appearance
of braces in the input. For example, when the user says `\.{\\hbox}
\.{to} \.{100pt\{$\langle\,\hbox{hlist}\,\rangle$\}}' in vertical mode,
the information about the box size (100pt, |exactly|) is put onto |save_stack|
with a level boundary word just above it, and |cur_group:=adjusted_hbox_group|;
\TeX\ enters restricted horizontal mode to process the hlist. The right
brace eventually causes |save_stack| to be restored to its former state,
at which time the information about the box size (100pt, |exactly|) is
available once again; a box is packaged and we leave restricted horizontal
mode, appending the new box to the current list of the enclosing mode
(in this case to the current list of vertical mode), followed by any
vertical adjustments that were removed from the box by |hpack|.

The next few sections of the program are therefore concerned with the
treatment of left and right curly braces.

If a left brace occurs in the middle of a page or paragraph, it simply
introduces a new level of grouping, and the matching right brace will not have
such a drastic effect. Such grouping affects neither the mode nor the
current list.

@c
static void run_left_brace (void) {
    new_save_level(simple_group);
    eq_word_define(int_base + no_local_whatsits_code, 0);
    eq_word_define(int_base + no_local_dirs_code, 0);
}

static void run_begin_group (void) {
    new_save_level(semi_simple_group);
    eq_word_define(int_base + no_local_whatsits_code, 0);
    eq_word_define(int_base + no_local_dirs_code, 0);
}

static void run_end_group (void) {
    if (cur_group == semi_simple_group) {
        fixup_directions();
    } else {
        off_save();
    }
}

@ Constructions that require a box are started by calling |scan_box| with
a specified context code. The |scan_box| routine verifies
that a |make_box| command comes next and then it calls |begin_box|.

@c
static void run_move (void) {
    int t = cur_chr;
    scan_normal_dimen();
    if (t == 0)
        scan_box(cur_val);
    else
        scan_box(-cur_val);
}

@ @c
static void run_leader_ship (void) {
    scan_box(leader_flag - a_leaders + cur_chr);
}

@ @c
static void run_make_box (void) {
    begin_box(0);
}

@ @c
static void run_box_dir (void) {
    scan_register_num();
    cur_box = box(cur_val);
    scan_optional_equals();
    scan_direction();
    if (cur_box != null)
        box_dir(cur_box) = cur_val;
}

@ There is a really small patch to add a new primitive called
\.{\\quitvmode}. In vertical modes, it is identical to \.{\\indent},
but in horizontal and math modes it is really a no-op (as opposed to
\.{\\indent}, which executes the |indent_in_hmode| procedure).

A paragraph begins when horizontal-mode material occurs in vertical mode,
or when the paragraph is explicitly started by `\.{\\quitvmode}',
`\.{\\indent}' or `\.{\\noindent}'.

@c
static void run_start_par_vmode (void) {
    new_graf((cur_chr > 0));
}

@ @c
static void run_start_par (void) {
   if (cur_chr != 2)
       indent_in_hmode();
}

@ @c
static void run_new_graf (void) {
   back_input();
   new_graf(true);
}

@ A paragraph ends when a |par_end| command is sensed, or when we are in
horizontal mode when reaching the right brace of vertical-mode routines
like \.{\\vbox}, \.{\\insert}, or \.{\\output}.

@c
static void run_par_end_vmode (void) {
    normal_paragraph();
    if (mode > 0) {
        check_filter(vmode_par);
        build_page();
    }
}

@ @c
static void run_par_end_hmode (void) {
    if (align_state < 0)
        off_save();         /* this tries to  recover from an alignment that didn't end properly */
    end_graf(bottom_level); /* this takes us to the enclosing mode, if |mode>0| */
    if (mode == vmode) {
        check_filter(hmode_par);
        build_page();
    }
}

@ @c
static void append_italic_correction_mmode (void) {
    tail_append(new_kern(0));
}

@ @c
static void run_local_box (void) {
    append_local_box(cur_chr);
}

@ @c
static void run_halign_mmode (void) {
    if (privileged()) {
        if (cur_group == math_shift_group)
            init_align();
        else
            off_save();
    }
}

@ @c
static void run_eq_no (void) {
    if (privileged()) {
        if (cur_group == math_shift_group)
            start_eq_no();
        else
            off_save();
    }
}

@ @c
static void run_letter_mmode (void) {
   set_math_char(get_math_code(cur_chr));
}

@ @c
static void run_char_num_mmode (void) {
    scan_char_num();
    cur_chr = cur_val;
    set_math_char(get_math_code(cur_chr));
}

@ @c
static void run_math_char_num_mmode (void) {
    mathcodeval mval;           /* to build up an argument to |set_math_char| */
    if (cur_chr == 0)
        mval = scan_mathchar(tex_mathcode);
    else if (cur_chr == 1)
        mval = scan_mathchar(umath_mathcode);
    else
        mval = scan_mathchar(umathnum_mathcode);
    set_math_char(mval);
}

@ @c
static void run_math_given_mmode (void) {
    mathcodeval mval;           /* to build up an argument to |set_math_char| */
    mval = mathchar_from_integer(cur_chr, tex_mathcode);
    set_math_char(mval);
}

static void run_xmath_given_mmode (void) {
    mathcodeval mval;           /* to build up an argument to |set_math_char| */
    mval = mathchar_from_integer(cur_chr, umath_mathcode);
    set_math_char(mval);
}

@ @c
static void run_delim_num (void) {
    mathcodeval mval;           /* to build up an argument to |set_math_char| */
    if (cur_chr == 0)
        mval = scan_delimiter_as_mathchar(tex_mathcode);
    else
        mval = scan_delimiter_as_mathchar(umath_mathcode);
    set_math_char(mval);

}

@ @c
static void run_vcenter (void) {
    scan_spec(vcenter_group);
    normal_paragraph();
    push_nest();
    mode = -vmode;
    prev_depth = ignore_depth;
    if (every_vbox != null)
        begin_token_list(every_vbox, every_vbox_text);
}

@ @c
static void run_math_style (void) {
    tail_append(new_style((small_number) cur_chr));
}

@ @c
static void run_non_script (void) {
    tail_append(new_glue(zero_glue));
    subtype(tail) = cond_math_glue;
}

@ @c
static void run_math_choice (void) {
    if (cur_chr == 0)
        append_choices();
    else
        setup_math_style();
}

@ @c
static void run_math_shift (void) {
    if (cur_group == math_shift_group)
        after_math();
    else
        off_save();
}

@ @c
static void run_after_assignment (void) {
    get_token();
    after_token = cur_tok;
}

@ @c
static void run_after_group (void) {
    get_token();
    save_for_after(cur_tok);
}

@ @c
static void run_extension (void) {
    do_extension(0);
}

static void run_normal (void) {
{
    switch (cur_chr) {
        case save_pos_code:
            new_whatsit(save_pos_node);
            break;
        case save_cat_code_table_code:
            scan_int();
            if ((cur_val < 0) || (cur_val > 0x7FFF)) {
                print_err("Invalid \\catcode table");
                help1("All \\catcode table ids must be between 0 and 0x7FFF");
                error();
            } else {
                if (cur_val == cat_code_table) {
                    print_err("Invalid \\catcode table");
                    help1("You cannot overwrite the current \\catcode table");
                    error();
                } else {
                    copy_cat_codes(cat_code_table, cur_val);
                }
            }
            break;
        case init_cat_code_table_code:
            scan_int();
            if ((cur_val < 0) || (cur_val > 0x7FFF)) {
                print_err("Invalid \\catcode table");
                help1("All \\catcode table ids must be between 0 and 0x7FFF");
                error();
            } else {
                if (cur_val == cat_code_table) {
                    print_err("Invalid \\catcode table");
                    help1("You cannot overwrite the current \\catcode table");
                    error();
                } else {
                    initex_cat_codes(cur_val);
                }
            }
            break;
        case set_random_seed_code:
            /*  Negative random seed values are silently converted to positive ones */
            scan_int();
            if (cur_val < 0)
                negate(cur_val);
            random_seed = cur_val;
            init_randoms(random_seed);
            break;
        case late_lua_code:
            new_whatsit(late_lua_node); /* type == normal */
            late_lua_name(tail) = scan_lua_state();
            (void) scan_toks(false, false);
            late_lua_data(tail) = def_ref;
            break;
        case expand_font_code:
            read_expand_font();
            break;
        default:
            confusion("int1");
            break;
        }
    }
}


static void run_option(void) {
    switch (cur_chr) {
        case math_option_code:
            if (scan_keyword("compensateitalic")) {
                scan_int();
                math_compensate_italic = cur_val;
            } else if (scan_keyword("alwayscharitalic")) {
                scan_int();
                math_always_char_italic = cur_val;
            } else if (scan_keyword("nodelimitershift")) {
                scan_int();
                math_no_delimiter_shift = cur_val;
            } else {
                normal_warning("mathoption","unknown key",false,false);
            }
            break;
        default:
            /* harmless */
            break;
    }
}


@ For mode-independent commands, the following macro is useful.

Also, there is a list of cases where the user has probably gotten into or out of math
mode by mistake. \TeX\ will insert a dollar sign and rescan the current token, and
it makes sense ot have a macro for that as well.

@c
#define any_mode(A,B) jump_table[vmode+(A)]=B; jump_table[hmode+(A)]=B; jump_table[mmode+(A)]=B
#define non_math(A,B) jump_table[vmode+(A)]=B; jump_table[hmode+(A)]=B;


@ The |main_control| uses a jump table, and |init_main_control| sets that table up.
@c
typedef void (*main_control_function) (void);
main_control_function *jump_table;

static void init_main_control (void) {
    jump_table = xmalloc((mmode+max_command_cmd+1) * sizeof(main_control_function)) ;

    jump_table[hmode + char_num_cmd] = run_char_num;
    jump_table[hmode + letter_cmd] = run_char;
    jump_table[hmode + other_char_cmd] = run_char;
    jump_table[hmode + char_given_cmd] = run_char;
    jump_table[hmode + spacer_cmd] = run_app_space;
    jump_table[hmode + ex_space_cmd] = run_app_space;
    jump_table[mmode + ex_space_cmd] = run_app_space;
    jump_table[hmode + boundary_cmd] = run_boundary;
    jump_table[hmode + no_boundary_cmd] = run_no_boundary;
    jump_table[hmode + char_ghost_cmd] = run_char_ghost;
    jump_table[mmode + char_ghost_cmd] = run_char_ghost;
    any_mode(relax_cmd, run_relax);
    jump_table[vmode + spacer_cmd] = run_relax;
    jump_table[mmode + spacer_cmd] = run_relax;
    jump_table[mmode + boundary_cmd] = run_relax;
    jump_table[mmode + no_boundary_cmd] = run_relax;
    any_mode(ignore_spaces_cmd,run_ignore_spaces);
    jump_table[vmode + stop_cmd] = run_stop;
    jump_table[vmode + math_char_num_cmd] = run_non_math_math;
    jump_table[vmode + math_given_cmd] = run_non_math_math;
    jump_table[vmode + xmath_given_cmd] = run_non_math_math;
    jump_table[hmode + math_char_num_cmd] = run_math_char_num;
    jump_table[hmode + math_given_cmd] = run_math_given;
    jump_table[hmode + xmath_given_cmd] = run_xmath_given;

    jump_table[vmode + vmove_cmd] = report_illegal_case;
    jump_table[hmode + hmove_cmd] = report_illegal_case;
    jump_table[mmode + hmove_cmd] = report_illegal_case;
    any_mode(last_item_cmd, report_illegal_case);
    jump_table[vmode + vadjust_cmd] = report_illegal_case;
    jump_table[vmode + ital_corr_cmd] = report_illegal_case;
    non_math(eq_no_cmd,report_illegal_case);
    any_mode(mac_param_cmd,report_illegal_case);

    non_math(sup_mark_cmd, insert_dollar_sign);
    non_math(sub_mark_cmd, insert_dollar_sign);
    non_math(super_sub_script_cmd, insert_dollar_sign);
    non_math(math_comp_cmd, insert_dollar_sign);
    non_math(delim_num_cmd, insert_dollar_sign);
    non_math(left_right_cmd, insert_dollar_sign);
    non_math(above_cmd, insert_dollar_sign);
    non_math(radical_cmd, insert_dollar_sign);
    non_math(math_style_cmd, insert_dollar_sign);
    non_math(math_choice_cmd, insert_dollar_sign);
    non_math(vcenter_cmd, insert_dollar_sign);
    non_math(non_script_cmd, insert_dollar_sign);
    non_math(mkern_cmd, insert_dollar_sign);
    non_math(limit_switch_cmd, insert_dollar_sign);
    non_math(mskip_cmd, insert_dollar_sign);
    non_math(math_accent_cmd, insert_dollar_sign);
    jump_table[mmode + endv_cmd] =  insert_dollar_sign;
    jump_table[mmode + par_end_cmd] =  insert_dollar_sign_par_end;
    jump_table[mmode + stop_cmd] =  insert_dollar_sign;
    jump_table[mmode + vskip_cmd] =  insert_dollar_sign;
    jump_table[mmode + un_vbox_cmd] =  insert_dollar_sign;
    jump_table[mmode + valign_cmd] =  insert_dollar_sign;
    jump_table[mmode + hrule_cmd] =  insert_dollar_sign;
    jump_table[mmode + no_hrule_cmd] =  insert_dollar_sign;
    jump_table[vmode + hrule_cmd] = run_rule;
    jump_table[vmode + no_hrule_cmd] = run_rule;
    jump_table[hmode + vrule_cmd] = run_rule;
    jump_table[hmode + no_vrule_cmd] = run_rule;
    jump_table[mmode + vrule_cmd] = run_rule;
    jump_table[mmode + no_vrule_cmd] = run_rule;
    jump_table[vmode + vskip_cmd] = append_glue;
    jump_table[hmode + hskip_cmd] = append_glue;
    jump_table[mmode + hskip_cmd] = append_glue;
    jump_table[mmode + mskip_cmd] = append_glue;
    any_mode(kern_cmd, append_kern);
    jump_table[mmode + mkern_cmd] = append_kern;
    non_math(left_brace_cmd, run_left_brace);
    any_mode(begin_group_cmd,run_begin_group);
    any_mode(end_group_cmd, run_end_group);
    any_mode(right_brace_cmd, handle_right_brace);
    jump_table[vmode + hmove_cmd] = run_move;
    jump_table[hmode + vmove_cmd] = run_move;
    jump_table[mmode + vmove_cmd] = run_move;
    any_mode(leader_ship_cmd, run_leader_ship);
    any_mode(make_box_cmd, run_make_box);
    any_mode(assign_box_dir_cmd, run_box_dir);
    jump_table[vmode + start_par_cmd] = run_start_par_vmode;
    jump_table[hmode + start_par_cmd] = run_start_par;
    jump_table[mmode + start_par_cmd] = run_start_par;
    jump_table[vmode + letter_cmd] = run_new_graf;
    jump_table[vmode + other_char_cmd] = run_new_graf;
    jump_table[vmode + char_num_cmd] = run_new_graf;
    jump_table[vmode + char_given_cmd] = run_new_graf;
    jump_table[vmode + char_ghost_cmd] = run_new_graf;
    jump_table[vmode + math_shift_cmd] = run_new_graf;
    jump_table[vmode + math_shift_cs_cmd] = run_new_graf;
    jump_table[vmode + un_hbox_cmd] = run_new_graf;
    jump_table[vmode + vrule_cmd] = run_new_graf;
    jump_table[vmode + no_vrule_cmd] = run_new_graf;
    jump_table[vmode + accent_cmd] = run_new_graf;
    jump_table[vmode + discretionary_cmd] = run_new_graf;
    jump_table[vmode + hskip_cmd] = run_new_graf;
    jump_table[vmode + valign_cmd] = run_new_graf;
    jump_table[vmode + ex_space_cmd] = run_new_graf;
    jump_table[vmode + boundary_cmd] = run_new_graf;
    jump_table[vmode + no_boundary_cmd] = run_new_graf;
    jump_table[vmode + par_end_cmd] = run_par_end_vmode;
    jump_table[hmode + par_end_cmd] = run_par_end_hmode;
    jump_table[hmode + stop_cmd] = head_for_vmode;
    jump_table[hmode + vskip_cmd] = head_for_vmode;
    jump_table[hmode + hrule_cmd] = head_for_vmode;
    jump_table[hmode + no_hrule_cmd] = head_for_vmode;
    jump_table[hmode + un_vbox_cmd] = head_for_vmode;
    jump_table[hmode + halign_cmd] = head_for_vmode;
    any_mode(insert_cmd,begin_insert_or_adjust);
    jump_table[hmode + vadjust_cmd] = begin_insert_or_adjust;
    jump_table[mmode + vadjust_cmd] = begin_insert_or_adjust;
    any_mode(mark_cmd, handle_mark);
    any_mode(break_penalty_cmd, append_penalty);
    any_mode(remove_item_cmd, delete_last);
    jump_table[vmode + un_vbox_cmd] = unpackage;
    jump_table[hmode + un_hbox_cmd] = unpackage;
    jump_table[mmode + un_hbox_cmd] = unpackage;
    jump_table[hmode + ital_corr_cmd] = append_italic_correction;
    jump_table[mmode + ital_corr_cmd] = append_italic_correction_mmode;
    jump_table[hmode + discretionary_cmd] = append_discretionary;
    jump_table[mmode + discretionary_cmd] = append_discretionary;
    any_mode(assign_local_box_cmd, run_local_box);
    jump_table[hmode + accent_cmd] = make_accent;
    any_mode(car_ret_cmd,align_error);
    any_mode(tab_mark_cmd,align_error);
    any_mode(no_align_cmd,no_align_error);
    any_mode(omit_cmd, omit_error);
    jump_table[vmode + halign_cmd] = init_align;
    jump_table[hmode + valign_cmd] = init_align;
    jump_table[mmode + halign_cmd] = run_halign_mmode;
    jump_table[vmode + endv_cmd] = do_endv;
    jump_table[hmode + endv_cmd] = do_endv;
    any_mode(end_cs_name_cmd, cs_error);
    jump_table[hmode + math_shift_cmd] = init_math;
    jump_table[hmode + math_shift_cs_cmd] = init_math;
    jump_table[mmode + eq_no_cmd] = run_eq_no;
    jump_table[mmode + left_brace_cmd] = math_left_brace;
    jump_table[mmode + letter_cmd] = run_letter_mmode;
    jump_table[mmode + other_char_cmd] = run_letter_mmode;
    jump_table[mmode + char_given_cmd] = run_letter_mmode;
    jump_table[mmode + char_num_cmd] = run_char_num_mmode;
    jump_table[mmode + math_char_num_cmd] = run_math_char_num_mmode;
    jump_table[mmode + math_given_cmd] = run_math_given_mmode;
    jump_table[mmode + xmath_given_cmd] = run_xmath_given_mmode;
    jump_table[mmode + delim_num_cmd] = run_delim_num;
    jump_table[mmode + math_comp_cmd] = math_math_comp;
    jump_table[mmode + limit_switch_cmd] = math_limit_switch;
    jump_table[mmode + radical_cmd] = math_radical;
    jump_table[mmode + accent_cmd] = math_ac;
    jump_table[mmode + math_accent_cmd] = math_ac;
    jump_table[mmode + vcenter_cmd] = run_vcenter;
    jump_table[mmode + math_style_cmd] = run_math_style;
    jump_table[mmode + non_script_cmd] = run_non_script;
    jump_table[mmode + math_choice_cmd] = run_math_choice;
    jump_table[mmode + above_cmd] = math_fraction;
    jump_table[mmode + sub_mark_cmd] = sub_sup;
    jump_table[mmode + sup_mark_cmd] = sub_sup;
    jump_table[mmode + super_sub_script_cmd] = sub_sup;
    jump_table[mmode + left_right_cmd] = math_left_right;
    jump_table[mmode + math_shift_cmd] = run_math_shift;
    jump_table[mmode + math_shift_cs_cmd] = run_math_shift;
    any_mode(toks_register_cmd, prefixed_command);
    any_mode(assign_toks_cmd, prefixed_command);
    any_mode(assign_int_cmd, prefixed_command);
    any_mode(assign_attr_cmd, prefixed_command);
    any_mode(assign_dir_cmd, prefixed_command);
    any_mode(assign_dimen_cmd, prefixed_command);
    any_mode(assign_glue_cmd, prefixed_command);
    any_mode(assign_mu_glue_cmd, prefixed_command);
    any_mode(assign_font_dimen_cmd, prefixed_command);
    any_mode(assign_font_int_cmd, prefixed_command);
    any_mode(set_aux_cmd, prefixed_command);
    any_mode(set_prev_graf_cmd, prefixed_command);
    any_mode(set_page_dimen_cmd, prefixed_command);
    any_mode(set_page_int_cmd, prefixed_command);
    any_mode(set_box_dimen_cmd, prefixed_command);
    any_mode(set_tex_shape_cmd, prefixed_command);
    any_mode(set_etex_shape_cmd, prefixed_command);
    any_mode(def_char_code_cmd, prefixed_command);
    any_mode(def_del_code_cmd, prefixed_command);
    any_mode(extdef_math_code_cmd, prefixed_command);
    any_mode(extdef_del_code_cmd, prefixed_command);
    any_mode(def_family_cmd, prefixed_command);
    any_mode(set_math_param_cmd, prefixed_command);
    any_mode(set_font_cmd, prefixed_command);
    any_mode(def_font_cmd, prefixed_command);
    any_mode(letterspace_font_cmd, prefixed_command);
    any_mode(copy_font_cmd, prefixed_command);
    any_mode(register_cmd, prefixed_command);
    any_mode(advance_cmd, prefixed_command);
    any_mode(multiply_cmd, prefixed_command);
    any_mode(divide_cmd, prefixed_command);
    any_mode(prefix_cmd, prefixed_command);
    any_mode(let_cmd, prefixed_command);
    any_mode(shorthand_def_cmd, prefixed_command);
    any_mode(read_to_cs_cmd, prefixed_command);
    any_mode(def_cmd, prefixed_command);
    any_mode(set_box_cmd, prefixed_command);
    any_mode(hyph_data_cmd, prefixed_command);
    any_mode(set_interaction_cmd, prefixed_command);
    any_mode(after_assignment_cmd,run_after_assignment);
    any_mode(after_group_cmd,run_after_group);
    any_mode(in_stream_cmd,open_or_close_in);
    any_mode(message_cmd,issue_message);
    any_mode(case_shift_cmd, shift_case);
    any_mode(xray_cmd, show_whatever);
    any_mode(normal_cmd, run_normal);
    any_mode(extension_cmd, run_extension);
    any_mode(option_cmd, run_option);
}

@ And here is |main_control| itself.  It is quite short nowadays.

@c
void main_control(void)
{
    main_control_state = goto_next;
    init_main_control () ;

    if (equiv(every_job_loc) != null)
        begin_token_list(equiv(every_job_loc), every_job_text);

    while (1) {
	if (main_control_state == goto_skip_token)
            main_control_state = goto_next; /* reset */
        else
            get_x_token();

        /* Give diagnostic information, if requested */
        /* When a new token has just been fetched at |big_switch|, we have an
           ideal place to monitor \TeX's activity. */
        if (interrupt != 0 && OK_to_interrupt) {
            back_input();
            check_interrupt();
            continue;
        }
        if (int_par(tracing_commands_code) > 0)
            show_cur_cmd_chr();

        (jump_table[(abs(mode) + cur_cmd)])(); /* run the command */

        if (main_control_state == goto_return) {
	    return;
        }
    }
    return; /* not reached */
}

@ @c
void app_space(void)
{                               /* handle spaces when |space_factor<>1000| */
    halfword q;                 /* glue node */
    if ((space_factor >= 2000) && (xspace_skip != zero_glue)) {
        q = new_param_glue(xspace_skip_code);
    } else {
        if (space_skip != zero_glue) {
            main_p = new_spec(space_skip);
        } else {
            main_p = new_spec(zero_glue);
            width(main_p) = space(cur_font);
            stretch(main_p) = space_stretch(cur_font);
            shrink(main_p) = space_shrink(cur_font);
        }
        /* Modify the glue specification in |main_p| according to the space factor */
        if (space_factor >= 2000)
            width(main_p) = width(main_p) + extra_space(cur_font);
        stretch(main_p) = xn_over_d(stretch(main_p), space_factor, 1000);
        shrink(main_p) = xn_over_d(shrink(main_p), 1000, space_factor);

        q = new_glue(main_p);
        glue_ref_count(main_p) = null;
    }
    couple_nodes(tail, q);
    tail = q;
}

@ @c
void insert_dollar_sign(void)
{
    back_input();
    cur_tok = math_shift_token + '$';
    print_err("Missing $ inserted");
    help2("I've inserted a begin-math/end-math symbol since I think",
          "you left one out. Proceed, with fingers crossed.");
    ins_error();
}

@  We can silently ignore  \.{\\par}s in a math formula.

@c
void insert_dollar_sign_par_end(void)
{
    if (!int_par(suppress_mathpar_error_code)) {
        insert_dollar_sign() ;
    }
}





@ The `|you_cant|' procedure prints a line saying that the current command
is illegal in the current mode; it identifies these things symbolically.

@c
void you_cant(void)
{
    print_err("You can't use `");
    print_cmd_chr((quarterword) cur_cmd, cur_chr);
    print_in_mode(mode);
}

@
When erroneous situations arise, \TeX\ usually issues an error message
specific to the particular error. For example, `\.{\\noalign}' should
not appear in any mode, since it is recognized by the |align_peek| routine
in all of its legitimate appearances; a special error message is given
when `\.{\\noalign}' occurs elsewhere. But sometimes the most appropriate
error message is simply that the user is not allowed to do what he or she
has attempted. For example, `\.{\\moveleft}' is allowed only in vertical mode,
and `\.{\\lower}' only in non-vertical modes.  Such cases are enumerated
here and in the other sections referred to under `See also \dots.'

@c
void report_illegal_case(void)
{
    you_cant();
    help4("Sorry, but I'm not programmed to handle this case;",
          "I'll just pretend that you didn''t ask for it.",
          "If you're in the wrong mode, you might be able to",
          "return to the right one by typing `I}' or `I$' or `I\\par'.");
    error();
}


@ Some operations are allowed only in privileged modes, i.e., in cases
that |mode>0|. The |privileged| function is used to detect violations
of this rule; it issues an error message and returns |false| if the
current |mode| is negative.

@c
boolean privileged(void)
{
    if (mode > 0) {
        return true;
    } else {
        report_illegal_case();
        return false;
    }
}


@ We don't want to leave |main_control| immediately when a |stop| command
is sensed, because it may be necessary to invoke an \.{\\output} routine
several times before things really grind to a halt. (The output routine
might even say `\.{\\gdef\\end\{...\}}', to prolong the life of the job.)
Therefore |its_all_over| is |true| only when the current page
and contribution list are empty, and when the last output was not a
``dead cycle.''

@c
boolean its_all_over(void)
{                               /* do this when \.{\\end} or \.{\\dump} occurs */
    if (privileged()) {
        if ((page_head == page_tail) && (head == tail) && (dead_cycles == 0)) {
            return true;
        }
        back_input();           /* we will try to end again after ejecting residual material */
        tail_append(new_null_box());
        width(tail) = hsize;
        tail_append(new_glue(fill_glue));
        tail_append(new_penalty(-010000000000));
        lua_node_filter_s(buildpage_filter_callback,lua_key_index(end));
        build_page();           /* append \.{\\hbox to \\hsize\{\}\\vfill\\penalty-'10000000000} */
    }
    return false;
}


@ The |hskip| and |vskip| command codes are used for control sequences
like \.{\\hss} and \.{\\vfil} as well as for \.{\\hskip} and \.{\\vskip}.
The difference is in the value of |cur_chr|.

All the work relating to glue creation has been relegated to the
following subroutine. It does not call |build_page|, because it is
used in at least one place where that would be a mistake.

@c
void append_glue(void)
{
    int s;                      /* modifier of skip command */
    s = cur_chr;
    switch (s) {
    case fil_code:
        cur_val = fil_glue;
        break;
    case fill_code:
        cur_val = fill_glue;
        break;
    case ss_code:
        cur_val = ss_glue;
        break;
    case fil_neg_code:
        cur_val = fil_neg_glue;
        break;
    case skip_code:
        scan_glue(glue_val_level);
        break;
    case mskip_code:
        scan_glue(mu_val_level);
        break;
    }                           /* now |cur_val| points to the glue specification */
    tail_append(new_glue(cur_val));
    if (s >= skip_code) {
        decr(glue_ref_count(cur_val));
        if (s > skip_code)
            subtype(tail) = mu_glue;
    }
}

@ @c
void append_kern(void)
{
    int s;                      /* |subtype| of the kern node */
    s = cur_chr;
    scan_dimen((s == mu_glue), false, false);
    tail_append(new_kern(cur_val));
    subtype(tail) = (quarterword) s;
}


@ We have to deal with errors in which braces and such things are not
properly nested. Sometimes the user makes an error of commission by
inserting an extra symbol, but sometimes the user makes an error of omission.
\TeX\ can't always tell one from the other, so it makes a guess and tries
to avoid getting into a loop.

The |off_save| routine is called when the current group code is wrong. It tries
to insert something into the user's input that will help clean off
the top level.

@c
void off_save(void)
{
    halfword p, q;              /* inserted token */
    if (cur_group == bottom_level) {
        /* Drop current token and complain that it was unmatched */
        print_err("Extra ");
        print_cmd_chr((quarterword) cur_cmd, cur_chr);
        help1("Things are pretty mixed up, but I think the worst is over.");
        error();

    } else {
        back_input();
        p = get_avail();
        set_token_link(temp_token_head, p);
        print_err("Missing ");
        /* Prepare to insert a token that matches |cur_group|, and print what it is */
        /* At this point, |link(temp_token_head)=p|, a pointer to an empty one-word node. */
        switch (cur_group) {
        case semi_simple_group:
            set_token_info(p, cs_token_flag + frozen_end_group);
            tprint_esc("endgroup");
            break;
        case math_shift_group:
            set_token_info(p, math_shift_token + '$');
            print_char('$');
            break;
        case math_left_group:
            set_token_info(p, cs_token_flag + frozen_right);
            q = get_avail();
            set_token_link(p, q);
            p = token_link(p);
            set_token_info(p, other_token + '.');
            tprint_esc("right.");
            break;
        default:
            set_token_info(p, right_brace_token + '}');
            print_char('}');
            break;
        }

        tprint(" inserted");
        ins_list(token_link(temp_token_head));
        help5("I've inserted something that you may have forgotten.",
              "(See the <inserted text> above.)",
              "With luck, this will get me unwedged. But if you",
              "really didn't forget anything, try typing `2' now; then",
              "my insertion and my current dilemma will both disappear.");
        error();
    }
}


@ The routine for a |right_brace| character branches into many subcases,
since a variety of things may happen, depending on |cur_group|. Some
types of groups are not supposed to be ended by a right brace; error
messages are given in hopes of pinpointing the problem. Most branches
of this routine will be filled in later, when we are ready to understand
them; meanwhile, we must prepare ourselves to deal with such errors.

@c
void handle_right_brace(void)
{
    halfword p, q;              /* for short-term use */
    scaled d;                   /* holds |split_max_depth| in |insert_group| */
    int f;                      /* holds |floating_penalty| in |insert_group| */
    p = null;
    switch (cur_group) {
    case simple_group:
        fixup_directions();
        break;
    case bottom_level:
        print_err("Too many }'s");
        help2("You've closed more groups than you opened.",
              "Such booboos are generally harmless, so keep going.");
        error();
        break;
    case semi_simple_group:
    case math_shift_group:
    case math_left_group:
        extra_right_brace();
        break;
    case hbox_group:
        /* When the right brace occurs at the end of an \.{\\hbox} or \.{\\vbox} or
           \.{\\vtop} construction, the |package| routine comes into action. We might
           also have to finish a paragraph that hasn't ended. */
        package(0);
        break;
    case adjusted_hbox_group:
        adjust_tail = adjust_head;
        pre_adjust_tail = pre_adjust_head;
        package(0);
        break;
    case vbox_group:
        end_graf(vbox_group);
        package(0);
        break;
    case vtop_group:
        end_graf(vtop_group);
        package(vtop_code);
        break;
    case insert_group:
        end_graf(insert_group);
        q = split_top_skip;
        add_glue_ref(q);
        d = split_max_depth;
        f = floating_penalty;
        unsave();
        save_ptr--;
        /* now |saved_value(0)| is the insertion number, or the |vadjust| subtype */
        p = vpack(vlink(head), 0, additional, -1);
        pop_nest();
        if (saved_type(0) == saved_insert) {
            tail_append(new_node(ins_node, saved_value(0)));
            height(tail) = height(p) + depth(p);
            ins_ptr(tail) = list_ptr(p);
            split_top_ptr(tail) = q;
            depth(tail) = d;
            float_cost(tail) = f;
        } else if (saved_type(0) == saved_adjust) {
            tail_append(new_node(adjust_node, saved_value(0)));
            adjust_ptr(tail) = list_ptr(p);
            delete_glue_ref(q);
        } else {
            confusion("insert_group");
        }
        list_ptr(p) = null;
        flush_node(p);
        if (nest_ptr == 0) {
            check_filter(insert);
            build_page();
        }
        break;
    case output_group:
	/* this is needed in case the \.{\\output} executes a \.{\\textdir} command. */
	if (dir_level(text_dir_ptr) == cur_level) {
	    /* DIR: Remove from |text_dir_ptr| */
	    halfword text_dir_tmp = vlink(text_dir_ptr);
	    flush_node(text_dir_ptr);
	    text_dir_ptr = text_dir_tmp;
	}
        resume_after_output();
        break;
    case disc_group:
        build_discretionary();
        break;
    case local_box_group:
        build_local_box();
        break;
    case align_group:
        back_input();
        cur_tok = cs_token_flag + frozen_cr;
        print_err("Missing \\cr inserted");
        help1("I'm guessing that you meant to end an alignment here.");
        ins_error();
        break;
    case no_align_group:
        end_graf(no_align_group);
        unsave();
        align_peek();
        break;
    case vcenter_group:
        end_graf(vcenter_group);
        finish_vcenter();
        break;
    case math_choice_group:
        build_choices();
        break;
    case math_group:
        close_math_group(p);
        break;
    default:
        confusion("rightbrace");
        break;
    }
}

@ @c
void extra_right_brace(void)
{
    print_err("Extra }, or forgotten ");
    switch (cur_group) {
    case semi_simple_group:
        tprint_esc("endgroup");
        break;
    case math_shift_group:
        print_char('$');
        break;
    case math_left_group:
        tprint_esc("right");
        break;
    }
    help5("I've deleted a group-closing symbol because it seems to be",
          "spurious, as in `$x}$'. But perhaps the } is legitimate and",
          "you forgot something else, as in `\\hbox{$x}'. In such cases",
          "the way to recover is to insert both the forgotten and the",
          "deleted material, e.g., by typing `I$}'.");
    error();
    incr(align_state);
}


@ Here is where we clear the parameters that are supposed to revert to their
default values after every paragraph and when internal vertical mode is entered.

@c
void normal_paragraph(void)
{
    if (looseness != 0)
        eq_word_define(int_base + looseness_code, 0);
    if (hang_indent != 0)
        eq_word_define(dimen_base + hang_indent_code, 0);
    if (hang_after != 1)
        eq_word_define(int_base + hang_after_code, 1);
    if (par_shape_ptr != null)
        eq_define(par_shape_loc, shape_ref_cmd, null);
    if (inter_line_penalties_ptr != null)
        eq_define(inter_line_penalties_loc, shape_ref_cmd, null);
}


@ The global variable |cur_box| will point to a newly-made box. If the box
is void, we will have |cur_box=null|. Otherwise we will have
|type(cur_box)=hlist_node| or |vlist_node| or |rule_node|; the |rule_node|
case can occur only with leaders.

@c
halfword cur_box;               /* box to be placed into its context */


@ The |box_end| procedure does the right thing with |cur_box|, if
|box_context| represents the context as explained above.

@c
void box_end(int box_context)
{
    if (box_context < box_flag) {
        /* Append box |cur_box| to the current list, shifted by |box_context| */
        /*
           The global variable |adjust_tail| will be non-null if and only if the
           current box might include adjustments that should be appended to the
           current vertical list.
         */
        if (cur_box != null) {
            shift_amount(cur_box) = box_context;
            if (abs(mode) == vmode) {
                if (pre_adjust_tail != null) {
                    if (pre_adjust_head != pre_adjust_tail)
                        append_list(pre_adjust_head, pre_adjust_tail);
                    pre_adjust_tail = null;
                }
                append_to_vlist(cur_box);
                if (adjust_tail != null) {
                    if (adjust_head != adjust_tail)
                        append_list(adjust_head, adjust_tail);
                    adjust_tail = null;
                }
	        if (mode > 0) {
                    check_filter(box);
                    build_page();
                }
            } else {
                if (abs(mode) == hmode)
                    space_factor = 1000;
                else
                    cur_box = new_sub_box(cur_box);
                couple_nodes(tail, cur_box);
                tail = cur_box;
            }
        }

    } else if (box_context < ship_out_flag) {
        /* Store |cur_box| in a box register */
        if (box_context < global_box_flag)
            eq_define(box_base + box_context - box_flag, box_ref_cmd, cur_box);
        else
            geq_define(box_base + box_context - global_box_flag, box_ref_cmd,
                       cur_box);

    } else if (cur_box != null) {
        if (box_context > ship_out_flag) {
            /* Append a new leader node that uses |cur_box| */
            /* Get the next non-blank non-relax... */
            do {
                get_x_token();
            } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));

            if (((cur_cmd == hskip_cmd) && (abs(mode) != vmode)) ||
                ((cur_cmd == vskip_cmd) && (abs(mode) == vmode))) {
                append_glue();
                subtype(tail) =
                    (quarterword) (box_context - (leader_flag - a_leaders));
                leader_ptr(tail) = cur_box;
            } else {
                print_err("Leaders not followed by proper glue");
                help3
                    ("You should say `\\leaders <box or rule><hskip or vskip>'.",
                     "I found the <box or rule>, but there's no suitable",
                     "<hskip or vskip>, so I'm ignoring these leaders.");
                back_error();
                flush_node_list(cur_box);
            }

        } else
            ship_out(static_pdf, cur_box, SHIPPING_PAGE);
    }
}

@ the next input should specify a box or perhaps a rule

@c
void scan_box(int box_context)
{
    /* Get the next non-blank non-relax... */
    do {
        get_x_token();
    } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));

    if (cur_cmd == make_box_cmd) {
        begin_box(box_context);
    } else if ((box_context >= leader_flag) &&
            ((cur_cmd == hrule_cmd) || (cur_cmd == vrule_cmd) ||
             (cur_cmd == no_hrule_cmd) || (cur_cmd == no_vrule_cmd))) {
        cur_box = scan_rule_spec();
        box_end(box_context);
    } else {
        print_err("A <box> was supposed to be here");
        help3("I was expecting to see \\hbox or \\vbox or \\copy or \\box or",
              "something like that. So you might find something missing in",
              "your output. But keep trying; you can fix this later.");
        back_error();
    }
}

@ @c
void new_graf(boolean indented)
{
    halfword p, q, dir_graf_tmp;
    halfword dir_rover;
    prev_graf = 0;
    if ((mode == vmode) || (head != tail)) {
        tail_append(new_param_glue(par_skip_code));
    }
    push_nest();
    mode = hmode;
    space_factor = 1000;
    /* LOCAL: Add local paragraph node */
    tail_append(make_local_par_node());

    if (indented) {
        p = new_null_box();
        box_dir(p) = par_direction;
        width(p) = par_indent;
        subtype(p) = HLIST_SUBTYPE_INDENT;
        q = tail;
        tail_append(p);
    } else {
        q = tail;
    }
    dir_rover = text_dir_ptr;
    while (dir_rover != null) {
        if ((vlink(dir_rover) != null) || (dir_dir(dir_rover) != par_direction)) {
            dir_graf_tmp = new_dir(dir_dir(dir_rover));
            try_couple_nodes(dir_graf_tmp,vlink(q));
            couple_nodes(q,dir_graf_tmp);
        }
        dir_rover = vlink(dir_rover);
    }
    q = head;
    while (vlink(q) != null)
        q = vlink(q);
    tail = q;
    if (every_par != null)
        begin_token_list(every_par, every_par_text);
    if (nest_ptr == 1) {
        check_filter(new_graf);
        build_page();           /* put |par_skip| glue on current page */
    }
}

@ @c
void indent_in_hmode(void)
{
    halfword p;
    if (cur_chr > 0) {          /* \.{\\indent} */
        p = new_null_box();
        width(p) = par_indent;
        if (abs(mode) == hmode)
            space_factor = 1000;
        else
            p = new_sub_box(p);
        tail_append(p);
    }
}

@ @c
void head_for_vmode(void)
{
    if (mode < 0) {
        if (cur_cmd != hrule_cmd) {
            off_save();
        } else {
            print_err("You can't use `\\hrule' here except with leaders");
            help2("To put a horizontal rule in an hbox or an alignment,",
                  "you should use \\leaders or \\hrulefill (see The TeXbook).");
            error();
        }
    } else {
        back_input();
        cur_tok = par_token;
        back_input();
        token_type = inserted;
    }
}


@ TODO (BUG?): |dir_save| would have been set by |line_break| by means
of |post_line_break|, but this is not done right now, as it introduces
pretty heavy memory leaks. This means the current code is probably
wrong in some way that relates to in-paragraph displays.

@c
void end_graf(int line_break_context)
{
    if (mode == hmode) {
        if ((head == tail) || (vlink(head) == tail)) {
            if (vlink(head) == tail)
                flush_node(vlink(head));
            pop_nest();         /* null paragraphs are ignored, all contain a |local_paragraph| node */
        } else {
            line_break(false, line_break_context);
        }
        if (dir_save != null) {
            flush_node_list(dir_save);
            dir_save = null;
        }
        normal_paragraph();
        error_count = 0;
    }
}


@ @c
void begin_insert_or_adjust(void)
{
    if (cur_cmd != vadjust_cmd) {
        scan_register_num();
        if (cur_val == output_box) {
            print_err("You can't \\insert");
            print_int(output_box);
            help1("I'm changing to \\insert0; box \\outputbox is special.");
            error();
            cur_val = 0;
        }
        set_saved_record(0, saved_insert, 0, cur_val);
    } else if (scan_keyword("pre")) {
        set_saved_record(0, saved_adjust, 0, 1);
    } else {
        set_saved_record(0, saved_adjust, 0, 0);
    }
    save_ptr++;
    new_save_level(insert_group);
    scan_left_brace();
    normal_paragraph();
    push_nest();
    mode = -vmode;
    prev_depth = ignore_depth;
}


@ I (TH)'ve renamed the |make_mark| procedure to this, because if the
current chr code is 1, then the actual command was \.{\\clearmarks},
which does not generate a mark node but instead destroys the current
mark tokenlists.

@c
void handle_mark(void)
{
    halfword p;                 /* new node */
    halfword c;                 /* the mark class */
    if (cur_chr == clear_marks_code) {
        scan_mark_num();
        c = cur_val;
        delete_top_mark(c);
        delete_bot_mark(c);
        delete_first_mark(c);
        delete_split_first_mark(c);
        delete_split_bot_mark(c);
    } else {
        if (cur_chr == 0) {
            c = 0;
        } else {
            scan_mark_num();
            c = cur_val;
            if (c > biggest_used_mark)
                biggest_used_mark = c;
        }
        p = scan_toks(false, true);
        p = new_node(mark_node, 0);     /* the |subtype| is not used */
        mark_class(p) = c;
        mark_ptr(p) = def_ref;
        couple_nodes(tail, p);
        tail = p;
    }
}


@ @c
void append_penalty(void)
{
    scan_int();
    tail_append(new_penalty(cur_val));
    if (mode == vmode) {
        check_filter(penalty);
        build_page();
    }
}


@ When |delete_last| is called, |cur_chr| is the |type| of node that
will be deleted, if present.

The |remove_item| command removes a penalty, kern, or glue node if it
appears at the tail of the current list, using a brute-force linear scan.
Like \.{\\lastbox}, this command is not allowed in vertical mode (except
internal vertical mode), since the current list in vertical mode is sent
to the page builder.  But if we happen to be able to implement it in
vertical mode, we do.

@c
void delete_last(void)
{
    halfword p, q;              /* run through the current list */
    if ((mode == vmode) && (tail == head)) {
        /* Apologize for inability to do the operation now,
           unless \.{\\unskip} follows non-glue */
        if ((cur_chr != glue_node) || (last_glue != max_halfword)) {
            you_cant();
            if (cur_chr == kern_node) {
                help2
                    ("Sorry...I usually can't take things from the current page.",
                     "Try `I\\kern-\\lastkern' instead.");
            } else if (cur_chr != glue_node) {
                help2
                    ("Sorry...I usually can't take things from the current page.",
                     "Perhaps you can make the output routine do it.");
            } else {
                help2
                    ("Sorry...I usually can't take things from the current page.",
                     "Try `I\\vskip-\\lastskip' instead.");
            }
            error();
        }

    } else {
        /* todo: clean this up */
        if (!is_char_node(tail)) {
            if (type(tail) == cur_chr) {
                q = head;
                do {
                    p = q;
                    if (!is_char_node(q)) {
                        if (type(q) == disc_node) {
                            if (p == tail)
                                return;
                        }
                    }
                    q = vlink(p);
                } while (q != tail);
                vlink(p) = null;
                flush_node_list(tail);
                tail = p;
            }
        }
    }
}

@ @c
void unpackage(void)
{
    halfword p;                 /* the box */
    halfword r;                 /* to remove marginal kern nodes */
    int c;                      /* should we copy? */
    halfword s;                 /* for varmem assignment */
    if (cur_chr > copy_code) {
        /* Handle saved items and |goto done| */
        try_couple_nodes(tail, disc_ptr[cur_chr]);
        disc_ptr[cur_chr] = null;
        goto DONE;
    }
    c = cur_chr;
    scan_register_num();
    p = box(cur_val);
    if (p == null)
        return;
    if ((abs(mode) == mmode)
        || ((abs(mode) == vmode) && (type(p) != vlist_node))
        || ((abs(mode) == hmode) && (type(p) != hlist_node))) {
        print_err("Incompatible list can't be unboxed");
        help3("Sorry, Pandora. (You sneaky devil.)",
              "I refuse to unbox an \\hbox in vertical mode or vice versa.",
              "And I can't open any boxes in math mode.");
        error();
        return;
    }
    if (c == copy_code) {
        s = copy_node_list(list_ptr(p));
        try_couple_nodes(tail,s);
    } else {
        try_couple_nodes(tail,list_ptr(p));
        box(cur_val) = null;
        list_ptr(p) = null;
        flush_node(p);
    }
  DONE:
    while (vlink(tail) != null) {
        r = vlink(tail);
        if (!is_char_node(r) && (type(r) == margin_kern_node)) {
            try_couple_nodes(tail,vlink(r));
            flush_node(r);
        }
        tail = vlink(tail);
    }
}

@
Italic corrections are converted to kern nodes when the |ital_corr| command
follows a character. In math mode the same effect is achieved by appending
a kern of zero here, since italic corrections are supplied later.

@c
void append_italic_correction(void)
{
    halfword p;                 /* |char_node| at the tail of the current list */
    internal_font_number f;     /* the font in the |char_node| */
    if (tail != head) {
        if (is_char_node(tail))
            p = tail;
        else
            return;
        f = font(p);
        tail_append(new_kern(char_italic(f, character(p))));
        subtype(tail) = explicit;
    }
}


@ @c
void append_local_box(int kind)
{
    incr(save_ptr);
    set_saved_record(-1, saved_boxtype, 0, kind);
    new_save_level(local_box_group);
    scan_left_brace();
    push_nest();
    mode = -hmode;
    space_factor = 1000;
}


@ Discretionary nodes are easy in the common case `\.{\\-}', but in the
general case we must process three braces full of items.

The space factor does not change when we append a discretionary node,
but it starts out as 1000 in the subsidiary lists.

@c
void append_discretionary(void)
{
    int c;
    tail_append(new_disc());
    subtype(tail) = (quarterword) cur_chr;
    if (cur_chr == explicit_disc) {
        /* \- */
        c = get_pre_hyphen_char(cur_lang);
        if (c != 0) {
            vlink(pre_break(tail)) = new_char(equiv(cur_font_loc), c);
            alink(vlink(pre_break(tail))) = pre_break(tail);
            tlink(pre_break(tail)) = vlink(pre_break(tail));
        }
        c = get_post_hyphen_char(cur_lang);
        if (c != 0) {
            vlink(post_break(tail)) = new_char(equiv(cur_font_loc), c);
            alink(vlink(post_break(tail))) = post_break(tail);
            tlink(post_break(tail)) = vlink(post_break(tail));
        }
        disc_penalty(tail) = int_par(ex_hyphen_penalty_code);
    } else {
        /* \discretionary */
        incr(save_ptr);
        set_saved_record(-1, saved_disc, 0, 0);
        new_save_level(disc_group);
        scan_left_brace();
        push_nest();
        mode = -hmode;
        space_factor = 1000;
        /* already preset: disc_penalty(tail) = int_par(hyphen_penalty_code); */
    }
}

@ The test for |p != null| ensures that empty \.{\\localleftbox} and
    \.{\\localrightbox} commands are not applied.

@c
void build_local_box(void)
{
    halfword p;
    int kind;
    unsave();
    assert(saved_type(-1) == saved_boxtype);
    kind = saved_value(-1);
    decr(save_ptr);
    p = vlink(head);
    pop_nest();
    if (p != null)
        p = hpack(p, 0, additional, -1);
    if (kind == 0)
        eq_define(local_left_box_base, box_ref_cmd, p);
    else
        eq_define(local_right_box_base, box_ref_cmd, p);
    if (abs(mode) == hmode) {
        /* LOCAL: Add local paragraph node */
        tail_append(make_local_par_node());
    }
    eq_word_define(int_base + no_local_whatsits_code, no_local_whatsits + 1);
}


@ The three discretionary lists are constructed somewhat as if they were
hboxes. A~subroutine called |build_discretionary| handles the transitions.
(This is sort of fun.)

@c
void build_discretionary(void)
{
    halfword p, q;              /* for link manipulation */
    int n;                      /* length of discretionary list */
    unsave();
    /* Prune the current list, if necessary, until it contains only
       |char_node|, |kern_node|, |hlist_node|, |vlist_node| and
       |rule_node| items; set |n| to the length of the list,
       and set |q| to the lists tail */
    /* During this loop, |p=vlink(q)| and there are |n| items preceding |p|. */
    q = head;
    p = vlink(q);
    n = 0;
    while (p != null) {
        if (!is_char_node(p) && type(p) > rule_node && type(p) != kern_node) {
            print_err("Improper discretionary list");
            help1("Discretionary lists must contain only boxes and kerns.");
            error();
            begin_diagnostic();
            tprint_nl("The following discretionary sublist has been deleted:");
            show_box(p);
            end_diagnostic(true);
            flush_node_list(p);
            vlink(q) = null;
            break;
        }
        alink(p) = q;
        q = p;
        p = vlink(q);
        incr(n);
    }

    p = vlink(head);
    pop_nest();
    assert(saved_type(-1) == saved_disc);
    switch (saved_value(-1)) {
    case 0:
        if (n > 0) {
            vlink(pre_break(tail)) = p;
            alink(p) = pre_break(tail);
            tlink(pre_break(tail)) = q;
        }
        break;
    case 1:
        if (n > 0) {
            vlink(post_break(tail)) = p;
            alink(p) = post_break(tail);
            tlink(post_break(tail)) = q;
        }
        break;
    case 2:
        /* Attach list |p| to the current list, and record its length;
           then finish up and |return| */
        if ((n > 0) && (abs(mode) == mmode)) {
            print_err("Illegal math \\discretionary");
            help2("Sorry: The third part of a discretionary break must be",
                  "empty, in math formulas. I had to delete your third part.");
            flush_node_list(p);
            error();
        } else {
            if (n > 0) {
                vlink(no_break(tail)) = p;
                alink(p) = no_break(tail);
                tlink(no_break(tail)) = q;
            }
        }
        decr(save_ptr);
        return;
        break;
    }                           /* there are no other cases */
    set_saved_record(-1, saved_disc, 0, (saved_value(-1) + 1));
    new_save_level(disc_group);
    scan_left_brace();
    push_nest();
    mode = -hmode;
    space_factor = 1000;
}


@ The positioning of accents is straightforward but tedious. Given an accent
of width |a|, designed for characters of height |x| and slant |s|;
and given a character of width |w|, height |h|, and slant |t|: We will shift
the accent down by |x-h|, and we will insert kern nodes that have the effect of
centering the accent over the character and shifting the accent to the
right by $\delta={1\over2}(w-a)+h\cdot t-x\cdot s$.  If either character is
absent from the font, we will simply use the other, without shifting.

@c
void make_accent(void)
{
    double s, t;                /* amount of slant */
    halfword p, q, r;           /* character, box, and kern nodes */
    internal_font_number f;     /* relevant font */
    scaled a, h, x, w, delta;   /* heights and widths, as explained above */
    scan_char_num();
    f = equiv(cur_font_loc);
    p = new_glyph(f, cur_val);
    if (p != null) {
        x = x_height(f);
        s = float_cast(slant(f)) / float_constant(65536);       /* real division */
        a = glyph_width(p);
        do_assignments();
        /* Create a character node |q| for the next character,
           but set |q:=null| if problems arise */
        q = null;
        f = equiv(cur_font_loc);
        if ((cur_cmd == letter_cmd) ||
            (cur_cmd == other_char_cmd) || (cur_cmd == char_given_cmd)) {
            q = new_glyph(f, cur_chr);
        } else if (cur_cmd == char_num_cmd) {
            scan_char_num();
            q = new_glyph(f, cur_val);
        } else {
            back_input();
        }

        if (q != null) {
            /* Append the accent with appropriate kerns, then set |p:=q| */
            /* The kern nodes appended here must be distinguished from other kerns, lest
               they be wiped away by the hyphenation algorithm or by a previous line break.

               The two kerns are computed with (machine-dependent) |real| arithmetic, but
               their sum is machine-independent; the net effect is machine-independent,
               because the user cannot remove these nodes nor access them via \.{\\lastkern}.
             */
            t = float_cast(slant(f)) / float_constant(65536);   /* real division */
            w = glyph_width(q);
            h = glyph_height(q);
            if (h != x) {       /* the accent must be shifted up or down */
                p = hpack(p, 0, additional, -1);
                shift_amount(p) = x - h;
            }
            delta = round(float_cast(w - a) / float_constant(2) + h * t - x * s);       /* real multiplication */
            r = new_kern(delta);
            subtype(r) = acc_kern;
            couple_nodes(tail, r);
            couple_nodes(r, p);
            tail = new_kern(-a - delta);
            subtype(tail) = acc_kern;
            couple_nodes(p, tail);
            p = q;

        }
        couple_nodes(tail, p);
        tail = p;
        space_factor = 1000;
    }
}


@ When `\.{\\cr}' or `\.{\\span}' or a tab mark comes through the scanner
into |main_control|, it might be that the user has foolishly inserted
one of them into something that has nothing to do with alignment. But it is
far more likely that a left brace or right brace has been omitted, since
|get_next| takes actions appropriate to alignment only when `\.{\\cr}'
or `\.{\\span}' or tab marks occur with |align_state=0|. The following
program attempts to make an appropriate recovery.

@c
void align_error(void)
{
    if (abs(align_state) > 2) {
        /* Express consternation over the fact that no alignment is in progress */
        print_err("Misplaced ");
        print_cmd_chr((quarterword) cur_cmd, cur_chr);
        if (cur_tok == tab_token + '&') {
            help6("I can't figure out why you would want to use a tab mark",
                  "here. If you just want an ampersand, the remedy is",
                  "simple: Just type `I\\&' now. But if some right brace",
                  "up above has ended a previous alignment prematurely,",
                  "you're probably due for more error messages, and you",
                  "might try typing `S' now just to see what is salvageable.");
        } else {
            help5("I can't figure out why you would want to use a tab mark",
                  "or \\cr or \\span just now. If something like a right brace",
                  "up above has ended a previous alignment prematurely,",
                  "you're probably due for more error messages, and you",
                  "might try typing `S' now just to see what is salvageable.");
        }
        error();

    } else {
        back_input();
        if (align_state < 0) {
            print_err("Missing { inserted");
            incr(align_state);
            cur_tok = left_brace_token + '{';
        } else {
            print_err("Missing } inserted");
            decr(align_state);
            cur_tok = right_brace_token + '}';
        }
        help3("I've put in what seems to be necessary to fix",
              "the current column of the current alignment.",
              "Try to go on, since this might almost work.");
        ins_error();
    }
}


@ The help messages here contain a little white lie, since \.{\\noalign}
and \.{\\omit} are allowed also after `\.{\\noalign\{...\}}'.

@c
void no_align_error(void)
{
    print_err("Misplaced \\noalign");
    help2("I expect to see \\noalign only after the \\cr of",
          "an alignment. Proceed, and I'll ignore this case.");
    error();
}

void omit_error(void)
{
    print_err("Misplaced \\omit");
    help2("I expect to see \\omit only after tab marks or the \\cr of",
          "an alignment. Proceed, and I'll ignore this case.");
    error();
}


@ We've now covered most of the abuses of \.{\\halign} and \.{\\valign}.
Let's take a look at what happens when they are used correctly.

An |align_group| code is supposed to remain on the |save_stack|
during an entire alignment, until |fin_align| removes it.

A devious user might force an |endv| command to occur just about anywhere;
we must defeat such hacks.

@c
void do_endv(void)
{
    base_ptr = input_ptr;
    input_stack[base_ptr] = cur_input;
    while ((input_stack[base_ptr].index_field != v_template) &&
           (input_stack[base_ptr].loc_field == null) &&
           (input_stack[base_ptr].state_field == token_list))
        decr(base_ptr);
    if ((input_stack[base_ptr].index_field != v_template) ||
        (input_stack[base_ptr].loc_field != null) ||
        (input_stack[base_ptr].state_field != token_list))
        fatal_error("(interwoven alignment preambles are not allowed)");
    /*.interwoven alignment preambles... */
    if (cur_group == align_group) {
        end_graf(align_group);
        if (fin_col())
            fin_row();
    } else {
        off_save();
    }
}

@ Finally, \.{\\endcsname} is not supposed to get through to |main_control|.

@c
void cs_error(void)
{
    print_err("Extra \\endcsname");
    help1("I'm ignoring this, since I wasn't doing a \\csname.");
    error();
}


@
  Assignments to values in |eqtb| can be global or local. Furthermore, a
  control sequence can be defined to be `\.{\\long}', `\.{\\protected}',
  or `\.{\\outer}', and it might or might not be expanded. The prefixes
  `\.{\\global}', `\.{\\long}', `\.{\\protected}',
  and `\.{\\outer}' can occur in any order. Therefore we assign binary numeric
  codes, making it possible to accumulate the union of all specified prefixes
  by adding the corresponding codes.  (PASCAL's |set| operations could also
  have been used.)

  Every prefix, and every command code that might or might not be prefixed,
  calls the action procedure |prefixed_command|. This routine accumulates
  a sequence of prefixes until coming to a non-prefix, then it carries out
  the command.



@ If the user says, e.g., `\.{\\global\\global}', the redundancy is
silently accepted.


@ The different types of code values have different legal ranges; the
following program is careful to check each case properly.

@c
#define check_def_code(A) do {						\
	if (((cur_val<0)&&(p<(A)))||(cur_val>n)) {			\
	    print_err("Invalid code (");				\
	    print_int(cur_val);						\
	    if (p<(A))							\
		tprint("), should be in the range 0..");		\
	    else							\
		tprint("), should be at most ");			\
	    print_int(n);						\
	    help1("I'm going to use 0 instead of that illegal code value."); \
	    error();							\
	    cur_val=0;							\
	}								\
    } while (0)


@ @c
void prefixed_command(void)
{
    int a;                      /* accumulated prefix codes so far */
    internal_font_number f;     /* identifies a font */
    halfword j;                 /* index into a \.{\\parshape} specification */
    halfword p, q;              /* for temporary short-term use */
    int n;                      /* ditto */
    boolean e;                  /* should a definition be expanded? or was \.{\\let} not done? */
    mathcodeval mval;           /* for handling of \.{\\mathchardef}s */
    a = 0;
    while (cur_cmd == prefix_cmd) {
        if (!odd(a / cur_chr))
            a = a + cur_chr;
        /* Get the next non-blank non-relax... */
        do {
            get_x_token();
        } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));

        if (cur_cmd <= max_non_prefixed_command) {
            /* Discard erroneous prefixes and |return| */
            print_err("You can't use a prefix with `");
            print_cmd_chr((quarterword) cur_cmd, cur_chr);
            print_char('\'');
            help2
                ("I'll pretend you didn't say \\long or \\outer or \\global or",
                 "\\protected.");
            back_error();
            return;
        }
        if (int_par(tracing_commands_code) > 2)
            show_cur_cmd_chr();
    }
    /* Discard the prefixes \.{\\long} and \.{\\outer} if they are irrelevant */
    if (a >= 8) {
        j = protected_token;
        a = a - 8;
    } else {
        j = 0;
    }
    if ((cur_cmd != def_cmd) && ((a % 4 != 0) || (j != 0))) {
        print_err("You can't use `\\long' or `\\outer' or `\\protected' with `");
        print_cmd_chr((quarterword) cur_cmd, cur_chr);
        print_char('\'');
        help1("I'll pretend you didn't say \\long or \\outer or \\protected here.");
        error();
    }

    /* Adjust for the setting of \.{\\globaldefs} */
    if (global_defs != 0) {
        if (global_defs < 0) {
            if (is_global(a))
                a = a - 4;
        } else {
            if (!is_global(a))
                a = a + 4;
        }
    }
    switch (cur_cmd) {
    case set_font_cmd:
        /* Here's an example of the way many of the following routines operate.
           (Unfortunately, they aren't all as simple as this.) */
        define(cur_font_loc, data_cmd, cur_chr);
        break;
    case def_cmd:
        /* When a |def| command has been scanned,
           |cur_chr| is odd if the definition is supposed to be global, and
           |cur_chr>=2| if the definition is supposed to be expanded. */

        if (odd(cur_chr) && !is_global(a) && (global_defs >= 0))
            a = a + 4;
        e = (cur_chr >= 2);
        get_r_token();
        p = cur_cs;
        q = scan_toks(true, e);
        if (j != 0) {
            q = get_avail();
            set_token_info(q, j);
            set_token_link(q, token_link(def_ref));
            set_token_link(def_ref, q);
        }
        define(p, call_cmd + (a % 4), def_ref);
        break;
    case let_cmd:
        n = cur_chr;
        get_r_token();
        p = cur_cs;
        if (n == normal) {
            do {
                get_token();
            } while (cur_cmd == spacer_cmd);
            if (cur_tok == other_token + '=') {
                get_token();
                if (cur_cmd == spacer_cmd)
                    get_token();
            }
        } else {
            get_token();
            q = cur_tok;
            get_token();
            back_input();
            cur_tok = q;
            back_input();       /* look ahead, then back up */
        }                       /* note that |back_input| doesn't affect |cur_cmd|, |cur_chr| */
        if (cur_cmd >= call_cmd)
            add_token_ref(cur_chr);
        define(p, cur_cmd, cur_chr);
        break;
    case shorthand_def_cmd:
        /* We temporarily define |p| to be |relax|, so that an occurrence of |p|
           while scanning the definition will simply stop the scanning instead of
           producing an ``undefined control sequence'' error or expanding the
           previous meaning.  This allows, for instance, `\.{\\chardef\\foo=123\\foo}'.
         */
        n = cur_chr;
        get_r_token();
        p = cur_cs;
        define(p, relax_cmd, too_big_char);
        scan_optional_equals();
        switch (n) {
        case char_def_code:
            scan_char_num();
            define(p, char_given_cmd, cur_val);
            break;
        case math_char_def_code:
            mval = scan_mathchar(tex_mathcode);
            cur_val =
                (mval.class_value * 16 + mval.family_value) * 256 +
                mval.character_value;
            define(p, math_given_cmd, cur_val);
            break;
        case xmath_char_def_code:
            mval = scan_mathchar(umath_mathcode);
            cur_val =
                (mval.class_value + (8 * mval.family_value)) * (65536 * 32) +
                mval.character_value;
            define(p, xmath_given_cmd, cur_val);
            break;
        case umath_char_def_code:
            mval = scan_mathchar(umathnum_mathcode);
            cur_val =
                (mval.class_value + (8 * mval.family_value)) * (65536 * 32) +
                mval.character_value;
            define(p, xmath_given_cmd, cur_val);
            break;
        default:
            scan_register_num();
            switch (n) {
            case count_def_code:
                define(p, assign_int_cmd, count_base + cur_val);
                break;
            case attribute_def_code:
                define(p, assign_attr_cmd, attribute_base + cur_val);
                break;
            case dimen_def_code:
                define(p, assign_dimen_cmd, scaled_base + cur_val);
                break;
            case skip_def_code:
                define(p, assign_glue_cmd, skip_base + cur_val);
                break;
            case mu_skip_def_code:
                define(p, assign_mu_glue_cmd, mu_skip_base + cur_val);
                break;
            case toks_def_code:
                define(p, assign_toks_cmd, toks_base + cur_val);
                break;
            default:
                confusion("shorthand_def");
                break;
            }
            break;
        }
        break;
    case read_to_cs_cmd:
        j = cur_chr;
        scan_int();
        n = cur_val;
        if (!scan_keyword("to")) {
            print_err("Missing `to' inserted");
            help2("You should have said `\\read<number> to \\cs'.",
                  "I'm going to look for the \\cs now.");
            error();
        }
        get_r_token();
        p = cur_cs;
        read_toks(n, p, j);
        define(p, call_cmd, cur_val);
        break;
    case toks_register_cmd:
    case assign_toks_cmd:
        /* The token-list parameters, \.{\\output} and \.{\\everypar}, etc., receive
           their values in the following way. (For safety's sake, we place an
           enclosing pair of braces around an \.{\\output} list.) */
        q = cur_cs;
        if (cur_cmd == toks_register_cmd) {
            scan_register_num();
            p = toks_base + cur_val;
        } else {
            p = cur_chr;        /* |p=every_par_loc| or |output_routine_loc| or \dots */
        }
        scan_optional_equals();
        /* Get the next non-blank non-relax non-call token */
        do {
            get_x_token();
        } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));

        if (cur_cmd != left_brace_cmd) {
            /* If the right-hand side is a token parameter
               or token register, finish the assignment and |goto done| */
            if (cur_cmd == toks_register_cmd) {
                scan_register_num();
                cur_cmd = assign_toks_cmd;
                cur_chr = toks_base + cur_val;
            }
            if (cur_cmd == assign_toks_cmd) {
                q = equiv(cur_chr);
                if (q == null) {
                    define(p, undefined_cs_cmd, null);
                } else {
                    add_token_ref(q);
                    define(p, call_cmd, q);
                }
                goto DONE;
            }
        }
        back_input();
        cur_cs = q;
        q = scan_toks(false, false);
        if (token_link(def_ref) == null) {      /* empty list: revert to the default */
            define(p, undefined_cs_cmd, null);
            free_avail(def_ref);
        } else {
            if (p == output_routine_loc) {      /* enclose in curlies */
                p = get_avail();
                set_token_link(q, p);
                p = output_routine_loc;
                q = token_link(q);
                set_token_info(q, right_brace_token + '}');
                q = get_avail();
                set_token_info(q, left_brace_token + '{');
                set_token_link(q, token_link(def_ref));
                set_token_link(def_ref, q);
            }
            define(p, call_cmd, def_ref);
        }
        break;
    case assign_int_cmd:
        /* Similar routines are used to assign values to the numeric parameters. */
        p = cur_chr;
        scan_optional_equals();
        scan_int();
        assign_internal_value(a, p, cur_val);
        break;
    case assign_attr_cmd:
        p = cur_chr;
        scan_optional_equals();
        scan_int();
        if ((p - attribute_base) > max_used_attr)
            max_used_attr = (p - attribute_base);
        attr_list_cache = cache_disabled;
        word_define(p, cur_val);
        break;
    case assign_dir_cmd:
        /* DIR: Assign direction codes */
        scan_direction();
        switch (cur_chr) {
        case int_base + page_direction_code:
            eq_word_define(int_base + page_direction_code, cur_val);
            break;
        case int_base + body_direction_code:
            eq_word_define(int_base + body_direction_code, cur_val);
            break;
        case int_base + par_direction_code:
            eq_word_define(int_base + par_direction_code, cur_val);
            break;
        case int_base + math_direction_code:
            eq_word_define(int_base + math_direction_code, cur_val);
            break;
        case int_base + text_direction_code:
#if 0
    /* various tests hint that this is unnecessary and
     * sometimes even produces weird results, eg
     *  (\hbox{\textdir TRT ABC\textdir TLT DEF})
     * becomes
     *  (DEFCBA)
     * in the output
     */
            if ((no_local_dirs > 0) && (abs(mode) == hmode)) {
                /* DIR: Add local dir node */
                tail_append(new_dir(text_direction));
            }
#endif
            update_text_dir_ptr(cur_val);
            if (abs(mode) == hmode) {
                /* DIR: Add local dir node */
                tail_append(new_dir(cur_val));
                dir_level(tail) = cur_level;
            }
            eq_word_define(int_base + text_direction_code, cur_val);
            eq_word_define(int_base + no_local_dirs_code, no_local_dirs + 1);
            break;
        }
        break;
    case assign_dimen_cmd:
        p = cur_chr;
        scan_optional_equals();
        scan_normal_dimen();
        assign_internal_value(a, p, cur_val);
        break;
    case assign_glue_cmd:
    case assign_mu_glue_cmd:
        p = cur_chr;
        n = cur_cmd;
        scan_optional_equals();
        if (n == assign_mu_glue_cmd)
            scan_glue(mu_val_level);
        else
            scan_glue(glue_val_level);
        trap_zero_glue();
        define(p, glue_ref_cmd, cur_val);
        break;
    case def_char_code_cmd:
    case def_del_code_cmd:
        /* Let |n| be the largest legal code value, based on |cur_chr| */
        if (cur_chr == cat_code_base)
            n = max_char_code;
        else if (cur_chr == sf_code_base)
            n = 077777;
        else
            n = biggest_char;

        p = cur_chr;
        if (cur_chr == math_code_base) {
            if (is_global(a))
                cur_val1 = level_one;
            else
                cur_val1 = cur_level;
            scan_extdef_math_code(cur_val1, tex_mathcode);
        } else if (cur_chr == lc_code_base) {
            scan_char_num();
            p = cur_val;
            scan_optional_equals();
            scan_int();
            check_def_code(lc_code_base);
            define_lc_code(p, cur_val);
        } else if (cur_chr == uc_code_base) {
            scan_char_num();
            p = cur_val;
            scan_optional_equals();
            scan_int();
            check_def_code(uc_code_base);
            define_uc_code(p, cur_val);
        } else if (cur_chr == sf_code_base) {
            scan_char_num();
            p = cur_val;
            scan_optional_equals();
            scan_int();
            check_def_code(sf_code_base);
            define_sf_code(p, cur_val);
        } else if (cur_chr == cat_code_base) {
            scan_char_num();
            p = cur_val;
            scan_optional_equals();
            scan_int();
            check_def_code(cat_code_base);
            define_cat_code(p, cur_val);
        } else if (cur_chr == del_code_base) {
            if (is_global(a))
                cur_val1 = level_one;
            else
                cur_val1 = cur_level;
            scan_extdef_del_code(cur_val1, tex_mathcode);
        }
        break;
    case extdef_math_code_cmd:
    case extdef_del_code_cmd:
        if (is_global(a))
            cur_val1 = level_one;
        else
            cur_val1 = cur_level;
        if (cur_chr == math_code_base)
            scan_extdef_math_code(cur_val1, umath_mathcode);
        else if (cur_chr == math_code_base + 1)
            scan_extdef_math_code(cur_val1, umathnum_mathcode);
        else if (cur_chr == del_code_base)
            scan_extdef_del_code(cur_val1, umath_mathcode);
        else if (cur_chr == del_code_base + 1)
            scan_extdef_del_code(cur_val1, umathnum_mathcode);
        break;
    case def_family_cmd:
        p = cur_chr;
        scan_math_family_int();
        cur_val1 = cur_val;
        scan_optional_equals();
        scan_font_ident();
        define_fam_fnt(cur_val1, p, cur_val);
        break;
    case set_math_param_cmd:
        p = cur_chr;
        get_token();
        if (cur_cmd != math_style_cmd) {
            print_err("Missing math style, treated as \\displaystyle");
            help1
                ("A style should have been here; I inserted `\\displaystyle'.");
            cur_val1 = display_style;
            back_error();
        } else {
            cur_val1 = cur_chr;
        }
        scan_optional_equals();
        if (p < math_param_first_mu_glue) {
            if (p == math_param_radical_degree_raise)
                scan_int();
            else
                scan_dimen(false, false, false);
        } else {
            scan_glue(mu_val_level);
            trap_zero_glue();
            if (cur_val == glue_par(thin_mu_skip_code))
                cur_val = thin_mu_skip_code;
            else if (cur_val == glue_par(med_mu_skip_code))
                cur_val = med_mu_skip_code;
            else if (cur_val == glue_par(thick_mu_skip_code))
                cur_val = thick_mu_skip_code;
        }
        define_math_param(p, cur_val1, cur_val);
        break;
    case register_cmd:
    case advance_cmd:
    case multiply_cmd:
    case divide_cmd:
        do_register_command(a);
        break;
    case set_box_cmd:
        /* The processing of boxes is somewhat different, because we may need
           to scan and create an entire box before we actually change the value
           of the old one. */
        scan_register_num();
        if (is_global(a))
            n = global_box_flag + cur_val;
        else
            n = box_flag + cur_val;
        scan_optional_equals();
        if (set_box_allowed) {
            scan_box(n);
        } else {
            print_err("Improper \\setbox");
            help2("Sorry, \\setbox is not allowed after \\halign in a display,",
                  "or between \\accent and an accented character.");
            error();
        }
        break;
    case set_aux_cmd:
        /* The |space_factor| or |prev_depth| settings are changed when a |set_aux|
           command is sensed. Similarly, |prev_graf| is changed in the presence of
           |set_prev_graf|, and |dead_cycles| or |insert_penalties| in the presence of
           |set_page_int|. These definitions are always global. */
        alter_aux();
        break;
    case set_prev_graf_cmd:
        alter_prev_graf();
        break;
    case set_page_dimen_cmd:
        alter_page_so_far();
        break;
    case set_page_int_cmd:
        alter_integer();
        break;
    case set_box_dimen_cmd:
        /* When some dimension of a box register is changed, the change isn't exactly
           global; but \TeX\ does not look at the \.{\\global} switch. */
        alter_box_dimen();
        break;
    case set_tex_shape_cmd:
        q = cur_chr;
        scan_optional_equals();
        scan_int();
        n = cur_val;
        if (n <= 0) {
            p = null;
        } else {
            p = new_node(shape_node, 2 * (n + 1) + 1);
            vinfo(p + 1) = n;
            for (j = 1; j <= n; j++) {
                scan_normal_dimen();
                varmem[p + 2 * j].cint = cur_val;       /* indentation */
                scan_normal_dimen();
                varmem[p + 2 * j + 1].cint = cur_val;   /* width */
            }
        }
        define(q, shape_ref_cmd, p);
        break;
    case set_etex_shape_cmd:
        q = cur_chr;
        scan_optional_equals();
        scan_int();
        n = cur_val;
        if (n <= 0) {
            p = null;
        } else {
            n = (cur_val / 2) + 1;
            p = new_node(shape_node, 2 * n + 1 + 1);
            vinfo(p + 1) = n;
            n = cur_val;
            varmem[p + 2].cint = n;     /* number of penalties */
            for (j = p + 3; j <= p + n + 2; j++) {
                scan_int();
                varmem[j].cint = cur_val;       /* penalty values */
            }
            if (!odd(n))
                varmem[p + n + 3].cint = 0;     /* unused */
        }
        define(q, shape_ref_cmd, p);
        break;
    case hyph_data_cmd:
        /* All of \TeX's parameters are kept in |eqtb| except the font information,
           the interaction mode, and the hyphenation tables; these are strictly global.
         */
        switch (cur_chr) {
        case 0:
            new_hyph_exceptions();
            break;
        case 1:
            new_patterns();
            break;
        case 2:
            new_pre_hyphen_char();
            break;
        case 3:
            new_post_hyphen_char();
            break;
        case 4:
            new_pre_exhyphen_char();
            break;
        case 5:
            new_post_exhyphen_char();
            break;
        case 6:
            new_hyphenation_min();
            break;
        }
        break;
    case assign_font_dimen_cmd:
        set_font_dimen();
        break;
    case assign_font_int_cmd:
        n = cur_chr;
        scan_font_ident();
        f = cur_val;
        if (n == no_lig_code) {
            set_no_ligatures(f);
        } else if (n < lp_code_base) {
            scan_optional_equals();
            scan_int();
            if (n == 0)
                set_hyphen_char(f, cur_val);
            else
                set_skew_char(f, cur_val);
        } else {
            scan_char_num();
            p = cur_val;
            scan_optional_equals();
            scan_int();
            switch (n) {
            case lp_code_base:
                set_lp_code(f, p, cur_val);
                break;
            case rp_code_base:
                set_rp_code(f, p, cur_val);
                break;
            case ef_code_base:
                set_ef_code(f, p, cur_val);
                break;
            case tag_code:
                set_tag_code(f, p, cur_val);
                break;
            }
        }
        break;
    case def_font_cmd:
        /* Here is where the information for a new font gets loaded. */
        tex_def_font((small_number) a);
        break;
    case letterspace_font_cmd:
        new_letterspaced_font((small_number) a);
        break;
    case copy_font_cmd:
        make_font_copy((small_number) a);
        break;
    case set_interaction_cmd:
        new_interaction();
        break;
    default:
        confusion("prefix");
        break;
    }                           /* end of Assignments cases */
  DONE:
    /* Insert a token saved by \.{\\afterassignment}, if any */
    if (after_token != 0) {
        cur_tok = after_token;
        back_input();
        after_token = 0;
    }
}

@ @c
void fixup_directions(void)
{
    int temp_no_whatsits;
    int temp_no_dirs;
    int temporary_dir;
    temp_no_whatsits = no_local_whatsits;
    temp_no_dirs = no_local_dirs;
    temporary_dir = text_direction;
    if (dir_level(text_dir_ptr) == cur_level) {
        /* DIR: Remove from |text_dir_ptr| */
        halfword text_dir_tmp = vlink(text_dir_ptr);
        flush_node(text_dir_ptr);
        text_dir_ptr = text_dir_tmp;

    }
    unsave();
    if (abs(mode) == hmode) {
        if (temp_no_dirs != 0) {
            /* DIR: Add local dir node */
            tail_append(new_dir(text_direction));

            dir_dir(tail) = temporary_dir - 64;
        }
        if (temp_no_whatsits != 0) {
            /* LOCAL: Add local paragraph node */
            tail_append(make_local_par_node());

        }
    }
}


@ When a control sequence is to be defined, by \.{\\def} or \.{\\let} or
something similar, the |get_r_token| routine will substitute a special
control sequence for a token that is not redefinable.

@c
void get_r_token(void)
{
  RESTART:
    do {
        get_token();
    } while (cur_tok == space_token);
    if ((cur_cs == 0) || (cur_cs > eqtb_top) ||
        ((cur_cs > frozen_control_sequence) && (cur_cs <= eqtb_size))) {
        print_err("Missing control sequence inserted");
        help5("Please don't say `\\def cs{...}', say `\\def\\cs{...}'.",
              "I've inserted an inaccessible control sequence so that your",
              "definition will be completed without mixing me up too badly.",
              "You can recover graciously from this error, if you're",
              "careful; see exercise 27.2 in The TeXbook.");
        if (cur_cs == 0)
            back_input();
        cur_tok = cs_token_flag + frozen_protection;
        ins_error();
        goto RESTART;
    }
}

@ @c
void assign_internal_value(int a, halfword p, int val)
{
    halfword n;
    if ((p >= int_base) && (p < attribute_base)) {
        switch ((p - int_base)) {
        case cat_code_table_code:
            if (valid_catcode_table(val)) {
                if (val != int_par(cat_code_table_code))
                    word_define(p, val);
            } else {
                print_err("Invalid \\catcode table");
                help2
                    ("You can only switch to a \\catcode table that is initialized",
                     "using \\savecatcodetable or \\initcatcodetable, or to table 0");
                error();
            }
            break;
        case output_box_code:
            if ((val > 65535) | (val < 0)) {
                print_err("Invalid \\outputbox");
                help1
                    ("The value for \\outputbox has to be between 0 and 65535.");
                error();
            } else {
                word_define(p, val);
            }
            break;
        case new_line_char_code:
            if (val > 127) {
                print_err("Invalid \\newlinechar");
                help2
                    ("The value for \\newlinechar has to be no higher than 127.",
                     "Your invalid assignment will be ignored.");
                error();
            } else {
                word_define(p, val);
            }
            break;
        case end_line_char_code:
            if (val > 127) {
                print_err("Invalid \\endlinechar");
                help2
                    ("The value for \\endlinechar has to be no higher than 127.",
                     "Your invalid assignment will be ignored.");
                error();
            } else {
                word_define(p, val);
            }
            break;
        case language_code:
            if (val < 0) {
	        word_define(int_base + cur_lang_code, -1);
                word_define(p, -1);
            } else if (val > 16383) {
                print_err("Invalid \\language");
                help2
                    ("The absolute value for \\language has to be no higher than 16383.",
                     "Your invalid assignment will be ignored.");
                error();
            } else {
	        word_define(int_base + cur_lang_code, val);
                word_define(p, val);
            }
            break;
        default:
            word_define(p, val);
            break;
        }
        /* If we are defining subparagraph penalty levels while we are
           in hmode, then we put out a whatsit immediately, otherwise
           we leave it alone.  This mechanism might not be sufficiently
           powerful, and some other algorithm, searching down the stack,
           might be necessary.  Good first step. */
        if ((abs(mode) == hmode) &&
            ((p == (int_base + local_inter_line_penalty_code)) ||
             (p == (int_base + local_broken_penalty_code)))) {
            /* LOCAL: Add local paragraph node */
            tail_append(make_local_par_node());

            eq_word_define(int_base + no_local_whatsits_code,
                           no_local_whatsits + 1);
        }

    } else if ((p >= dimen_base) && (p <= eqtb_size)) {
        if (p == (dimen_base + page_left_offset_code)) {
            n = val - one_true_inch;
            word_define(dimen_base + h_offset_code, n);
        } else if (p == (dimen_base + h_offset_code)) {
            n = val + one_true_inch;
            word_define(dimen_base + page_left_offset_code, n);
        } else if (p == (dimen_base + page_top_offset_code)) {
            n = val - one_true_inch;
            word_define(dimen_base + v_offset_code, n);
        } else if (p == (dimen_base + v_offset_code)) {
            n = val + one_true_inch;
            word_define(dimen_base + page_top_offset_code, n);
        }
        word_define(p, val);

    } else if ((p >= local_base) && (p < toks_base)) {  /* internal locals  */
        define(p, call_cmd, val);
    } else {
        confusion("assign internal value");
    }
}


@ When a glue register or parameter becomes zero, it will always point to
|zero_glue| because of the following procedure. (Exception: The tabskip
glue isn't trapped while preambles are being scanned.)

@c
void trap_zero_glue(void)
{
    if ((width(cur_val) == 0) && (stretch(cur_val) == 0)
        && (shrink(cur_val) == 0)) {
        add_glue_ref(zero_glue);
        delete_glue_ref(cur_val);
        cur_val = zero_glue;
    }
}

@ We use the fact that |register<advance<multiply<divide|

@c
void do_register_command(int a)
{
    halfword l, q, r, s;        /* for list manipulation */
    int p;                      /* type of register involved */
    q = cur_cmd;
    /* Compute the register location |l| and its type |p|; but |return| if invalid */
    /* Here we use the fact that the consecutive codes |int_val..mu_val| and
       |assign_int..assign_mu_glue| correspond to each other nicely. */
    l = 0;
    if (q != register_cmd) {
        get_x_token();
        if ((cur_cmd >= assign_int_cmd) && (cur_cmd <= assign_mu_glue_cmd)) {
            l = cur_chr;
            p = cur_cmd - assign_int_cmd;
            goto FOUND;
        }
        if (cur_cmd != register_cmd) {
            print_err("You can't use `");
            print_cmd_chr((quarterword) cur_cmd, cur_chr);
            tprint("' after ");
            print_cmd_chr((quarterword) q, 0);
            help1("I'm forgetting what you said and not changing anything.");
            error();
            return;
        }
    }
    p = cur_chr;
    scan_register_num();
    if (p == int_val_level)
        l = cur_val + count_base;
    else if (p == attr_val_level)
        l = cur_val + attribute_base;
    else if (p == dimen_val_level)
        l = cur_val + scaled_base;
    else if (p == glue_val_level)
        l = cur_val + skip_base;
    else if (p == mu_val_level)
        l = cur_val + mu_skip_base;

  FOUND:
    if (q == register_cmd) {
        scan_optional_equals();
    } else if (scan_keyword("by")) {
        ;                       /* optional `\.{by}' */
    }
    arith_error = false;
    if (q < multiply_cmd) {
        /* Compute result of |register| or |advance|, put it in |cur_val| */
        if (p < glue_val_level) {
            if ((p == int_val_level) || (p == attr_val_level))
                scan_int();
            else
                scan_normal_dimen();
            if (q == advance_cmd)
                cur_val = cur_val + eqtb[l].cint;
        } else {
            scan_glue(p);
            if (q == advance_cmd) {
                /* Compute the sum of two glue specs */
                q = new_spec(cur_val);
                r = equiv(l);
                delete_glue_ref(cur_val);
                width(q) = width(q) + width(r);
                if (stretch(q) == 0) {
                    stretch_order(q) = normal;
                }
                if (stretch_order(q) == stretch_order(r)) {
                    stretch(q) = stretch(q) + stretch(r);
                } else if ((stretch_order(q) < stretch_order(r))
                           && (stretch(r) != 0)) {
                    stretch(q) = stretch(r);
                    stretch_order(q) = stretch_order(r);
                }
                if (shrink(q) == 0) {
                    shrink_order(q) = normal;
                }
                if (shrink_order(q) == shrink_order(r)) {
                    shrink(q) = shrink(q) + shrink(r);
                } else if ((shrink_order(q) < shrink_order(r))
                           && (shrink(r) != 0)) {
                    shrink(q) = shrink(r);
                    shrink_order(q) = shrink_order(r);
                }
                cur_val = q;
            }
        }

    } else {
        /* Compute result of |multiply| or |divide|, put it in |cur_val| */
        scan_int();
        if (p < glue_val_level) {
            if (q == multiply_cmd) {
                if ((p == int_val_level) || (p == attr_val_level)) {
                    cur_val = mult_integers(eqtb[l].cint, cur_val);
                } else {
                    cur_val = nx_plus_y(eqtb[l].cint, cur_val, 0);
                }
            } else {
                cur_val = x_over_n(eqtb[l].cint, cur_val);
            }
        } else {
            s = equiv(l);
            r = new_spec(s);
            if (q == multiply_cmd) {
                width(r) = nx_plus_y(width(s), cur_val, 0);
                stretch(r) = nx_plus_y(stretch(s), cur_val, 0);
                shrink(r) = nx_plus_y(shrink(s), cur_val, 0);
            } else {
                width(r) = x_over_n(width(s), cur_val);
                stretch(r) = x_over_n(stretch(s), cur_val);
                shrink(r) = x_over_n(shrink(s), cur_val);
            }
            cur_val = r;
        }

    }
    if (arith_error) {
        print_err("Arithmetic overflow");
        help2("I can't carry out that multiplication or division,",
              "since the result is out of range.");
        if (p >= glue_val_level)
            delete_glue_ref(cur_val);
        error();
        return;
    }
    if (p < glue_val_level) {
        if (p == attr_val_level) {
            if ((l - attribute_base) > max_used_attr)
                max_used_attr = (l - attribute_base);
            attr_list_cache = cache_disabled;
        }
        if ((p == int_val_level) || (p == dimen_val_level))
            assign_internal_value(a, l, cur_val);
        else
            word_define(l, cur_val);
    } else {
        trap_zero_glue();
        define(l, glue_ref_cmd, cur_val);
    }
}


@ @c
void alter_aux(void)
{
    halfword c;                 /* |hmode| or |vmode| */
    if (cur_chr != abs(mode)) {
        report_illegal_case();
    } else {
        c = cur_chr;
        scan_optional_equals();
        if (c == vmode) {
            scan_normal_dimen();
            prev_depth = cur_val;
        } else {
            scan_int();
            if ((cur_val <= 0) || (cur_val > 32767)) {
                print_err("Bad space factor");
                help1("I allow only values in the range 1..32767 here.");
                int_error(cur_val);
            } else {
                space_factor = cur_val;
            }
        }
    }
}

@ @c
void alter_prev_graf(void)
{
    int p;                      /* index into |nest| */
    p = nest_ptr;
    while (abs(nest[p].mode_field) != vmode)
        decr(p);
    scan_optional_equals();
    scan_int();
    if (cur_val < 0) {
        print_err("Bad \\prevgraf");
        help1("I allow only nonnegative values here.");
        int_error(cur_val);
    } else {
        nest[p].pg_field = cur_val;
    }
}

@ @c
void alter_page_so_far(void)
{
    int c;                      /* index into |page_so_far| */
    c = cur_chr;
    scan_optional_equals();
    scan_normal_dimen();
    page_so_far[c] = cur_val;
}

@ @c
void alter_integer(void)
{
    int c;                      /* 0 for \.{\\deadcycles}, 1 for \.{\\insertpenalties}, etc. */
    c = cur_chr;
    scan_optional_equals();
    scan_int();
    if (c == 0) {
        dead_cycles = cur_val;
    } else if (c == 2) {
        if ((cur_val < batch_mode) || (cur_val > error_stop_mode)) {
            print_err("Bad interaction mode");
            help2("Modes are 0=batch, 1=nonstop, 2=scroll, and",
                  "3=errorstop. Proceed, and I'll ignore this case.");
            int_error(cur_val);
        } else {
            cur_chr = cur_val;
            new_interaction();
        }
    } else {
        insert_penalties = cur_val;
    }
}

@ @c
void alter_box_dimen(void)
{
    int c;                      /* |width_offset| or |height_offset| or |depth_offset| */
    int b;                      /* box number */
    c = cur_chr;
    scan_register_num();
    b = cur_val;
    scan_optional_equals();
    scan_normal_dimen();
    if (box(b) != null)
        varmem[box(b) + c].cint = cur_val;
}


@ @c
void new_interaction(void)
{
    print_ln();
    interaction = cur_chr;
    if (interaction == batch_mode)
        kpse_make_tex_discard_errors = 1;
    else
        kpse_make_tex_discard_errors = 0;
    fixup_selector(log_opened_global);
}


@ The \.{\\afterassignment} command puts a token into the global
variable |after_token|. This global variable is examined just after
every assignment has been performed.

@c
halfword after_token;           /* zero, or a saved token */


@ Here is a procedure that might be called `Get the next non-blank non-relax
non-call non-assignment token'.

@c
void do_assignments(void)
{
    while (true) {
        /* Get the next non-blank non-relax... */
        do {
            get_x_token();
        } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));

        if (cur_cmd <= max_non_prefixed_command)
            return;
        set_box_allowed = false;
        prefixed_command();
        set_box_allowed = true;
    }
}

@ @c
void open_or_close_in(void)
{
    int c;                      /* 1 for \.{\\openin}, 0 for \.{\\closein} */
    int n;                      /* stream number */
    char *fn;
    c = cur_chr;
    scan_four_bit_int();
    n = cur_val;
    if (read_open[n] != closed) {
        lua_a_close_in(read_file[n], (n + 1));
        read_open[n] = closed;
    }
    if (c != 0) {
        scan_optional_equals();
        do {
            get_x_token();
        } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));
        back_input();
        if (cur_cmd != left_brace_cmd) {
            scan_file_name();   /* set |cur_name| to desired file name */
            if (cur_ext == get_nullstr())
                cur_ext = maketexstring(".tex");
        } else {
            scan_file_name_toks();
        }
        fn = pack_file_name(cur_name, cur_area, cur_ext);
        if (lua_a_open_in(&(read_file[n]), fn, (n + 1))) {
            read_open[n] = just_open;
        }
    }
}

@ @c
boolean long_help_seen;         /* has the long \.{\\errmessage} help been used? */

void issue_message(void)
{
    int old_setting;            /* holds |selector| setting */
    int c;                      /* identifies \.{\\message} and \.{\\errmessage} */
    str_number s;               /* the message */
    c = cur_chr;
    (void) scan_toks(false, true);
    old_setting = selector;
    selector = new_string;
    token_show(def_ref);
    selector = old_setting;
    flush_list(def_ref);
    str_room(1);
    s = make_string();
    if (c == 0) {
        /* Print string |s| on the terminal */
        if (term_offset + (int) str_length(s) > max_print_line - 2)
            print_ln();
        else if ((term_offset > 0) || (file_offset > 0))
            print_char(' ');
        print(s);
        update_terminal();

    } else {
        /* Print string |s| as an error message */
        /* If \.{\\errmessage} occurs often in |scroll_mode|, without user-defined
           \.{\\errhelp}, we don't want to give a long help message each time. So we
           give a verbose explanation only once. */
        print_err("");
        print(s);
        if (err_help != null) {
            use_err_help = true;
        } else if (long_help_seen) {
            help1("(That was another \\errmessage.)");
        } else {
            if (interaction < error_stop_mode)
                long_help_seen = true;
            help4("This error message was generated by an \\errmessage",
                  "command, so I can't give any explicit help.",
                  "Pretend that you're Hercule Poirot: Examine all clues,",
                  "and deduce the truth by order and method.");
        }
        error();
        use_err_help = false;

    }
    flush_str(s);
}


@ The |error| routine calls on |give_err_help| if help is requested from
the |err_help| parameter.

@c
void give_err_help(void)
{
    token_show(err_help);
}



@ The \.{\\uppercase} and \.{\\lowercase} commands are implemented by
building a token list and then changing the cases of the letters in it.

@c
void shift_case(void)
{
    halfword b;                 /* |lc_code_base| or |uc_code_base| */
    halfword p;                 /* runs through the token list */
    halfword t;                 /* token */
    halfword c;                 /* character code */
    halfword i;                 /* inbetween */
    b = cur_chr;
    p = scan_toks(false, false);
    p = token_link(def_ref);
    while (p != null) {
        /* Change the case of the token in |p|, if a change is appropriate */
        /*
           When the case of a |chr_code| changes, we don't change the |cmd|.
           We also change active characters.
         */
        t = token_info(p);
        if (t < cs_token_flag) {
            c = t % STRING_OFFSET;
            if (b == uc_code_base)
                i = get_uc_code(c);
            else
                i = get_lc_code(c);
            if (i != 0)
                set_token_info(p, t - c + i);
        } else if (is_active_cs(cs_text(t - cs_token_flag))) {
            c = active_cs_value(cs_text(t - cs_token_flag));
            if (b == uc_code_base)
                i = get_uc_code(c);
            else
                i = get_lc_code(c);
            if (i != 0)
                set_token_info(p, active_to_cs(i, true) + cs_token_flag);
        }
        p = token_link(p);
    }
    back_list(token_link(def_ref));
    free_avail(def_ref);        /* omit reference count */
}


@ We come finally to the last pieces missing from |main_control|, namely the
`\.{\\show}' commands that are useful when debugging.

@c
void show_whatever(void)
{
    halfword p;                 /* tail of a token list to show */
    int t;                      /* type of conditional being shown */
    int m;                      /* upper bound on |fi_or_else| codes */
    int l;                      /* line where that conditional began */
    int n;                      /* level of \.{\\if...\\fi} nesting */
    switch (cur_chr) {
    case show_lists:
        begin_diagnostic();
        show_activities();
        break;
    case show_box_code:
        /* Show the current contents of a box */
        scan_register_num();
        begin_diagnostic();
        tprint_nl("> \\box");
        print_int(cur_val);
        print_char('=');
        if (box(cur_val) == null)
            tprint("void");
        else
            show_box(box(cur_val));
        break;
    case show_code:
        /* Show the current meaning of a token, then |goto common_ending| */
        get_token();
        if (interaction == error_stop_mode)
            wake_up_terminal();
        tprint_nl("> ");
        if (cur_cs != 0) {
            sprint_cs(cur_cs);
            print_char('=');
        }
        print_meaning();
        goto COMMON_ENDING;
        break;
        /* Cases for |show_whatever| */
    case show_groups:
        begin_diagnostic();
        show_save_groups();
        break;
    case show_ifs:
        begin_diagnostic();
        tprint_nl("");
        print_ln();
        if (cond_ptr == null) {
            tprint_nl("### ");
            tprint("no active conditionals");
        } else {
            p = cond_ptr;
            n = 0;
            do {
                incr(n);
                p = vlink(p);
            } while (p != null);
            p = cond_ptr;
            t = cur_if;
            l = if_line;
            m = if_limit;
            do {
                tprint_nl("### level ");
                print_int(n);
                tprint(": ");
                print_cmd_chr(if_test_cmd, t);
                if (m == fi_code)
                    tprint_esc("else");
                print_if_line(l);
                decr(n);
                t = if_limit_subtype(p);
                l = if_line_field(p);
                m = if_limit_type(p);
                p = vlink(p);
            } while (p != null);
        }
        break;

    default:
        /* Show the current value of some parameter or register,
           then |goto common_ending| */
        p = the_toks();
        if (interaction == error_stop_mode)
            wake_up_terminal();
        tprint_nl("> ");
        token_show(temp_token_head);
        flush_list(token_link(temp_token_head));
        goto COMMON_ENDING;
        break;
    }
    /* Complete a potentially long \.{\\show} command */
    end_diagnostic(true);
    print_err("OK");
    if (selector == term_and_log) {
        if (tracing_online <= 0) {
            selector = term_only;
            tprint(" (see the transcript file)");
            selector = term_and_log;
        }
    }

  COMMON_ENDING:
    if (interaction < error_stop_mode) {
        help0();
        decr(error_count);
    } else if (tracing_online > 0) {
        help3("This isn't an error message; I'm just \\showing something.",
              "Type `I\\show...' to show more (e.g., \\show\\cs,",
              "\\showthe\\count10, \\showbox255, \\showlists).");
    } else {
        help5("This isn't an error message; I'm just \\showing something.",
              "Type `I\\show...' to show more (e.g., \\show\\cs,",
              "\\showthe\\count10, \\showbox255, \\showlists).",
              "And type `I\\tracingonline=1\\show...' to show boxes and",
              "lists on your terminal as well as in the transcript file.");
    }
    error();
}


@ @c
void initialize(void)
{                               /* this procedure gets things started properly */
    int k;                      /* index into |mem|, |eqtb|, etc. */
    /* Initialize whatever \TeX\ might access */
    /* Set initial values of key variables */
    initialize_errors();
    initialize_arithmetic();
    max_used_attr = -1;
    attr_list_cache = cache_disabled;
    initialize_nesting();

    /* Start a new current page */
    page_contents = empty;
    page_tail = page_head;
#if 0
    vlink(page_head) = null;
#endif
    last_glue = max_halfword;
    last_penalty = 0;
    last_kern = 0;
    last_node_type = -1;
    page_depth = 0;
    page_max_depth = 0;

    initialize_equivalents();
    no_new_control_sequence = true;     /* new identifiers are usually forbidden */
    init_primitives();

    mag_set = 0;
    initialize_marks();
    initialize_read();

    assert(static_pdf == NULL);
    static_pdf = init_pdf_struct(static_pdf);

    format_ident = 0;
    format_name = get_nullstr();
    for (k = 0; k <= 17; k++)
        write_open[k] = false;
    initialize_directions();
    seconds_and_micros(epochseconds, microseconds);
    init_start_time(static_pdf);

    edit_name_start = 0;
    stop_at_space = true;

    if (ini_version) {
        /* Initialize table entries (done by \.{INITEX} only) */

        init_node_mem(500);
        initialize_tokens();
        /* Initialize the special list heads and constant nodes */
        initialize_alignments();
        initialize_buildpage();

        initialize_active();

        set_eq_type(undefined_control_sequence, undefined_cs_cmd);
        set_equiv(undefined_control_sequence, null);
        set_eq_level(undefined_control_sequence, level_zero);
        for (k = null_cs; k <= (eqtb_top - 1); k++)
            eqtb[k] = eqtb[undefined_control_sequence];
        set_equiv(glue_base, zero_glue);
        set_eq_level(glue_base, level_one);
        set_eq_type(glue_base, glue_ref_cmd);
        for (k = glue_base + 1; k <= local_base - 1; k++)
            eqtb[k] = eqtb[glue_base];
        glue_ref_count(zero_glue) =
            glue_ref_count(zero_glue) + local_base - glue_base;

        par_shape_ptr = null;
        set_eq_type(par_shape_loc, shape_ref_cmd);
        set_eq_level(par_shape_loc, level_one);
        for (k = etex_pen_base; k <= (etex_pens - 1); k++)
            eqtb[k] = eqtb[par_shape_loc];
        for (k = output_routine_loc; k <= toks_base + biggest_reg; k++)
            eqtb[k] = eqtb[undefined_control_sequence];
        box(0) = null;
        set_eq_type(box_base, box_ref_cmd);
        set_eq_level(box_base, level_one);
        for (k = box_base + 1; k <= (box_base + biggest_reg); k++)
            eqtb[k] = eqtb[box_base];
        cur_font = null_font;
        set_eq_type(cur_font_loc, data_cmd);
        set_eq_level(cur_font_loc, level_one);
        set_equiv(cat_code_base, 0);
        set_eq_type(cat_code_base, data_cmd);
        set_eq_level(cat_code_base, level_one);
        eqtb[internal_math_param_base] = eqtb[cat_code_base];
        eqtb[lc_code_base] = eqtb[cat_code_base];
        eqtb[uc_code_base] = eqtb[cat_code_base];
        eqtb[sf_code_base] = eqtb[cat_code_base];
        eqtb[math_code_base] = eqtb[cat_code_base];
        cat_code_table = 0;
        initialize_math_codes();
        initialize_text_codes();
        initex_cat_codes(0);
        for (k = '0'; k <= '9'; k++)
            set_math_code(k, tex_mathcode, var_code, 0, k, level_one);
        for (k = 'A'; k <= 'Z'; k++) {
            set_math_code(k, tex_mathcode, var_code, 1, k, level_one);
            set_math_code((k + 32), tex_mathcode, var_code, 1, (k + 32),
                          level_one);
            set_lc_code(k, k + 32, level_one);
            set_lc_code(k + 32, k + 32, level_one);
            set_uc_code(k, k, level_one);
            set_uc_code(k + 32, k, level_one);
            set_sf_code(k, 999, level_one);
        }
        for (k = int_base; k <= attribute_base - 1; k++)
            eqtb[k].cint = 0;
        for (k = attribute_base; k <= del_code_base - 1; k++)
            eqtb[k].cint = UNUSED_ATTRIBUTE;
        mag = 1000;
        tolerance = 10000;
        hang_after = 1;
        max_dead_cycles = 25;
        escape_char = '\\';
        end_line_char = carriage_return;
        set_del_code('.', tex_mathcode, 0, 0, 0, 0, level_one); /* this null delimiter is used in error recovery */
        ex_hyphen_char = '-';
        output_box = 255;
        for (k = dimen_base; k <= eqtb_size; k++)
            eqtb[k].cint = 0;
        page_left_offset = one_inch;
        page_top_offset = one_inch;
        page_right_offset = one_inch;
        page_bottom_offset = one_inch;
        ini_init_primitives();
        hash_used = frozen_control_sequence;    /* nothing is used */
        hash_high = 0;
        cs_count = 0;
        set_eq_type(frozen_dont_expand, dont_expand_cmd);
        cs_text(frozen_dont_expand) = maketexstring("notexpanded:");
        set_eq_type(frozen_primitive, ignore_spaces_cmd);
        set_equiv(frozen_primitive, 1);
        set_eq_level(frozen_primitive, level_one);
        cs_text(frozen_primitive) = maketexstring("primitive");
        create_null_font();
        font_bytes = 0;
        px_dimen = one_bp;
        math_eqno_gap_step = 1000 ;
        cs_text(frozen_protection) = maketexstring("inaccessible");
        format_ident = maketexstring(" (INITEX)");
        cs_text(end_write) = maketexstring("endwrite");
        set_eq_level(end_write, level_one);
        set_eq_type(end_write, outer_call_cmd);
        set_equiv(end_write, null);

    }
    synctexoffset = int_base + synctex_code;

}
