% mathnodes.w
%
% Copyright 2006-2012 Taco Hoekwater <taco@@luatex.org>
% Copyright 2012 Khaled Hosny <khaledhosny@@eglug.org>
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

@ math codes
@c
static sa_tree mathcode_head = NULL;

/* the 0xFFFFFFFF is a flag value */
#define MATHCODESTACK 8
#define MATHCODEDEFAULT 0xFFFFFFFF

@ delcodes
@c
static sa_tree delcode_head = NULL;

#define DELCODESTACK 4
#define DELCODEDEFAULT 0xFFFFFFFF

@ some helpers for mathcode printing

@c
#define print_hex_digit(A) do {                 \
    if ((A)>=10) print_char('A'+(A)-10);        \
    else print_char('0'+(A));                   \
  } while (0)

#define two_hex(A) do {       \
    print_hex_digit((A)/16);  \
    print_hex_digit((A)%16);  \
  } while (0)

#define four_hex(A) do {      \
    two_hex((A)/256);         \
    two_hex((A)%256);         \
  } while (0)

#define six_hex(A) do {       \
    two_hex((A)/65536);       \
    two_hex(((A)%65536)/256); \
    two_hex((A)%256);         \
  } while (0)

@ @c
mathcodeval mathchar_from_integer(int value, int extcode)
{
    mathcodeval mval;
    if (extcode == tex_mathcode) {
        mval.class_value = (value / 0x1000);
        mval.family_value = ((value % 0x1000) / 0x100);
        mval.character_value = (value % 0x100);
    } else { /* some xetexended xetex thing */
        int mfam = (value / 0x200000) & 0x7FF;
        mval.class_value = mfam % 0x08;
        mval.family_value = mfam / 0x08;
        mval.character_value = value & 0x1FFFFF;
    }
    return mval;
}

@ @c
void show_mathcode_value(mathcodeval c)
{
    print_char('"');
    print_hex_digit(c.class_value);
    print_char('"');
    two_hex(c.family_value);
    print_char('"');
    six_hex(c.character_value);
}

@ @c
static void show_mathcode(int n)
{
    mathcodeval c = get_math_code(n);
    tprint_esc("Umathcode");
    print_int(n);
    print_char('=');
    show_mathcode_value(c);
}

@ @c
static void unsavemathcode(quarterword gl)
{
    sa_stack_item st;
    if (mathcode_head->stack == NULL)
        return;
    while (mathcode_head->stack_ptr > 0 && abs(mathcode_head->stack[mathcode_head->stack_ptr].level) >= gl) {
        st = mathcode_head->stack[mathcode_head->stack_ptr];
        if (st.level > 0) {
            rawset_sa_item(mathcode_head, st.code, st.value);
            if (int_par(tracing_restores_code) > 1) {
                begin_diagnostic();
                print_char('{');
                tprint("restoring");
                print_char(' ');
                show_mathcode(st.code);
                print_char('}');
                end_diagnostic(false);
            }
        }
        (mathcode_head->stack_ptr)--;
    }
}

@ @c
void set_math_code(int n, int mathclass, int mathfamily, int mathcharacter, quarterword level)
{
    sa_tree_item v;
    v.math_code_value.class_value = mathclass;
    v.math_code_value.family_value = mathfamily;
    v.math_code_value.character_value = mathcharacter;
    set_sa_item(mathcode_head, n, v, level);
    if (int_par(tracing_assigns_code) > 1) {
        begin_diagnostic();
        print_char('{');
        tprint("assigning");
        print_char(' ');
        show_mathcode(n);
        print_char('}');
        end_diagnostic(false);
    }
}

@ @c
/* we could use two structs ... tex and umath */

mathcodeval get_math_code(int n)
{
    mathcodeval d;
    sa_tree_item v = get_sa_item(mathcode_head, n);
    if (v.uint_value == MATHCODEDEFAULT) {
        d.class_value = 0;
        d.family_value = 0;
        d.character_value = n;
    } else {
        d.class_value = v.math_code_value.class_value;
        d.family_value = v.math_code_value.family_value;
        d.character_value = v.math_code_value.character_value;
    }
    return d;
}

@ @c
int get_math_code_num(int n)
{
    mathcodeval d = get_math_code(n);
    return (d.class_value + (d.family_value * 8)) * (65536 * 32) + d.character_value;
}

