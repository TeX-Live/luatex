% pdffont.w
%
% Copyright 2009-2014 Taco Hoekwater <taco@@luatex.org>
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

\def\pdfTeX{pdf\TeX}

@ @c
static const char _svn_version[] =
    "$Id$"
    "$URL$";

#include "ptexlib.h"

@ @c
#define font_id_text(A) cs_text(font_id_base+(A))       /* a frozen font identifier's name */

int pk_dpi;                     /* PK pixel density value from \.{texmf.cnf} */


@ As \pdfTeX{} should also act as a back-end driver, it needs to support virtual
fonts too. Information about virtual fonts can be found in the source of some
\.{DVI}-related programs.

Whenever we want to write out a character in a font to PDF output, we
should check whether the used character is a virtual or real character.
The |has_packet()| C macro checks for this condition.


@ The following code typesets a character to PDF output

@c
scaled_whd output_one_char(PDF pdf, halfword p)
{
    scaled_whd ci;              /* the real width, height and depth of the character */
    internal_font_number f = font(p);
    int c = character(p);
    int ex_glyph = ex_glyph(p)/1000;
    ci = get_charinfo_whd(f, c);
    if (!(char_exists(f,c))) {
        char_warning(f,c);
        return ci;
    }
    //ci.wd = round_xn_over_d(ci.wd, 1000 + ex_glyph, 1000);
    ci.wd = ext_xn_over_d(ci.wd, 1000000 + ex_glyph(p), 1000000);
    switch (pdf->posstruct->dir) {
    case dir_TLT:
        break;
    case dir_TRT:
        pos_left(ci.wd);
        break;
    case dir_LTL:
        pos_down(ci.ht);
        pos_left(ci.wd);
        break;
    case dir_RTT:
        pos_down(ci.ht);
        pos_left(ci.wd / 2);
        break;
    default:
        assert(0);
    }
    if (has_packet(f, c)) {
        do_vf_packet(pdf, f, c, ex_glyph);
    } else
        backend_out[glyph_node] (pdf, f, c, ex_glyph);  /* |pdf_place_glyph(pdf, f, c, ex_glyph);| */
    return ci;
}

@ Mark |f| as a used font; set |font_used(f)|, |font_size(f)| and |pdf_font_num(f)|
@c
static void pdf_use_font(internal_font_number f, int fontnum)
{
    set_font_used(f, true);
    assert((fontnum > 0) || ((fontnum < 0) && (pdf_font_num(-fontnum) > 0)));
    set_pdf_font_num(f, fontnum);
}

@ To set PDF font we need to find out fonts with the same name, because \TeX\
can load the same font several times for various sizes. For such fonts we
define only one font resource. The array |pdf_font_num| holds the object
number of font resource. A negative value of an entry of |pdf_font_num|
indicates that the corresponding font shares the font resource with the font

@c
#define same(n,f,k) (n(f) != NULL && n(k) != NULL && strcmp(n(f), n(k)) == 0)

static boolean font_shareable(internal_font_number f, internal_font_number k)
{
    int ret = 0;
    /* For some lua-loaded (for instance AFM) fonts, it is normal to have
       a zero cidregistry,  and such fonts do not have a fontmap entry yet
       at this point, so the test should use the other branch  */
    if (font_cidregistry(f) == NULL && font_cidregistry(k) == NULL &&
        font_encodingbytes(f) != 2 && font_encodingbytes(k) != 2) {
        if (font_map(k) != NULL &&
            font_map(f) != NULL && (same(font_name, k, f))) {
            ret = 1;
        }
    } else {
        if ((same(font_filename, k, f) && same(font_fullname, k, f))) {
            ret = 1;
        }
#ifdef DEBUG
        printf("\nfont_shareable(%d:%s:%s,%d:%s:%s): => %d\n",
               f, font_filename(f), font_fullname(f),
               k, font_filename(k), font_fullname(k), ret);
#endif
    }
    return ret;
}

