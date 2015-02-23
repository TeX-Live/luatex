% pdfpage.w
%
% Copyright 2006-2012 Taco Hoekwater <taco@@luatex.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

@ eternal constants
@c
#define one_bp ((double) 65536 * (double) 72.27 / 72)   /* number of sp per 1bp */

@ @c
void init_pdf_pagecalculations(PDF pdf)
{
    pdfstructure *p;
    int decimal_digits = pdf->decimal_digits;
    assert(pdf != NULL);
    if (pdf->pstruct == NULL)
        pdf->pstruct = xmalloc(sizeof(pdfstructure));
    p = pdf->pstruct;
    setpdffloat(p->pdf.h, 0, decimal_digits);
    setpdffloat(p->pdf.v, 0, decimal_digits);
    p->cw.e = 1;
    p->fs_cur.e = p->fs.e = decimal_digits + 2; /* "+ 2" makes less corrections inside []TJ */
    /* for placement outside BT...ET */
    setpdffloat(p->cm[0], 1, 0);
    setpdffloat(p->cm[1], 0, 0);
    setpdffloat(p->cm[2], 0, 0);
    setpdffloat(p->cm[3], 1, 0);
    setpdffloat(p->cm[4], 0, decimal_digits);   /* horizontal movement on page */
    setpdffloat(p->cm[5], 0, decimal_digits);   /* vertical movement on page */
    /* for placement inside BT...ET */
    setpdffloat(p->tm0_cur, 0, 6);      /* mantissa holds HZ expand * ExtendFont */
    setpdffloat(p->tm[0], ten_pow[6], 6);       /* mantissa holds HZ expand * ExtendFont */
    setpdffloat(p->tm[1], 0, 0);
    setpdffloat(p->tm[2], 0, 3);        /* mantissa holds SlantFont, 0 = default */
    setpdffloat(p->tm[3], ten_pow[6], 6);
    setpdffloat(p->tm[4], 0, decimal_digits);   /* mantissa holds delta from |pdf_bt_pos.h| */
    setpdffloat(p->tm[5], 0, decimal_digits);   /* mantissa holds delta from |pdf_bt_pos.v| */
    /*  */
    p->f_pdf_cur = p->f_pdf = null_font;
    p->fs_cur.m = p->fs.m = 0;
    p->wmode = WMODE_H;
    p->mode = PMODE_PAGE;
    p->ishex = 0;
    p->need_tf = false;
    p->need_tm = false;
    p->k1 = ten_pow[p->pdf.h.e] / one_bp;
}

@ @c
void synch_pos_with_cur(posstructure * pos, posstructure * refpos,
                        scaledpos cur)
{
    switch (pos->dir) {
    case dir_TLT:
        pos->pos.h = refpos->pos.h + cur.h;
        pos->pos.v = refpos->pos.v - cur.v;
        break;
    case dir_TRT:
        pos->pos.h = refpos->pos.h - cur.h;
        pos->pos.v = refpos->pos.v - cur.v;
        break;
    case dir_LTL:
        pos->pos.h = refpos->pos.h + cur.v;
        pos->pos.v = refpos->pos.v - cur.h;
        break;
    case dir_RTT:
        pos->pos.h = refpos->pos.h - cur.v;
        pos->pos.v = refpos->pos.v - cur.h;
        break;
    default:
        assert(0);
    }
}