@ @c
static void initializemathcode(void)
{
    mathcode_head = new_sa_tree(MATHCODESTACK, 1, (sa_tree_item) MATHCODEDEFAULT);
}

static void dumpmathcode(void)
{
    dump_sa_tree(mathcode_head);
}

static void undumpmathcode(void)
{
    mathcode_head = undump_sa_tree();
}

@ @c
static void show_delcode(int n)
{
    delcodeval c;
    c = get_del_code(n);
    tprint_esc("Udelcode");
    print_int(n);
    print_char('=');
    if (c.small_family_value < 0) {
        print_char('-');
        print_char('1');
    } else {
        print_char('"');
        two_hex(c.small_family_value);
        six_hex(c.small_character_value);
    }
}

@ @c
static void unsavedelcode(quarterword gl)
{
    sa_stack_item st;
    if (delcode_head->stack == NULL)
        return;
    while (delcode_head->stack_ptr > 0 && abs(delcode_head->stack[delcode_head->stack_ptr].level) >= gl) {
        st = delcode_head->stack[delcode_head->stack_ptr];
        if (st.level > 0) {
            rawset_sa_item(delcode_head, st.code, st.value);
            if (int_par(tracing_restores_code) > 1) {
                begin_diagnostic();
                print_char('{');
                tprint("restoring");
                print_char(' ');
                show_delcode(st.code);
                print_char('}');
                end_diagnostic(false);
            }
        }
        (mathcode_head->stack_ptr)--;
    }
}

@ @c
void set_del_code(int n, int smathfamily, int smathcharacter, int lmathfamily, int lmathcharacter, quarterword gl)
{
    sa_tree_item v;
    v.del_code_value.class_value = 0;
    v.del_code_value.small_family_value = smathfamily;
    v.del_code_value.small_character_value = smathcharacter;
    v.del_code_value.dummy_value = 0;
    v.del_code_value.large_family_value = lmathfamily;
    v.del_code_value.large_character_value = lmathcharacter;
    set_sa_item(delcode_head, n, v, gl); /* always global */
    if (int_par(tracing_assigns_code) > 1) {
        begin_diagnostic();
        print_char('{');
        tprint("assigning");
        print_char(' ');
        show_delcode(n);
        print_char('}');
        end_diagnostic(false);
    }
}

@ @c
delcodeval get_del_code(int n)
{
    delcodeval d;
    sa_tree_item v = get_sa_item(delcode_head, n);
    if (v.uint_value == DELCODEDEFAULT) {
        d.class_value = 0;
        d.small_family_value = -1;
        d.small_character_value = 0;
        d.large_family_value = 0;
        d.large_character_value = 0;
    } else {
        d.class_value = v.del_code_value.class_value;
        d.small_family_value = v.del_code_value.small_family_value;
        d.small_character_value = v.del_code_value.small_character_value;
        d.large_family_value = v.del_code_value.large_family_value;
        d.large_character_value = v.del_code_value.large_character_value;
    }
    return d;
}

@ this really only works for old-style delcodes!

@c
int get_del_code_num(int n)
{
    delcodeval d = get_del_code(n);
    if (d.small_family_value < 0) {
        return -1;
    } else {
        return ((d.small_family_value * 256  + d.small_character_value) * 4096 +
                (d.large_family_value * 256) + d.large_character_value);
    }
}

@ @c
static void initializedelcode(void)
{
    delcode_head = new_sa_tree(DELCODESTACK, 2, (sa_tree_item) DELCODEDEFAULT);
}

@ @c
static void dumpdelcode(void)
{
    dump_sa_tree(delcode_head);
}

static void undumpdelcode(void)
{
    delcode_head = undump_sa_tree();
}

@ @c
void unsave_math_codes(quarterword grouplevel)
{
    unsavemathcode(grouplevel);
    unsavedelcode(grouplevel);
}

@ @c
void initialize_math_codes(void)
{
    initializemathcode();
    initializedelcode();
}

@ @c
void free_math_codes(void)
{
    destroy_sa_tree(mathcode_head);
    destroy_sa_tree(delcode_head);
}

@ @c
void dump_math_codes(void)
{
    dumpmathcode();
    dumpdelcode();
}

void undump_math_codes(void)
{
    undumpmathcode();
    undumpdelcode();
}
