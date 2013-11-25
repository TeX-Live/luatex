% pdfglyph.w
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
    "$Id: pdfglyph.w 4583 2013-02-21 20:18:08Z hhenkel $"
    "$URL: https://foundry.supelec.fr/svn/luatex/branches/ex-glyph/source/texk/web2c/luatexdir/pdf/pdfglyph.w $";

#include "ptexlib.h"
#include "pdf/pdfpage.h"

#define pdf2double(a) ((double) (a).m / ten_pow[(a).e])

@ eternal constants
@c
#define one_bp ((double) 65536 * (double) 72.27 / 72)   /* number of sp per 1bp */
#define e_tj 3                  /* must be 3; movements in []TJ are in fontsize/$10^3$ units */

@ @c
static long pdf_char_width(pdfstructure * p, internal_font_number f, int i)
{
    /* use exactly this formula also for calculating the /Width array values */
    return
        lround((double) char_width(f, i) / font_size(f) *
               ten_pow[e_tj + p->cw.e]);
}

@ @c
void pdf_print_charwidth(PDF pdf, internal_font_number f, int i)
{
    pdffloat cw;
    pdfstructure *p = pdf->pstruct;
    cw.m = pdf_char_width(p, f, i);
    cw.e = p->cw.e;
    print_pdffloat(pdf, cw);
}

@ @c
static void setup_fontparameters(PDF pdf, internal_font_number f, int ex_glyph)
{
    float slant, extend, expand, scale = 1.0;
    float u = 1.0;
    pdfstructure *p = pdf->pstruct;
    /* fix mantis bug \# 0000200 (acroread "feature") */
    if ((font_format(f) == opentype_format ||
         (font_format(f) == type1_format && font_encodingbytes(f) == 2))
        && font_units_per_em(f) > 0)
        u = font_units_per_em(f) / 1000.0;
    pdf->f_cur = f;
    p->f_pdf = pdf_set_font(pdf, f);
    p->fs.m = lround(font_size(f) / u / one_bp * ten_pow[p->fs.e]);
    slant = font_slant(f) / 1000.0;
    extend = font_extend(f) / 1000.0;
    expand = 1.0 + (ex_glyph/1) / 1000.0;
    p->tj_delta.e = p->cw.e - 1;        /* "- 1" makes less corrections inside []TJ */
    /* no need to be more precise than TeX (1sp) */
    while (p->tj_delta.e > 0
           && (double) font_size(f) / ten_pow[p->tj_delta.e + e_tj] < 0.5)
        p->tj_delta.e--;        /* happens for very tiny fonts */
    assert(p->cw.e >= p->tj_delta.e);   /* else we would need, e. g., |ten_pow[-1]| */
    p->tm[0].m = lround(scale * expand * extend * ten_pow[p->tm[0].e]);
    p->tm[2].m = lround(slant * ten_pow[p->tm[2].e]);
    p->tm[3].m = lround(scale * ten_pow[p->tm[3].e]);
    p->k2 =
        ten_pow[e_tj +
                p->cw.e] * scale / (ten_pow[p->pdf.h.e] * pdf2double(p->fs) *
                                    pdf2double(p->tm[0]));
}

@ @c
static void set_font(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    pdf_printf(pdf, "/F%d", (int) p->f_pdf);
    pdf_print_resname_prefix(pdf);
    pdf_out(pdf, ' ');
    print_pdffloat(pdf, p->fs);
    pdf_puts(pdf, " Tf ");
    p->f_pdf_cur = p->f_pdf;
    p->fs_cur.m = p->fs.m;
    p->need_tf = false;
    p->need_tm = true;          /* always follow Tf by Tm */
}

@ @c
static void set_textmatrix(PDF pdf, scaledpos pos)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    assert(is_textmode(p));
    move = calc_pdfpos(p, pos);
    if (p->need_tm || move) {
        print_pdf_matrix(pdf, p->tm);
        pdf_puts(pdf, " Tm ");
        p->pdf.h.m = p->pdf_bt_pos.h.m + p->tm[4].m;    /* Tm replaces */
        p->pdf.v.m = p->pdf_bt_pos.v.m + p->tm[5].m;
        p->need_tm = false;
    }
    p->tm0_cur.m = p->tm[0].m;
}