@ create a font object
@c
void pdf_init_font(PDF pdf, internal_font_number f)
{
    internal_font_number k/*, b*/;
    fm_entry *fm;
    int i, l;
    assert(!font_used(f));

    ///* if |f| is auto expanded then ensure the base font is initialized */
    //
    //    if (font_auto_expand(f) && ((b = pdf_font_blink(f)) != null_font)) {
    //    if (!font_used(b))
    //        pdf_init_font(pdf, b);
    //    set_font_map(f, font_map(b));
    //    /* propagate slant and extend from unexpanded base font */
    //    set_font_slant(f, font_slant(b));
    //    set_font_extend(f, font_extend(b));
    //}
    /* check whether |f| can share the font object with some |k|: we have 2 cases
       here: 1) |f| and |k| have the same tfm name (so they have been loaded at
       different sizes, eg 'cmr10' and 'cmr10 at 11pt'); 2) |f| has been auto
       expanded from |k|
     */

    /* take over slant and extend from map entry, if not already set;
       this should also be the only place where getfontmap() may be called. */

    fm = getfontmap(font_name(f));
    if (font_map(f) == NULL && fm != NULL) {
        font_map(f) = fm;
        if (is_slantset(fm))
            font_slant(f) = fm->slant;
        if (is_extendset(fm))
            font_extend(f) = fm->extend;
    }
    i = pdf->head_tab[obj_type_font];
    while (i != 0) {
        k = obj_info(pdf, i);
        if (font_shareable(f, k)) {
            assert(pdf_font_num(k) != 0);
            if (pdf_font_num(k) < 0)
                pdf_use_font(f, pdf_font_num(k));
            else
                pdf_use_font(f, -k);
            return;
        }
        i = obj_link(pdf, i);
    }
    /* create a new font object for |f| */
    l = pdf_create_obj(pdf, obj_type_font, f);
    pdf_use_font(f, l);
}

@ set the actual font on PDF page
@c
internal_font_number pdf_set_font(PDF pdf, internal_font_number f)
{
    int ff;                     /* for use with |set_ff| */
    if (!font_used(f))
        pdf_init_font(pdf, f);
    /*
       set |ff| to the tfm number of the base font sharing the font object with |f|;
       |ff| is either |f| itself (then it is its own base font),
       or some font with the same tfm name at different size and/or expansion.
     */
    ff = pdf_font_num(f) < 0 ? -pdf_font_num(f) : f;    /* aka |set_ff(f)| */
    assert(pdf_font_num(ff) > 0);       /* base font object number */
    addto_page_resources(pdf, obj_type_font, pdf_font_num(ff));
    return ff;
}

@ Here come some subroutines to deal with expanded fonts for HZ-algorithm.
return 1 == identical
@c
static boolean cmp_font_name(int id, char *tt)
{
    char *tid;
    if (!is_valid_font(id))
        return 0;
    tid = font_name(id);
    if (tt == NULL && tid == NULL)
        return 1;
    if (tt == NULL || tid == NULL || strcmp(tid, tt) != 0)
        return 0;
    return 1;
}

@ @c
internal_font_number tfm_lookup(char *s, scaled fs)
{                               /* looks up for a TFM with name |s| loaded at |fs| size; if found then flushes |s| */
    internal_font_number k;
    if (fs != 0) {
        for (k = 1; k <= max_font_id(); k++) {
            if (cmp_font_name(k, s) && font_size(k) == fs) {
                return k;
            }
        }
    } else {
        for (k = 1; k <= max_font_id(); k++) {
            if (cmp_font_name(k, s)) {
                return k;
            }
        }
    }
    return null_font;
}

@ @c
int fix_expand_value(internal_font_number f, int e)
{                               /* return the multiple of |font_step(f)| that is nearest to |e| */
    int step;
    int max_expand;
    boolean neg;
    if (e == 0)
        return 0;
    if (e < 0) {
        e = -e;
        neg = true;
        max_expand = font_max_shrink(f);
    } else {
        neg = false;
        max_expand = font_max_stretch(f);
    }
    if (e > max_expand) {
        e = max_expand;
    } else {
        step = font_step(f);
        if (e % step > 0)
            e = step * round_xn_over_d(e, 1, step);
    }
    if (neg)
        e = -e;
    return e;
}

@ @c
void set_expand_params(internal_font_number f, boolean auto_expand,
                       int stretch_limit, int shrink_limit, int font_step)
{                               /* expand a font with given parameters */
    set_font_step(f, font_step);
    set_font_auto_expand(f, auto_expand);
    set_font_max_shrink(f, shrink_limit);
    set_font_max_stretch(f, stretch_limit);
}