@ @c
boolean calc_pdfpos(pdfstructure * p, scaledpos pos)
{
    scaledpos new;
    boolean move_pdfpos = false;
    switch (p->mode) {
    case PMODE_PAGE:
        new.h = i32round(pos.h * p->k1);
        new.v = i32round(pos.v * p->k1);
        p->cm[4].m = new.h - p->pdf.h.m;        /* cm is concatenated */
        p->cm[5].m = new.v - p->pdf.v.m;
        if (new.h != p->pdf.h.m || new.v != p->pdf.v.m)
            move_pdfpos = true;
        break;
    case PMODE_TEXT:
        new.h = i32round(pos.h * p->k1);
        new.v = i32round(pos.v * p->k1);
        p->tm[4].m = new.h - p->pdf_bt_pos.h.m; /* Tm replaces */
        p->tm[5].m = new.v - p->pdf_bt_pos.v.m;
        if (new.h != p->pdf.h.m || new.v != p->pdf.v.m)
            move_pdfpos = true;
        break;
    case PMODE_CHAR:
    case PMODE_CHARARRAY:
        switch (p->wmode) {
        case WMODE_H:
            new.h =
                i32round((pos.h * p->k1 - (double) p->pdf_tj_pos.h.m) * p->k2);
            new.v = i32round(pos.v * p->k1);
            p->tj_delta.m = -i64round((double)
                                    ((new.h - p->cw.m) / ten_pow[p->cw.e -
                                                                 p->tj_delta.
                                                                 e]));
            p->tm[5].m = new.v - p->pdf_bt_pos.v.m;     /* p->tm[4] is meaningless */
            if (p->tj_delta.m != 0 || new.v != p->pdf.v.m)
                move_pdfpos = true;
            break;
        case WMODE_V:
            new.h = i32round(pos.h * p->k1);
            new.v =
                i32round(((double) p->pdf_tj_pos.v.m - pos.v * p->k1) * p->k2);
            p->tm[4].m = new.h - p->pdf_bt_pos.h.m;     /* p->tm[5] is meaningless */
            p->tj_delta.m = -i64round((double)
                                    ((new.v - p->cw.m) / ten_pow[p->cw.e -
                                                                 p->tj_delta.
                                                                 e]));
            if (p->tj_delta.m != 0 || new.h != p->pdf.h.m)
                move_pdfpos = true;
            break;
        default:
            assert(0);
        }
        break;
    default:
        assert(0);
    }
    return move_pdfpos;
}

@ @c
void print_pdf_matrix(PDF pdf, pdffloat * tm)
{
    int i;
    for (i = 0; i < 5; i++) {
        print_pdffloat(pdf, tm[i]);
        pdf_out(pdf, ' ');
    }
    print_pdffloat(pdf, tm[i]);
}

@ @c
void pdf_print_cm(PDF pdf, pdffloat * cm)
{
    print_pdf_matrix(pdf, cm);
    pdf_puts(pdf, " cm\n");
}

@ @c
void pdf_set_pos(PDF pdf, scaledpos pos)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    assert(is_pagemode(p));
    move = calc_pdfpos(p, pos);
    if (move) {
        pdf_print_cm(pdf, p->cm);
        p->pdf.h.m += p->cm[4].m;
        p->pdf.v.m += p->cm[5].m;
    }
}

@ @c
void pdf_set_pos_temp(PDF pdf, scaledpos pos)
{
    boolean move;
    pdfstructure *p = pdf->pstruct;
    assert(is_pagemode(p));
    move = calc_pdfpos(p, pos);
    if (move)
        pdf_print_cm(pdf, p->cm);
}

@ @c
static void begin_text(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_pagemode(p));
    p->pdf_bt_pos = p->pdf;
    pdf_puts(pdf, "BT\n");
    p->mode = PMODE_TEXT;
    p->need_tf = true;
}

static void end_text(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(is_textmode(p));
    pdf_puts(pdf, "ET\n");
    p->pdf = p->pdf_bt_pos;
    p->mode = PMODE_PAGE;
}

void pdf_end_string_nl(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    if (is_charmode(p))
        end_charmode(pdf);
    if (is_chararraymode(p))
        end_chararray(pdf);
}

@ @c
void pdf_goto_pagemode(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    assert(p != NULL);
    if (!is_pagemode(p)) {
        if (is_charmode(p))
            end_charmode(pdf);
        if (is_chararraymode(p))
            end_chararray(pdf);
        if (is_textmode(p))
            end_text(pdf);
    }
    assert(is_pagemode(p));
}

void pdf_goto_textmode(PDF pdf)
{
    pdfstructure *p = pdf->pstruct;
    const scaledpos origin = {
        0, 0
    };
    if (!is_textmode(p)) {
        if (is_pagemode(p)) {
            pdf_set_pos(pdf, origin);   /* reset to page origin */
            begin_text(pdf);
        } else {
            if (is_charmode(p))
                end_charmode(pdf);
            if (is_chararraymode(p))
                end_chararray(pdf);
        }
    }
    assert(is_textmode(p));
}