@ Print out a character to PDF buffer; the character will be printed in octal
form in the following cases: chars <= 32, backslash (92), left parenthesis
(40), and right parenthesis (41).
@c
static void pdf_print_char(PDF pdf, int c)
{
    if (c > 255)
        return;
    /* pdf_print_escaped(c) */
    if (c <= 32 || c == '\\' || c == '(' || c == ')' || c > 127) {
        pdf_room(pdf, 4);
        pdf_quick_out(pdf, '\\');
        pdf_quick_out(pdf, (unsigned char) ('0' + ((c >> 6) & 0x3)));
        pdf_quick_out(pdf, (unsigned char) ('0' + ((c >> 3) & 0x7)));
        pdf_quick_out(pdf, (unsigned char) ('0' + (c & 0x7)));
    } else
        pdf_out(pdf, c);
}

static void pdf_print_wide_char(PDF pdf, int c)
{
    char hex[5];
    snprintf(hex, 5, "%04X", c);
    pdf_out_block(pdf, (const char *) hex, 4);
}

@ @c
static void begin_charmode(PDF pdf, internal_font_number f, pdfstructure * p)
{
    assert(is_chararraymode(p));
    if (font_encodingbytes(f) == 2) {
        p->ishex = 1;
        pdf_out(pdf, '<');
    } else {
        p->ishex = 0;
        pdf_out(pdf, '(');
    }
    p->mode = PMODE_CHAR;
}

@ @c
void end_charmode(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_charmode(p));
    if (p->ishex == 1) {
        p->ishex = 0;
        pdf_out(pdf, '>');
    } else {
        pdf_out(pdf, ')');
    }
    p->mode = PMODE_CHARARRAY;
}

@ @c
static void begin_chararray(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_textmode(p));
    p->pdf_tj_pos = p->pdf;
    p->cw.m = 0;
    pdf_out(pdf, '[');
    p->mode = PMODE_CHARARRAY;
}

@ @c
void end_chararray(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_chararraymode(p));
    pdf_puts(pdf, "]TJ\n");
    p->pdf = p->pdf_tj_pos;
    p->mode = PMODE_TEXT;
}

@ @c
void pdf_place_glyph(PDF pdf, internal_font_number f, int c, int ex)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    scaledpos pos = pdf->posstruct->pos;
    if (!char_exists(f, c))
        return;
    /* ensure to be within BT...ET */
    if (is_pagemode(p)) {
        pdf_goto_textmode(pdf);
        p->need_tf = true;
    }
    /* all font setup */
    if (true || f != pdf->f_cur || p->need_tf) {
        setup_fontparameters(pdf, f, ex);
        if (p->need_tf || p->f_pdf != p->f_pdf_cur || p->fs.m != p->fs_cur.m) {
            pdf_goto_textmode(pdf);
            set_font(pdf);
        } else if (p->tm0_cur.m != p->tm[0].m) {
            /* catch in-line HZ expand change due to efcode */
            p->need_tm = true;
        }
    }
    /* all movements */
    move = calc_pdfpos(p, pos); /* within text or chararray or char mode */
    if (move || p->need_tm) {
        if (p->need_tm || (p->wmode == WMODE_H
                           && (p->pdf_bt_pos.v.m + p->tm[5].m) != p->pdf.v.m)
            || (p->wmode == WMODE_V
                && (p->pdf_bt_pos.h.m + p->tm[4].m) != p->pdf.h.m)
            || abs(p->tj_delta.m) >= 1000000) {
            pdf_goto_textmode(pdf);
            set_textmatrix(pdf, pos);
            begin_chararray(pdf);
            move = calc_pdfpos(p, pos); /* for fine adjustment */
        }
        if (move) {
            assert((p->wmode == WMODE_H
                    && (p->pdf_bt_pos.v.m + p->tm[5].m) == p->pdf.v.m)
                   || (p->wmode == WMODE_V
                       && (p->pdf_bt_pos.h.m + p->tm[4].m) == p->pdf.h.m));
            if (is_charmode(p))
                end_charmode(pdf);
            assert(p->tj_delta.m != 0);
            print_pdffloat(pdf, p->tj_delta);
            p->cw.m -= p->tj_delta.m * ten_pow[p->cw.e - p->tj_delta.e];
        }
    }
    /* glyph output */
    assert(is_chararraymode(p) || is_charmode(p));
    if (is_chararraymode(p))
        begin_charmode(pdf, f, p);
    pdf_mark_char(f, c);
    if (font_encodingbytes(f) == 2)
        pdf_print_wide_char(pdf, char_index(f, c));
    else
        pdf_print_char(pdf, c);
    p->cw.m += pdf_char_width(p, p->f_pdf, c);  /* aka |adv_char_width()| */
}