@ @c
void read_expand_font(void)
{                               /* read font expansion spec and load expanded font */
    int shrink_limit, stretch_limit, font_step;
    internal_font_number f;
    boolean auto_expand;
    /* read font expansion parameters */
    scan_font_ident();
    f = cur_val;
    if (f == null_font)
        pdf_error("font expansion", "invalid font identifier");
    //if (pdf_font_blink(f) != null_font)
    //    pdf_error("font expansion",
    //              "\\pdffontexpand cannot be used this way (the base font has been expanded)");
    scan_optional_equals();
    scan_int();
    stretch_limit = fix_int(cur_val, 0, 1000);
    scan_int();
    shrink_limit = fix_int(cur_val, 0, 500);
    scan_int();
    font_step = fix_int(cur_val, 0, 100);
    if (font_step == 0)
        pdf_error("font expansion", "invalid step");
    stretch_limit = stretch_limit - stretch_limit % font_step;
    if (stretch_limit < 0)
        stretch_limit = 0;
    shrink_limit = shrink_limit - shrink_limit % font_step;
    if (shrink_limit < 0)
        shrink_limit = 0;
    if ((stretch_limit == 0) && (shrink_limit == 0))
        pdf_error("font expansion", "invalid limit(s)");
    auto_expand = false;
    if (scan_keyword("autoexpand")) {
        auto_expand = true;
        /* Scan an optional space */
        get_x_token();
        if (cur_cmd != spacer_cmd)
            back_input();
    }

    if (font_step(f) != 0) {
        /* this font has been expanded, ensure the expansion parameters are identical */
        if (font_step(f) != font_step)
            pdf_error("font expansion",
                      "font has been expanded with different expansion step");

        if (((font_max_stretch(f) == 0) && (stretch_limit != 0)) ||
            ((font_max_stretch(f) > 0)
             && (font_max_stretch(f) != stretch_limit)))
            pdf_error("font expansion",
                      "font has been expanded with different stretch limit");

        if (((font_max_shrink(f) == 0) && (shrink_limit != 0)) ||
            ((font_max_shrink(f) > 0)
             && (font_max_shrink(f) != shrink_limit)))
            pdf_error("font expansion",
                      "font has been expanded with different shrink limit");

        if (font_auto_expand(f) != auto_expand)
            pdf_error("font expansion",
                      "font has been expanded with different auto expansion value");
    } else {
        if (font_used(f))
            pdf_warning("font expansion",
                        "font should be expanded before its first use", true,
                        true);
        set_expand_params(f, auto_expand, stretch_limit, shrink_limit,
                          font_step);
    }
}

@ @c
void new_letterspaced_font(small_number a)
{                               /* letter-space a font by creating a virtual font */
    pointer u;                  /* user's font identifier */
    str_number t;               /* name for the frozen font identifier */
    internal_font_number f, k;
    boolean nolig = false;
    get_r_token();
    u = cur_cs;
    if (u >= hash_base)
        t = cs_text(u);
    else
        t = maketexstring("FONT");
    define(u, set_font_cmd, null_font);
    scan_optional_equals();
    scan_font_ident();
    k = cur_val;
    scan_int();
    if (scan_keyword("nolig"))
       nolig=true;
    f = letter_space_font(k, fix_int(cur_val, -1000, 1000), nolig);
    equiv(u) = f;
    eqtb[font_id_base + f] = eqtb[u];
    font_id_text(f) = t;
}

@ @c
void make_font_copy(small_number a)
{                               /* make a font copy for further use with font expansion */
    pointer u;                  /* user's font identifier */
    str_number t;               /* name for the frozen font identifier */
    internal_font_number f, k;
    get_r_token();
    u = cur_cs;
    if (u >= hash_base)
        t = cs_text(u);
    else
        t = maketexstring("FONT");
    define(u, set_font_cmd, null_font);
    scan_optional_equals();
    scan_font_ident();
    k = cur_val;
    f = copy_font_info(k);
    equiv(u) = f;
    eqtb[font_id_base + f] = eqtb[u];
    font_id_text(f) = t;
}

@ @c
void pdf_include_chars(PDF pdf)
{
    str_number s;
    unsigned char *k, *j;       /* running index */
    internal_font_number f;
    scan_font_ident();
    f = cur_val;
    if (f == null_font)
        pdf_error("font", "invalid font identifier");
    pdf_check_vf(cur_val);
    if (!font_used(f))
        pdf_init_font(pdf, f);
    scan_pdf_ext_toks();
    s = tokens_to_string(def_ref);
    delete_token_ref(def_ref);
    j = str_string(s) + str_length(s);
    for (k = str_string(s); k < j; k++) {
        pdf_mark_char(f, *k);
    }
    flush_str(s);
}

@ @c
void glyph_to_unicode(void)
{
    str_number s1, s2;
    scan_pdf_ext_toks();
    s1 = tokens_to_string(def_ref);
    delete_token_ref(def_ref);
    scan_pdf_ext_toks();
    s2 = tokens_to_string(def_ref);
    delete_token_ref(def_ref);
    def_tounicode(s1, s2);
    flush_str(s2);
    flush_str(s1);
}
