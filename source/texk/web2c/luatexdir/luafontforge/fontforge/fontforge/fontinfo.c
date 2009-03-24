/* Copyright (C) 2000-2007 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "pfaeditui.h"
#include "ofl.h"
#include <ustring.h>
#include <chardata.h>
#include <utype.h>
#include "unicoderange.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
extern int _GScrollBar_Width;
#endif
#include <gkeysym.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#ifdef LUA_FF_LIB
#define Isspace(a) (a==' ')
#else
#define Isspace isspace
#endif

#ifndef LUA_FF_LIB
static int last_aspect=0;
#endif

GTextInfo emsizes[] = {
    { (unichar_t *) "1000", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "1024", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "2048", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "4096", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

GTextInfo interpretations[] = {
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "None", ignore "Interpretation|" */
/* GT: In french this could be "Aucun" or "Aucune" depending on the gender */
/* GT:  of "Interpretation" */
    { (unichar_t *) N_("Interpretation|None"), NULL, 0, 0, (void *) ui_none, NULL, 0, 0, 0, 0, 0, 0, 1},
/*  { (unichar_t *) N_("Adobe Public Use Defs."), NULL, 0, 0, (void *) ui_adobe, NULL, 0, 0, 0, 0, 0, 0, 1}, */
/*  { (unichar_t *) N_("Greek"), NULL, 0, 0, (void *) ui_greek, NULL, 0, 0, 0, 0, 0, 0, 1}, */
    { (unichar_t *) N_("Japanese"), NULL, 0, 0, (void *) ui_japanese, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Traditional Chinese"), NULL, 0, 0, (void *) ui_trad_chinese, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Simplified Chinese"), NULL, 0, 0, (void *) ui_simp_chinese, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Korean"), NULL, 0, 0, (void *) ui_korean, NULL, 0, 0, 0, 0, 0, 0, 1},
/*  { (unichar_t *) N_("AMS Public Use"), NULL, 0, 0, (void *) ui_ams, NULL, 0, 0, 0, 0, 0, 0, 1}, */
    { NULL }};
GTextInfo macstyles[] = {
    { (unichar_t *) N_("MacStyles|Bold"), NULL, 0, 0, (void *) sf_bold, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MacStyles|Italic"), NULL, 0, 0, (void *) sf_italic, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MacStyles|Condense"), NULL, 0, 0, (void *) sf_condense, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MacStyles|Expand"), NULL, 0, 0, (void *) sf_extend, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MacStyles|Underline"), NULL, 0, 0, (void *) sf_underline, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MacStyles|Outline"), NULL, 0, 0, (void *) sf_outline, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MacStyles|Shadow"), NULL, 0, 0, (void *) sf_shadow, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo widthclass[] = {
    { (unichar_t *) N_("Ultra-Condensed (50%)"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Extra-Condensed (62.5%)"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Condensed (75%)"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Semi-Condensed (87.5%)"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Medium (100%)"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Semi-Expanded (112.5%)"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Expanded (125%)"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Extra-Expanded (150%)"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ultra-Expanded (200%)"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo weightclass[] = {
    { (unichar_t *) N_("100 Thin"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("200 Extra-Light"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("300 Light"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("400 Book"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("500 Medium"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("600 Demi-Bold"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("700 Bold"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("800 Heavy"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("900 Black"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo fstype[] = {
    { (unichar_t *) N_("Never Embed/No Editing"), NULL, 0, 0, (void *) 0x02, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Printable Document"), NULL, 0, 0, (void *) 0x04, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Editable Document"), NULL, 0, 0, (void *) 0x08, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Installable Font"), NULL, 0, 0, (void *) 0x00, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo pfmfamily[] = {
    { (unichar_t *) N_("Serif"), NULL, 0, 0, (void *) 0x11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif"), NULL, 0, 0, (void *) 0x21, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Monospace"), NULL, 0, 0, (void *) 0x31, NULL, 0, 0, 0, 0, 0, 0, 1},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Script", ignore "cursive|" */
/* GT: English uses "script" to me a general writing style (latin, greek, kanji) */
/* GT: and the cursive handwriting style. Here we mean cursive handwriting. */
    { (unichar_t *) N_("cursive|Script"), NULL, 0, 0, (void *) 0x41, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Decorative"), NULL, 0, 0, (void *) 0x51, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo ibmfamily[] = {
    { (unichar_t *) N_("No Classification"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Old Style Serifs"), NULL, 0, 0, (void *) 0x100, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Rounded Legibility"), NULL, 0, 0, (void *) 0x101, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Geralde"), NULL, 0, 0, (void *) 0x102, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Venetian"), NULL, 0, 0, (void *) 0x103, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Modified Venetian"), NULL, 0, 0, (void *) 0x104, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Dutch Modern"), NULL, 0, 0, (void *) 0x105, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Dutch Trad"), NULL, 0, 0, (void *) 0x106, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Contemporary"), NULL, 0, 0, (void *) 0x107, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Calligraphic"), NULL, 0, 0, (void *) 0x108, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Miscellaneous"), NULL, 0, 0, (void *) 0x10f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Transitional Serifs"), NULL, 0, 0, (void *) 0x200, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("TS Direct Line"), NULL, 0, 0, (void *) 0x201, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("TS Script"), NULL, 0, 0, (void *) 0x202, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("TS Miscellaneous"), NULL, 0, 0, (void *) 0x20f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Modern Serifs"), NULL, 0, 0, (void *) 0x300, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MS Italian"), NULL, 0, 0, (void *) 0x301, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MS Script"), NULL, 0, 0, (void *) 0x302, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MS Miscellaneous"), NULL, 0, 0, (void *) 0x30f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Clarendon Serifs"), NULL, 0, 0, (void *) 0x400, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Clarendon"), NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Modern"), NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Traditional"), NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Newspaper"), NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Stub Serif"), NULL, 0, 0, (void *) 0x405, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Monotone"), NULL, 0, 0, (void *) 0x406, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Typewriter"), NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Miscellaneous"), NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slab Serifs"), NULL, 0, 0, (void *) 0x500, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slab Serifs|SS Monotone"), NULL, 0, 0, (void *) 0x501, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slab Serifs|SS Humanist"), NULL, 0, 0, (void *) 0x502, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slab Serifs|SS Geometric"), NULL, 0, 0, (void *) 0x503, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slab Serifs|SS Swiss"), NULL, 0, 0, (void *) 0x504, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slab Serifs|SS Typewriter"), NULL, 0, 0, (void *) 0x505, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slab Serifs|SS Miscellaneous"), NULL, 0, 0, (void *) 0x50f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Freeform Serifs"), NULL, 0, 0, (void *) 0x700, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("FS Modern"), NULL, 0, 0, (void *) 0x701, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("FS Miscellaneous"), NULL, 0, 0, (void *) 0x70f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif"), NULL, 0, 0, (void *) 0x800, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS IBM NeoGrotesque Gothic"), NULL, 0, 0, (void *) 0x801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS Humanist"), NULL, 0, 0, (void *) 0x802, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS Low-x Round Geometric"), NULL, 0, 0, (void *) 0x803, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS High-x Round Geometric"), NULL, 0, 0, (void *) 0x804, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS NeoGrotesque Gothic"), NULL, 0, 0, (void *) 0x805, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS Modified Grotesque Gothic"), NULL, 0, 0, (void *) 0x806, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS Typewriter Gothic"), NULL, 0, 0, (void *) 0x809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS Matrix"), NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif|SS Miscellaneous"), NULL, 0, 0, (void *) 0x80f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ornamentals"), NULL, 0, 0, (void *) 0x900, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Engraver"), NULL, 0, 0, (void *) 0x901, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Black Letter"), NULL, 0, 0, (void *) 0x902, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Decorative"), NULL, 0, 0, (void *) 0x903, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Three Dimensional"), NULL, 0, 0, (void *) 0x904, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Miscellaneous"), NULL, 0, 0, (void *) 0x90f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Scripts"), NULL, 0, 0, (void *) 0xa00, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Uncial"), NULL, 0, 0, (void *) 0xa01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Brush Joined"), NULL, 0, 0, (void *) 0xa02, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Formal Joined"), NULL, 0, 0, (void *) 0xa03, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Monotone Joined"), NULL, 0, 0, (void *) 0xa04, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Calligraphic"), NULL, 0, 0, (void *) 0xa05, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Brush Unjoined"), NULL, 0, 0, (void *) 0xa06, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Formal Unjoined"), NULL, 0, 0, (void *) 0xa07, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Monotone Unjoined"), NULL, 0, 0, (void *) 0xa08, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Miscellaneous"), NULL, 0, 0, (void *) 0xa0f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Symbolic"), NULL, 0, 0, (void *) 0xc00, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sy Mixed Serif"), NULL, 0, 0, (void *) 0xc03, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sy Old Style Serif"), NULL, 0, 0, (void *) 0xc06, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sy Neo-grotesque Sans Serif"), NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sy Miscellaneous"), NULL, 0, 0, (void *) 0xc0f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo os2versions[] = {
    { (unichar_t *) N_("OS2Version|Automatic"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("1"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("2"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("3"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("4"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
#ifndef LUA_FF_LIB
static GTextInfo gaspversions[] = {
    { (unichar_t *) N_("0"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("1"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
#endif
static GTextInfo panfamily[] = {
    { (unichar_t *) N_("PanoseFamily|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseFamily|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Text & Display"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Script", ignore "cursive|" */
/* GT: English uses "script" to me a general writing style (latin, greek, kanji) */
/* GT: and the cursive handwriting style. Here we mean cursive handwriting. */
    { (unichar_t *) N_("cursive|Script"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Decorative"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Pictorial"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "6", NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panserifs[] = {
    { (unichar_t *) N_("PanoseSerifs|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseSerifs|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Cove"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Obtuse Cove"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Square Cove"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Obtuse Square Cove"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseSerivfs|Square"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseSerifs|Thin"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Bone"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Exaggerated"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Triangle"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal Sans"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Obtuse Sans"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Perp Sans"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Flared"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseSerivfs|Rounded"), NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panweight[] = {
    { (unichar_t *) N_("PanoseWeight|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseWeight|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Very Light"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Light"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseWeight|Thin"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Book"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Medium"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Demi"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Bold"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Heavy"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Black"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Nord"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panprop[] = {
    { (unichar_t *) N_("PanoseProportion|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseProportion|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Old Style"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Modern"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Even Width"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Expanded"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Condensed"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Very Expanded"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Very Condensed"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Monospaced"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo pancontrast[] = {
    { (unichar_t *) N_("PanoseContrast|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|None"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|Very Low"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|Low"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|Medium Low"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|Medium"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|Medium High"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|High"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseContrast|Very High"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panstrokevar[] = {
    { (unichar_t *) N_("PanoseStrokeVariation|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseStrokeVariation|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gradual/Diagonal"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gradual/Transitional"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gradual/Vertical"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gradual/Horizontal"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Rapid/Vertical"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Rapid/Horizontal"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Instant/Vertical"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panarmstyle[] = {
    { (unichar_t *) N_("PanoseArmStyle|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseArmStyle|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Horizontal"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Wedge"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Vertical"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Single Serif"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Double Serif"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Horizontal"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Wedge"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Vertical"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Single Serif"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Double Serif"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panletterform[] = {
    { (unichar_t *) N_("PanoseLetterform|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseLetterform|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Contact"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Weighted"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Boxed"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Flattened"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Rounded"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Off-Center"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Square"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Contact"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Weighted"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Boxed"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Rounded"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Off-Center"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Square"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panmidline[] = {
    { (unichar_t *) N_("PanoseMidline|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Standard/Trimmed"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Standard/Pointed"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Standard/Serifed"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|High/Trimmed"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|High/Pointed"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|High/Serifed"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Constant/Trimmed"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Constant/Pointed"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Constant/Serifed"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Low/Trimmed"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Low/Pointed"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseMidline|Low/Serifed"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panxheight[] = {
    { (unichar_t *) N_("PanoseXHeight|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseXHeight|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseXHeight|Constant/Small"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseXHeight|Constant/Standard"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseXHeight|Constant/Large"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseXHeight|Ducking/Small"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseXHeight|Ducking/Standard"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("PanoseXHeight|Ducking/Large"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

#ifndef LUA_FF_LIB
static struct ms_2_locales { char *loc_name; int local_id; } ms_2_locals[] = {
    { "af", 0x436 },
    { "sq_AL", 0x41c },
    { "am", 0x45e },
    { "ar_SA", 0x401 },
    { "ar_IQ", 0x801 },
    { "ar_EG", 0xc01 },
    { "ar_LY", 0x1001 },
    { "ar_DZ", 0x1401 },
    { "ar_MA", 0x1801 },
    { "ar_TN", 0x1C01 },
    { "ar_OM", 0x2001 },
    { "ar_YE", 0x2401 },
    { "ar_SY", 0x2801 },
    { "ar_JO", 0x2c01 },
    { "ar_LB", 0x3001 },
    { "ar_KW", 0x3401 },
    { "ar_AE", 0x3801 },
    { "ar_BH", 0x3c01 },
    { "ar_QA", 0x4001 },
    { "hy", 0x42b },
    { "as", 0x44d },
    { "az", 0x42c },
    { "az", 0x82c },
    { "eu", 0x42d },
    { "be_BY", 0x423 },
    { "bn_IN", 0x445 },
    { "bn_BD", 0x845 },
    { "bg_BG", 0x402 },
    { "my", 0x455 },
    { "ca", 0x403 },
    { "km", 0x453 },
    { "zh_TW", 0x404 },		/* Trad */
    { "zh_CN", 0x804 },		/* Simp */
    { "zh_HK", 0xc04 },		/* Trad */
    { "zh_SG", 0x1004 },	/* Simp */
    { "zh_MO", 0x1404 },	/* Trad */
    { "hr", 0x41a },
    { "hr_BA", 0x101a },
    { "cs_CZ", 0x405 },
    { "da_DK", 0x406 },
    { "div", 0x465 },
    { "nl_NL", 0x413 },
    { "nl_BE", 0x813 },
    { "en_UK", 0x809 },
    { "en_US", 0x409 },
    { "en_CA", 0x1009 },
    { "en_AU", 0xc09 },
    { "en_NZ", 0x1409 },
    { "en_IE", 0x1809 },
    { "en_ZA", 0x1c09 },
    { "en_JM", 0x2009 },
    { "en", 0x2409 },
    { "en_BZ", 0x2809 },
    { "en_TT", 0x2c09 },
    { "en_ZW", 0x3009 },
    { "en_PH", 0x3409 },
    { "en_ID", 0x3809 },
    { "en_HK", 0x3c09 },
    { "en_IN", 0x4009 },
    { "en_MY", 0x4409 },
    { "et_EE", 0x425 },
    { "fo", 0x438 },
/* No language code for filipino */
    { "fa", 0x429 },
    { "fi_FI", 0x40b },
    { "fr_FR", 0x40c },
    { "fr_BE", 0x80c },
    { "fr_CA", 0xc0c },
    { "fr_CH", 0x100c },
    { "fr_LU", 0x140c },
    { "fr_MC", 0x180c },
    { "fr", 0x1c0c },		/* West Indes */
    { "fr_RE", 0x200c },
    { "fr_CD", 0x240c },
    { "fr_SN", 0x280c },
    { "fr_CM", 0x2c0c },
    { "fr_CI", 0x300c },
    { "fr_ML", 0x340c },
    { "fr_MA", 0x380c },
    { "fr_HT", 0x3c0c },
    { "fr_DZ", 0xe40c },	/* North African is most likely to be Algeria, possibly Tunisia */
    { "fy", 0x462 },
    { "gl", 0x456 },
    { "ka", 0x437 },
    { "de_DE", 0x407 },
    { "de_CH", 0x807 },
    { "de_AT", 0xc07 },
    { "de_LU", 0x1007 },
    { "de_LI", 0x1407 },
    { "el_GR", 0x408 },
    { "ga", 0x83c },
    { "gd", 0x43c },
    { "gn", 0x474 },
    { "gu", 0x447 },
    { "ha", 0x468 },
    { "he_IL", 0x40d },
    { "iw", 0x40d },		/* Obsolete name for Hebrew */
    { "hi", 0x439 },
    { "hu_HU", 0x40e },
    { "is_IS", 0x40f },
    { "id", 0x421 },
    { "in", 0x421 },		/* Obsolete name for Indonesean */
    { "iu", 0x45d },
    { "it_IT", 0x410 },
    { "it_CH", 0x810 },
    { "ja_JP", 0x411 },
    { "kn", 0x44b },
    { "ks_IN", 0x860 },
    { "kk", 0x43f },
    { "ky", 0x440 },
    { "km", 0x453 },
    { "kok", 0x457 },
    { "ko", 0x412 },
    { "ko", 0x812 },	/*Johab */
    { "lo", 0x454 },
    { "la", 0x476 },
    { "lv_LV", 0x426 },
    { "lt_LT", 0x427 },
    { "lt", 0x827 },	/* Classic */
    { "mk", 0x42f },
    { "ms", 0x43e },
    { "ms", 0x83e },
    { "ml", 0x44c },
    { "mt", 0x43a },
    { "mr", 0x44e },
    { "mn", 0x450 },
    { "ne_NP", 0x461 },
    { "ne_IN", 0x861 },
    { "no_NO", 0x414 },	/* Bokmal */
    { "no_NO", 0x814 },	/* Nynorsk */
    { "or", 0x448 },
    { "om", 0x472 },
    { "ps", 0x463 },
    { "pl_PL", 0x415 },
    { "pt_PT", 0x416 },
    { "pt_BR", 0x816 },
    { "pa_IN", 0x446 },
    { "pa_PK", 0x846 },
    { "qu_BO", 0x46b },
    { "qu_EC", 0x86b },
    { "qu_PE", 0xc6b },
    { "rm", 0x417 },
    { "ro_RO", 0x418 },
    { "ro_MD", 0x818 },
    { "ru_RU", 0x419 },
    { "ru_MD", 0x819 },
    { "smi", 0x43b },
    { "sa", 0x43b },
/* No language code for Sepedi */
    { "sr", 0xc1a },	/* Cyrillic */
    { "sr", 0x81a },	/* Latin */
    { "sd_IN", 0x459 },
    { "sd_PK", 0x859 },
    { "si", 0x45b },
    { "sk_SK", 0x41b },
    { "sl_SI", 0x424 },
    { "wen", 0x42e },
    { "es_ES", 0x40a },	/* traditional spanish */
    { "es_MX", 0x80a },
    { "es_ES", 0xc0a },	/* Modern spanish */
    { "es_GT", 0x100a },
    { "es_CR", 0x140a },
    { "es_PA", 0x180a },
    { "es_DO", 0x1c0a },
    { "es_VE", 0x200a },
    { "es_CO", 0x240a },
    { "es_PE", 0x280a },
    { "es_AR", 0x2c0a },
    { "es_EC", 0x300a },
    { "es_CL", 0x340a },
    { "es_UY", 0x380a },
    { "es_PY", 0x3c0a },
    { "es_BO", 0x400a },
    { "es_SV", 0x440a },
    { "es_HN", 0x480a },
    { "es_NI", 0x4c0a },
    { "es_PR", 0x500a },
    { "es_US", 0x540a },
    { "sutu", 0x430 },
    { "sw_KE", 0x441 },
    { "sv_SE", 0x41d },
    { "sv_FI", 0x81d },
    { "tl", 0x464 },
    { "tg", 0x464 },
    { "ta", 0x449 },
    { "tt", 0x444 },
    { "te", 0x44a },
    { "th", 0x41e },
    { "bo_CN", 0x451 },
    { "bo_BT", 0x451 },
    { "ti_ET", 0x473 },
    { "ti_ER", 0x873 },
    { "ts", 0x431 },
    { "tn", 0x432 },
    { "tr_TR", 0x41f },
    { "tk", 0x442 },
    { "uk_UA", 0x422 },
    { "ug", 0x480 },
    { "ur_PK", 0x420 },
    { "ur_IN", 0x820 },
    { "uz", 0x443 },	/* Latin */
    { "uz", 0x843 },	/* Cyrillic */
    { "ven", 0x433 },
    { "vi", 0x42a },
    { "cy", 0x452 },
    { "xh", 0x434 },
    { "yi", 0x43d },
    { "ji", 0x43d },	/* Obsolete Yiddish */
    { "yo", 0x46a },
    { "zu", 0x435 },
    { NULL }};
#endif

static GTextInfo mslanguages[] = {
    { (unichar_t *) N_("Afrikaans"), NULL, 0, 0, (void *) 0x436, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Albanian"), NULL, 0, 0, (void *) 0x41c, NULL, 0, 0, 0, 0, 0, 0, 1},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Malayalam", ignore "Lang|" */
    { (unichar_t *) N_("Lang|Amharic"), NULL, 0, 0, (void *) 0x45e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Saudi Arabia)"), NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Iraq)"), NULL, 0, 0, (void *) 0x801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Egypt)"), NULL, 0, 0, (void *) 0xc01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Libya)"), NULL, 0, 0, (void *) 0x1001, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Algeria)"), NULL, 0, 0, (void *) 0x1401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Morocco)"), NULL, 0, 0, (void *) 0x1801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Tunisia)"), NULL, 0, 0, (void *) 0x1C01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Oman)"), NULL, 0, 0, (void *) 0x2001, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Yemen)"), NULL, 0, 0, (void *) 0x2401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Syria)"), NULL, 0, 0, (void *) 0x2801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Jordan)"), NULL, 0, 0, (void *) 0x2c01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Lebanon)"), NULL, 0, 0, (void *) 0x3001, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Kuwait)"), NULL, 0, 0, (void *) 0x3401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (U.A.E.)"), NULL, 0, 0, (void *) 0x3801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Bahrain)"), NULL, 0, 0, (void *) 0x3c01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic (Qatar)"), NULL, 0, 0, (void *) 0x4001, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Armenian"), NULL, 0, 0, (void *) 0x42b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Assamese"), NULL, 0, 0, (void *) 0x44d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Azeri (Latin)"), NULL, 0, 0, (void *) 0x42c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Azeri (Cyrillic)"), NULL, 0, 0, (void *) 0x82c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Basque"), NULL, 0, 0, (void *) 0x42d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Byelorussian"), NULL, 0, 0, (void *) 0x423, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Bengali"), NULL, 0, 0, (void *) 0x445, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Bengali Bangladesh"), NULL, 0, 0, (void *) 0x845, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Bulgarian"), NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Burmese"), NULL, 0, 0, (void *) 0x455, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Catalan"), NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Cambodian"), NULL, 0, 0, (void *) 0x453, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Cherokee"), NULL, 0, 0, (void *) 0x45c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (Taiwan)"), NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (PRC)"), NULL, 0, 0, (void *) 0x804, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (Hong Kong)"), NULL, 0, 0, (void *) 0xc04, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (Singapore)"), NULL, 0, 0, (void *) 0x1004, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (Macau)"), NULL, 0, 0, (void *) 0x1404, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Croatian"), NULL, 0, 0, (void *) 0x41a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Croatian Bosnia/Herzegovina"), NULL, 0, 0, (void *) 0x101a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Czech"), NULL, 0, 0, (void *) 0x405, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Danish"), NULL, 0, 0, (void *) 0x406, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Divehi"), NULL, 0, 0, (void *) 0x465, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Dutch"), NULL, 0, 0, (void *) 0x413, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Flemish (Belgian Dutch)"), NULL, 0, 0, (void *) 0x813, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Edo"), NULL, 0, 0, (void *) 0x466, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (British)"), NULL, 0, 0, (void *) 0x809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (US)"), NULL, 0, 0, (void *) 0x409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Canada)"), NULL, 0, 0, (void *) 0x1009, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Australian)"), NULL, 0, 0, (void *) 0xc09, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (New Zealand)"), NULL, 0, 0, (void *) 0x1409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Irish)"), NULL, 0, 0, (void *) 0x1809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (South Africa)"), NULL, 0, 0, (void *) 0x1c09, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Jamaica)"), NULL, 0, 0, (void *) 0x2009, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Caribbean)"), NULL, 0, 0, (void *) 0x2409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Belize)"), NULL, 0, 0, (void *) 0x2809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Trinidad)"), NULL, 0, 0, (void *) 0x2c09, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Zimbabwe)"), NULL, 0, 0, (void *) 0x3009, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Philippines)"), NULL, 0, 0, (void *) 0x3409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Indonesia)"), NULL, 0, 0, (void *) 0x3809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Hong Kong)"), NULL, 0, 0, (void *) 0x3c09, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (India)"), NULL, 0, 0, (void *) 0x4009, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Malaysia)"), NULL, 0, 0, (void *) 0x4409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Estonian"), NULL, 0, 0, (void *) 0x425, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Faeroese"), NULL, 0, 0, (void *) 0x438, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Farsi"), NULL, 0, 0, (void *) 0x429, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Filipino"), NULL, 0, 0, (void *) 0x464, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Finnish"), NULL, 0, 0, (void *) 0x40b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French French"), NULL, 0, 0, (void *) 0x40c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Belgium"), NULL, 0, 0, (void *) 0x80c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Canadian"), NULL, 0, 0, (void *) 0xc0c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Swiss"), NULL, 0, 0, (void *) 0x100c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Luxembourg"), NULL, 0, 0, (void *) 0x140c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Monaco"), NULL, 0, 0, (void *) 0x180c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French West Indies"), NULL, 0, 0, (void *) 0x1c0c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("French R\303\251union"), NULL, 0, 0, (void *) 0x200c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French D.R. Congo"), NULL, 0, 0, (void *) 0x240c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Senegal"), NULL, 0, 0, (void *) 0x280c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Camaroon"), NULL, 0, 0, (void *) 0x2c0c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("French C\303\264te d'Ivoire"), NULL, 0, 0, (void *) 0x300c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Mali"), NULL, 0, 0, (void *) 0x340c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Morocco"), NULL, 0, 0, (void *) 0x380c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Haiti"), NULL, 0, 0, (void *) 0x3c0c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French North Africa"), NULL, 0, 0, (void *) 0xe40c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Frisian"), NULL, 0, 0, (void *) 0x462, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Fulfulde"), NULL, 0, 0, (void *) 0x467, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gaelic (Scottish)"), NULL, 0, 0, (void *) 0x43c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gaelic (Irish)"), NULL, 0, 0, (void *) 0x83c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Galician"), NULL, 0, 0, (void *) 0x467, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Georgian"), NULL, 0, 0, (void *) 0x437, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German German"), NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German Swiss"), NULL, 0, 0, (void *) 0x807, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German Austrian"), NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German Luxembourg"), NULL, 0, 0, (void *) 0x1007, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German Liechtenstein"), NULL, 0, 0, (void *) 0x1407, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Greek"), NULL, 0, 0, (void *) 0x408, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Guarani"), NULL, 0, 0, (void *) 0x474, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Gujarati"), NULL, 0, 0, (void *) 0x447, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Hausa"), NULL, 0, 0, (void *) 0x468, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Hawaiian"), NULL, 0, 0, (void *) 0x475, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Hebrew"), NULL, 0, 0, (void *) 0x40d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Hindi"), NULL, 0, 0, (void *) 0x439, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Hungarian"), NULL, 0, 0, (void *) 0x40e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ibibio"), NULL, 0, 0, (void *) 0x469, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Icelandic"), NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Igbo"), NULL, 0, 0, (void *) 0x470, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Indonesian"), NULL, 0, 0, (void *) 0x421, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Inuktitut"), NULL, 0, 0, (void *) 0x45d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Italian"), NULL, 0, 0, (void *) 0x410, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Italian Swiss"), NULL, 0, 0, (void *) 0x810, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Japanese"), NULL, 0, 0, (void *) 0x411, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Kannada"), NULL, 0, 0, (void *) 0x44b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Kanuri"), NULL, 0, 0, (void *) 0x471, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Kashmiri (India)"), NULL, 0, 0, (void *) 0x860, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Kazakh"), NULL, 0, 0, (void *) 0x43f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Khmer"), NULL, 0, 0, (void *) 0x453, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Kirghiz"), NULL, 0, 0, (void *) 0x440, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Konkani"), NULL, 0, 0, (void *) 0x457, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Korean"), NULL, 0, 0, (void *) 0x412, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Korean (Johab)"), NULL, 0, 0, (void *) 0x812, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lao"), NULL, 0, 0, (void *) 0x454, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Latvian"), NULL, 0, 0, (void *) 0x426, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Latin"), NULL, 0, 0, (void *) 0x476, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lithuanian"), NULL, 0, 0, (void *) 0x427, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lithuanian (Classic)"), NULL, 0, 0, (void *) 0x827, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Macedonian"), NULL, 0, 0, (void *) 0x42f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Malay"), NULL, 0, 0, (void *) 0x43e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Malay (Brunei)"), NULL, 0, 0, (void *) 0x83e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Malayalam"), NULL, 0, 0, (void *) 0x44c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Maltese"), NULL, 0, 0, (void *) 0x43a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Manipuri"), NULL, 0, 0, (void *) 0x458, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Maori"), NULL, 0, 0, (void *) 0x481, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Marathi"), NULL, 0, 0, (void *) 0x44e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Mongolian (Cyrillic)"), NULL, 0, 0, (void *) 0x450, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Mongolian (Mongolian)"), NULL, 0, 0, (void *) 0x850, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Nepali"), NULL, 0, 0, (void *) 0x461, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Nepali (India)"), NULL, 0, 0, (void *) 0x861, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Norwegian (Bokmal)"), NULL, 0, 0, (void *) 0x414, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Norwegian (Nynorsk)"), NULL, 0, 0, (void *) 0x814, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Oriya"), NULL, 0, 0, (void *) 0x448, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oromo"), NULL, 0, 0, (void *) 0x472, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Papiamentu"), NULL, 0, 0, (void *) 0x479, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Pashto"), NULL, 0, 0, (void *) 0x463, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Polish"), NULL, 0, 0, (void *) 0x415, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Portugese (Portugal)"), NULL, 0, 0, (void *) 0x416, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Portuguese (Brasil)"), NULL, 0, 0, (void *) 0x816, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Punjabi (India)"), NULL, 0, 0, (void *) 0x446, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Punjabi (Pakistan)"), NULL, 0, 0, (void *) 0x846, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Quecha (Bolivia)"), NULL, 0, 0, (void *) 0x46b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Quecha (Ecuador)"), NULL, 0, 0, (void *) 0x86b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Quecha (Peru)"), NULL, 0, 0, (void *) 0xc6b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Rhaeto-Romanic"), NULL, 0, 0, (void *) 0x417, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Romanian"), NULL, 0, 0, (void *) 0x418, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Romanian (Moldova)"), NULL, 0, 0, (void *) 0x818, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Russian"), NULL, 0, 0, (void *) 0x419, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Russian (Moldova)"), NULL, 0, 0, (void *) 0x819, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sami (Lappish)"), NULL, 0, 0, (void *) 0x43b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sanskrit"), NULL, 0, 0, (void *) 0x43b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sepedi"), NULL, 0, 0, (void *) 0x46c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Serbian (Cyrillic)"), NULL, 0, 0, (void *) 0xc1a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Serbian (Latin)"), NULL, 0, 0, (void *) 0x81a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sindhi India"), NULL, 0, 0, (void *) 0x459, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sindhi Pakistan"), NULL, 0, 0, (void *) 0x859, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Sinhalese"), NULL, 0, 0, (void *) 0x45b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slovak"), NULL, 0, 0, (void *) 0x41b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slovenian"), NULL, 0, 0, (void *) 0x424, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sorbian"), NULL, 0, 0, (void *) 0x42e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Traditional)"), NULL, 0, 0, (void *) 0x40a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish Mexico"), NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Modern)"), NULL, 0, 0, (void *) 0xc0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Guatemala)"), NULL, 0, 0, (void *) 0x100a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Costa Rica)"), NULL, 0, 0, (void *) 0x140a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Panama)"), NULL, 0, 0, (void *) 0x180a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Dominican Republic)"), NULL, 0, 0, (void *) 0x1c0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Venezuela)"), NULL, 0, 0, (void *) 0x200a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Colombia)"), NULL, 0, 0, (void *) 0x240a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Peru)"), NULL, 0, 0, (void *) 0x280a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Argentina)"), NULL, 0, 0, (void *) 0x2c0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Ecuador)"), NULL, 0, 0, (void *) 0x300a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Chile)"), NULL, 0, 0, (void *) 0x340a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Uruguay)"), NULL, 0, 0, (void *) 0x380a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Paraguay)"), NULL, 0, 0, (void *) 0x3c0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Bolivia)"), NULL, 0, 0, (void *) 0x400a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (El Salvador)"), NULL, 0, 0, (void *) 0x440a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Honduras)"), NULL, 0, 0, (void *) 0x480a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Nicaragua)"), NULL, 0, 0, (void *) 0x4c0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Puerto Rico)"), NULL, 0, 0, (void *) 0x500a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (United States)"), NULL, 0, 0, (void *) 0x540a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Latin America)"), NULL, 0, 0, (void *) 0xe40a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sutu"), NULL, 0, 0, (void *) 0x430, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Swahili (Kenyan)"), NULL, 0, 0, (void *) 0x441, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Swedish (Sweden)"), NULL, 0, 0, (void *) 0x41d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Swedish (Finland)"), NULL, 0, 0, (void *) 0x81d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Syriac"), NULL, 0, 0, (void *) 0x45a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Tagalog"), NULL, 0, 0, (void *) 0x464, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tajik"), NULL, 0, 0, (void *) 0x428, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tamazight (Arabic)"), NULL, 0, 0, (void *) 0x45f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tamazight (Latin)"), NULL, 0, 0, (void *) 0x85f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Tamil"), NULL, 0, 0, (void *) 0x449, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tatar (Tatarstan)"), NULL, 0, 0, (void *) 0x444, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Telugu"), NULL, 0, 0, (void *) 0x44a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Thai"), NULL, 0, 0, (void *) 0x41e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tibetan (PRC)"), NULL, 0, 0, (void *) 0x451, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tibetan Bhutan"), NULL, 0, 0, (void *) 0x851, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tigrinya Ethiopia"), NULL, 0, 0, (void *) 0x473, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tigrinyan Eritrea"), NULL, 0, 0, (void *) 0x873, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tsonga"), NULL, 0, 0, (void *) 0x431, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tswana"), NULL, 0, 0, (void *) 0x432, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Turkish"), NULL, 0, 0, (void *) 0x41f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Turkmen"), NULL, 0, 0, (void *) 0x442, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Uighur"), NULL, 0, 0, (void *) 0x480, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ukrainian"), NULL, 0, 0, (void *) 0x422, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Urdu (Pakistan)"), NULL, 0, 0, (void *) 0x420, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Urdu (India)"), NULL, 0, 0, (void *) 0x820, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Uzbek (Latin)"), NULL, 0, 0, (void *) 0x443, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Uzbek (Cyrillic)"), NULL, 0, 0, (void *) 0x843, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Venda"), NULL, 0, 0, (void *) 0x433, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Vietnamese"), NULL, 0, 0, (void *) 0x42a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Welsh"), NULL, 0, 0, (void *) 0x452, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Xhosa"), NULL, 0, 0, (void *) 0x434, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Yi"), NULL, 0, 0, (void *) 0x478, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Yiddish"), NULL, 0, 0, (void *) 0x43d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Yoruba"), NULL, 0, 0, (void *) 0x46a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Zulu"), NULL, 0, 0, (void *) 0x435, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo ttfnameids[] = {
/* Put styles (docs call it subfamily) first because it is most likely to change */
    { (unichar_t *) N_("Styles (SubFamily)"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) N_("Copyright"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Family"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Fullname"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("UniqueID"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Version"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
/* Don't give user access to PostscriptName, we set that elsewhere */
    { (unichar_t *) N_("Trademark"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Manufacturer"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Designer"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Descriptor"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Vendor URL"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Designer URL"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("License"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("License URL"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
/* slot 15 is reserved */
    { (unichar_t *) N_("Preferred Family"), NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Preferred Styles"), NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Compatible Full"), NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sample Text"), NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CID findfont Name"), NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static char *TN_DefaultName(GGadget *g, int r, int c);
static void TN_StrIDEnable(GGadget *g,GMenuItem *mi, int r, int c);
static void TN_LangEnable(GGadget *g,GMenuItem *mi, int r, int c);
static struct col_init ci[3] = {
    { me_enum, NULL, mslanguages, TN_LangEnable, N_("Language") },
    { me_enum, NULL, ttfnameids, TN_StrIDEnable, N_("String ID") },
    { me_func, TN_DefaultName, NULL, NULL, N_("String") }
    };
#endif
static GTextInfo gridfit[] = {
    { (unichar_t *) N_("No Grid Fit"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) N_("Grid Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo antialias[] = {
    { (unichar_t *) N_("No Anti-Alias"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) N_("Anti-Alias"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static GTextInfo symsmooth[] = {
    { (unichar_t *) N_("No Symmetric-Smooth"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) N_("Symmetric-Smoothing"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo gfsymsmooth[] = {
    { (unichar_t *) N_("No Grid Fit w/ Sym-Smooth"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) N_("Grid Fit w/ Sym-Smooth"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static struct col_init gaspci[5] = {
    { me_int , NULL, NULL, NULL, N_("Gasp|Pixels Per EM") },
    { me_enum, NULL, gridfit, NULL, N_("Gasp|Grid Fit") },
    { me_enum, NULL, antialias, NULL, N_("Gasp|Anti-Alias") },
    { me_enum, NULL, symsmooth, NULL, N_("Gasp|Symmetric Smoothing") },
    { me_enum, NULL, gfsymsmooth, NULL, N_("Gasp|Grid Fit w/ Sym Smooth") }
    };
#endif

struct langstyle { int lang; const char *str; };
static const char regulareng[] = "Regular";
static const char demiboldeng[] = "DemiBold";
static const char demiboldeng3[] = "Demi";
static const char demiboldeng5[] = "SemiBold";
static const char boldeng[] = "Bold";
static const char thineng[] = "Thin";
static const char lighteng[] = "Light";
static const char extralighteng[] = "ExtraLight";
static const char extralighteng2[] = "VeryLight";
static const char mediumeng[] = "Medium";
static const char bookeng[] = "Book";
static const char heavyeng[] = "Heavy";
static const char blackeng[] = "Black";
static const char italiceng[] = "Italic";
static const char obliqueeng[] = "Oblique";
static const char condensedeng[] = "Condensed";
static const char expandedeng[] = "Expanded";
static const char outlineeng[] = "Outline";

static const char regularfren[] = "Normal";
static const char boldfren[] = "Gras";
static const char demiboldfren[] = "DemiGras";
static const char demiboldfren2[] = "Demi";
static const char mediumfren[] = "Normal";
static const char lightfren[] = "Maigre";
static const char blackfren[] = "ExtraGras";
static const char italicfren[] = "Italique";
static const char obliquefren[] = "Oblique";
static const char condensedfren[] = "Etroite";
static const char expandedfren[] = "Elargi";
static const char outlinefren[] = "Contour";

static const char regulargerm[] = "Standard";
static const char demiboldgerm[] = "Halbfett";
static const char demiboldgerm2[] = "Schmallfett";
static const char boldgerm[] = "Fett";
static const char boldgerm2[] = "Dick";
static const char blackgerm[] = "Schwarz";
static const char lightgerm[] = "Mager";
static const char mediumgerm[] = "Mittel";
static const char bookgerm[] = "Buchschrift";
static const char italicgerm[] = "Kursiv";
static const char obliquegerm[] = "Schr\303\244g";
static const char condensedgerm[] = "Schmal";
static const char expandedgerm[] = "Breit";
static const char outlinegerm[] = "Konturert";

static const char regularspan[] = "Normal";
static const char boldspan[] = "Negrita";
static const char lightspan[] = "Fina";
static const char blackspan[] = "Supernegra";
static const char italicspan[] = "Cursiva";
static const char condensedspan[] = "Condensada";
static const char expandedspan[] = "Amplida";

static const char regulardutch[] = "Normaal";
static const char mediumdutch[] = "Normaal";
static const char bookdutch[] = "Normaal";
static const char bolddutch[] = "Vet";
static const char demibolddutch[] = "Halfvet";
static const char lightdutch[] = "Licht mager";
static const char blackdutch[] = "Extra vet";
static const char italicdutch[] = "Cursief";
static const char italicdutch2[] = "italiek";
static const char obliquedutch[] = "oblique";
static const char obliquedutch2[] = "schuin";
static const char condenseddutch[] = "Smal";
static const char expandeddutch[] = "Breed";
static const char outlinedutch[] = "Contour";

static const char regularswed[] = "Mager";
static const char boldswed[] = "Fet";
static const char lightswed[] = "Extrafin";
static const char blackswed[] = "Extrafet";
static const char italicswed[] = "Kursiv";
static const char condensedswed[] = "Smal";
static const char expandedswed[] = "Bred";

static const char regulardanish[] = "Normal";
static const char bolddanish[] = "Fed";
static const char demibolddanish[] = "Halvfed";
static const char lightdanish[] = "Fin";
static const char mediumdanish[] = "Medium";
static const char blackdanish[] = "Extrafed";
static const char italicdanish[] = "Kursiv";
static const char condenseddanish[] = "Smal";
static const char expandeddanish[] = "Bred";
static const char outlinedanish[] = "Kontour";

static const char regularnor[] = "Vanlig";
static const char boldnor[] = "Halvfet";
static const char lightnor[] = "Mager";
static const char blacknor[] = "Fet";
static const char italicnor[] = "Kursiv";
static const char condensednor[] = "Smal";
static const char expandednor[] = "Sperret";

static const char regularital[] = "Normale";
static const char demiboldital[] = "Nerretto";
static const char boldital[] = "Nero";
static const char thinital[] = "Fine";
static const char lightital[] = "Chiaro";
static const char mediumital[] = "Medio";
static const char bookital[] = "Libro";
static const char heavyital[] = "Nerissimo";
static const char blackital[] = "ExtraNero";
static const char italicital[] = "Cursivo";
static const char obliqueital[] = "Obliquo";
static const char condensedital[] = "Condensato";
static const char expandedital[] = "Allargato";

static const char regularru[] = "\320\236\320\261\321\213\321\207\320\275\321\213\320\271";
static const char demiboldru[] = "\320\237\320\276\320\273\321\203\320\266\320\270\321\200\320\275\321\213\320\271";
static const char boldru[] = "\320\236\320\261\321\213\321\207\320\275\321\213\320\271";
static const char heavyru[] = "\320\241\320\262\320\265\321\200\321\205\320\266\320\270\321\200\320\275\321\213\320\271";
static const char blackru[] = "\320\247\321\221\321\200\320\275\321\213\320\271";
static const char thinru[] = "\320\242\320\276\320\275\320\272\320\270\320\271";
static const char lightru[] = "\320\241\320\262\320\265\321\202\320\273\321\213\320\271";
static const char italicru[] = "\320\232\321\203\321\200\321\201\320\270\320\262";
static const char obliqueru[] = "\320\235\320\260\320\272\320\273\320\276\320\275";
static const char condensedru[] = "\320\243\320\267\320\272\320\270\320\271";
static const char expandedru[] = "\320\250\320\270\321\200\320\276\320\272\320\270\320\271";

static const char regularhu[] = "Norm\303\241l";
static const char demiboldhu[] = "Negyedk\303\266v\303\251r";
static const char demiboldhu2[] = "F\303\251lk\303\266v\303\251r";
static const char boldhu[] = "F\303\251lk\303\266v\303\251r";
static const char boldhu2[] = "H\303\241romnegyedk\303\266v\303\251r";
static const char thinhu[] = "Sov\303\241ny";
static const char lighthu[] = "Vil\303\241gos";
static const char mediumhu[] = "K\303\266zepes";
static const char bookhu[] = "Halv\303\241ny";
static const char bookhu2[] = "K\303\266nyv";
static const char heavyhu[] = "K\303\266v\303\251r";
static const char heavyhu2[] = "Extrak\303\266v\303\251r";
static const char blackhu[] = "Fekete";
static const char blackhu2[] = "S\303\266t\303\251t";
static const char italichu[] = "D\305\221lt";
static const char obliquehu[] = "D\303\266nt\303\266tt";
static const char obliquehu2[] = "Ferde";
static const char condensedhu[] = "Keskeny";
static const char expandedhu[] = "Sz\303\251les";
static const char outlinehu[] = "Kont\303\272ros";

static const char regularpl[] = "podstawowa";
static const char demiboldpl[] = "p\303\263\305\202gruba";
static const char boldpl[] = "pogrubiona";
static const char thinpl[] = "cienka";
static const char lightpl[] = "bardzo cienka";
static const char heavypl[] = "bardzo gruba";
static const char italicpl[] = "pochy\305\202a";
static const char obliquepl[] = "pochy\305\202a";
static const char condensedpl[] = "w\304\205ska";
static const char expandedpl[] = "szeroka";
static const char outlinepl[] = "konturowa";
static const char mediumpl[] = "zwyk\305\202a";
static const char bookpl[] = "zwyk\305\202a";


#ifndef LUA_FF_LIB

static struct langstyle regs[] = { {0x409, regulareng}, { 0x40c, regularfren }, { 0x410, regularital }, { 0x407, regulargerm }, { 0x40a, regularspan }, { 0x419, regularru }, { 0x40e, regularhu },
	{ 0x413, regulardutch}, { 0x41d, regularswed }, { 0x414, regularnor },
	{ 0x406, regulardanish}, {0x415, regularpl }, { 0x804, "\346\255\243\345\270\270"},
	{ 0x408, "\316\272\316\261\316\275\316\277\316\275\316\271\316\272\316\256"}, { 0x42a, "Chu\341\272\251n"}, { 0 }};
static struct langstyle meds[] = { {0x409, mediumeng}, { 0x410, mediumital },
	{ 0x40c, mediumfren }, { 0x407, mediumgerm }, { 0x40e, mediumhu },
	{ 0x406, mediumdanish}, {0x415, mediumpl }, { 0x804, "\344\270\255\347\255\211"}, 
	{ 0x408, "\302\265\316\265\317\203\316\261\316\257\316\261"}, { 0x42a, "V\341\273\253a"}, { 0x413, mediumdutch}, { 0 }};
static struct langstyle books[] = { {0x409, bookeng}, { 0x410, bookital },
	{ 0x407, bookgerm }, { 0x40e, bookhu }, { 0x40e, bookhu2 },
	{ 0x415, bookpl}, { 0x804, "\344\271\246\344\275\223"}, { 0x408, "\303\237\316\271\303\237\316\273\316\257\316\277\317\205"},
	{ 0x42a, "S\303\241ch"}, { 0x413, bookdutch}, { 0 }};
static struct langstyle bolds[] = { {0x409, boldeng}, { 0x410, boldital }, { 0x40c, boldfren }, { 0x407, boldgerm }, { 0x407, boldgerm2 }, { 0x40a, boldspan}, { 0x419, boldru }, { 0x40e, boldhu }, { 0x40e, boldhu2 }, 
	{ 0x413, bolddutch}, { 0x41d, boldswed }, { 0x414, boldnor },
	{ 0x406, bolddanish}, { 0x415, boldpl}, { 0x804, "\347\262\227\344\275\223"},  
	{ 0x408, "\316\255\316\275\317\204\316\277\316\275\316\267"}, { 0x42a, "\304\220\341\272\255m"}, { 0 }};
static struct langstyle italics[] = { {0x409, italiceng}, { 0x410, italicital }, { 0x40c, italicfren }, { 0x407, italicgerm }, { 0x40a, italicspan}, { 0x419, italicru }, { 0x40e, italichu },
	{ 0x413, italicdutch}, { 0x413, italicdutch2}, { 0x41d, italicswed }, { 0x414, italicnor },
	{ 0x406, italicdanish}, { 0x415, italicpl}, { 0x804, "\346\226\234\344\275\223"},
	{ 0x408, "\316\233\316\265\316\271\317\210\316\257\316\261\317\202"}, { 0x42a, "Nghi\303\252ng" }, { 0 }};
static struct langstyle obliques[] = { {0x409, obliqueeng}, { 0x410, obliqueital },
	{ 0x40c, obliquefren }, { 0x407, obliquegerm }, { 0x419, obliqueru },
	{ 0x40e, obliquehu }, { 0x40e, obliquehu2 }, {0x415, obliquepl},
	{ 0x804, "\346\226\234\344\275\223"}, { 0x408, "\317\200\316\273\316\254\316\263\316\271\316\261"},
	{ 0x42a, "Xi\303\252n" }, { 0x413, obliquedutch}, { 0x413, obliquedutch2}, { 0 }};
static struct langstyle demibolds[] = { {0x409, demiboldeng}, {0x409, demiboldeng3}, {0x409, demiboldeng5},
	{ 0x410, demiboldital }, { 0x40c, demiboldfren }, { 0x40c, demiboldfren2 }, { 0x407, demiboldgerm }, { 0x407, demiboldgerm2 },
	{ 0x419, demiboldru }, { 0x40e, demiboldhu }, { 0x40e, demiboldhu2 },
	{ 0x406, demibolddanish}, { 0x415, demiboldpl },
	{ 0x804, "\347\225\245\347\262\227"}, { 0x408, "\316\267\302\265\316\271\316\255\316\275\317\204\316\277\316\275\316\267"},
	{ 0x42a, "N\341\273\255a \304\221\341\272\255m"}, { 0x413, demibolddutch}, { 0 }};
static struct langstyle heavys[] = { {0x409, heavyeng}, { 0x410, heavyital },
	{ 0x419, heavyru }, { 0x40e, heavyhu }, { 0x40e, heavyhu2 },
	{ 0x415, heavypl }, { 0x804, "\347\262\227"}, { 0 }};
static struct langstyle blacks[] = { {0x409, blackeng}, { 0x410, blackital }, { 0x40c, blackfren }, { 0x407, blackgerm }, { 0x419, blackru }, { 0x40e, blackhu }, { 0x40e, blackhu2 }, { 0x40a, blackspan }, 
	{ 0x413, blackdutch}, { 0x41d, blackswed }, { 0x414, blacknor }, { 0x406, blackdanish}, 
	{ 0x415, heavypl }, { 0x804, "\351\273\221"},  { 0x408, "\302\265\316\261\317\215\317\201\316\261"},
									 { 0x42a, "\304\220en"}, {0} };
static struct langstyle thins[] = { {0x409, thineng}, { 0x410, thinital },
	{ 0x419, thinru }, { 0x40e, thinhu }, { 0x415, thinpl},
	{ 0x804, "\347\273\206"}, { 0 }};
static struct langstyle extralights[] = { {0x409, extralighteng}, {0x409, extralighteng2},
	{ 0x804, "\346\236\201\347\273\206"}, {0}};
static struct langstyle lights[] = { {0x409, lighteng}, {0x410, lightital}, {0x40c, lightfren}, {0x407, lightgerm}, { 0x419, lightru }, { 0x40e, lighthu }, { 0x40a, lightspan }, 
	{ 0x413, lightdutch}, { 0x41d, lightswed }, { 0x414, lightnor },
	{ 0x406, lightdanish}, { 0x415, lightpl}, { 0x804, "\347\273\206"},
	{ 0x408, "\316\273\316\265\317\200\317\204\316\256"}, { 0x42a, "Nh\341\272\271" }, { 0 }};
static struct langstyle condenseds[] = { {0x409, condensedeng}, {0x410, condensedital}, {0x40c, condensedfren}, {0x407, condensedgerm}, { 0x419, condensedru }, { 0x40e, condensedhu }, { 0x40a, condensedspan }, 
	{ 0x413, condenseddutch}, { 0x41d, condensedswed },
	{ 0x414, condensednor }, { 0x406, condenseddanish},
	{ 0x415, condensedpl }, { 0x804, "\345\216\213\347\274\251"},
	{ 0x408, "\317\200\317\205\316\272\316\275\316\256"}, { 0x42a, "H\341\272\271p" }, { 0 }};
static struct langstyle expandeds[] = { {0x409, expandedeng}, {0x410, expandedital}, {0x40c, expandedfren}, {0x407, expandedgerm}, { 0x419, expandedru }, { 0x40e, expandedhu }, { 0x40a, expandedspan }, 
	{ 0x413, expandeddutch}, { 0x41d, expandedswed }, { 0x414, expandednor },
	{ 0x406, expandeddanish}, { 0x415, expandedpl }, { 0x804, "\345\212\240\345\256\275"},
	{ 0x408, "\316\261\317\201\316\261\316\271\316\256"}, { 0x42a, "R\341\273\231ng" }, { 0 }};
static struct langstyle outlines[] = { {0x409, outlineeng}, {0x40c, outlinefren},
	{0x407, outlinegerm}, {0x40e, outlinehu}, { 0x406, outlinedanish},
	{0x415, outlinepl}, { 0x804, "\350\275\256\345\273\223"}, { 0x408, "\317\200\316\265\317\201\316\271\316\263\317\201\316\254\316\274\316\274\316\261\317\204\316\277\317\202"},
	{0x42a, "N\303\251t ngo\303\240i" }, { 0x413, outlinedutch}, { 0 }};
static struct langstyle *stylelist[] = {regs, meds, books, demibolds, bolds, heavys, blacks,
	extralights, lights, thins, italics, obliques, condenseds, expandeds, outlines, NULL };
#endif

#define CID_Features	101		/* Mac stuff */
#define CID_FeatureDel	103
#define CID_FeatureEdit	105

#define CID_Family	1002
#define CID_Weight	1003
#define CID_ItalicAngle	1004
#define CID_UPos	1005
#define CID_UWidth	1006
#define CID_Ascent	1007
#define CID_Descent	1008
#define CID_Notice	1010
#define CID_Version	1011
#define CID_UniqueID	1012
#define CID_HasVerticalMetrics	1013
#define CID_VOriginLab	1014
#define CID_VOrigin	1015
#define CID_Fontname	1016
#define CID_Em		1017
#define CID_Scale	1018
#define CID_IsOrder2	1019
#define CID_IsMultiLayer	1020
#define CID_Interpretation	1021
#define CID_IsStrokedFont	1022
#define CID_StrokeWidth		1023
#define CID_Namelist	1024
#define CID_XUID	1113
#define CID_Human	1114
#define CID_SameAsFontname	1115
#define CID_HasDefBase	1116
#define CID_DefBaseName	1117

#define CID_PrivateEntries	2001
#define	CID_PrivateValues	2002
#define	CID_Add			2003
#define CID_Guess		2004
#define CID_Remove		2005
#define CID_Hist		2006

#define CID_TTFTabs		3000
#define CID_WeightClass		3001
#define CID_WidthClass		3002
#define CID_PFMFamily		3003
#define CID_FSType		3004
#define CID_NoSubsetting	3005
#define CID_OnlyBitmaps		3006
#define CID_LineGap		3007
#define CID_VLineGap		3008
#define CID_VLineGapLab		3009
#define CID_WinAscent		3010
#define CID_WinAscentLab	3011
#define CID_WinAscentIsOff	3012
#define CID_WinDescent		3013
#define CID_WinDescentLab	3014
#define CID_WinDescentIsOff	3015
#define CID_TypoAscent		3016
#define CID_TypoAscentLab	3017
#define CID_TypoAscentIsOff	3018
#define CID_TypoDescent		3019
#define CID_TypoDescentLab	3020
#define CID_TypoDescentIsOff	3021
#define CID_TypoLineGap		3022
#define CID_HHeadAscent		3023
#define CID_HHeadAscentLab	3024
#define CID_HHeadAscentIsOff	3025
#define CID_HHeadDescent	3026
#define CID_HHeadDescentLab	3027
#define CID_HHeadDescentIsOff	3028
#define CID_Vendor		3029
#define CID_IBMFamily		3030
#define CID_OS2Version		3031
#define CID_UseTypoMetrics	3032
#define CID_WeightWidthSlopeOnly	3033

#define CID_SubSuperDefault	3100
#define CID_SubXSize		3101
#define CID_SubYSize		3102
#define CID_SubXOffset		3103
#define CID_SubYOffset		3104
#define CID_SuperXSize		3105
#define CID_SuperYSize		3106
#define CID_SuperXOffset	3107
#define CID_SuperYOffset	3108
#define CID_StrikeoutSize	3109
#define CID_StrikeoutPos	3110

#define CID_PanFamily		4001
#define CID_PanSerifs		4002
#define CID_PanWeight		4003
#define CID_PanProp		4004
#define CID_PanContrast		4005
#define CID_PanStrokeVar	4006
#define CID_PanArmStyle		4007
#define CID_PanLetterform	4008
#define CID_PanMidLine		4009
#define CID_PanXHeight		4010
#define CID_PanDefault		4011
#define CID_PanFamilyLab	4021
#define CID_PanSerifsLab	4022
#define CID_PanWeightLab	4023
#define CID_PanPropLab		4024
#define CID_PanContrastLab	4025
#define CID_PanStrokeVarLab	4026
#define CID_PanArmStyleLab	4027
#define CID_PanLetterformLab	4028
#define CID_PanMidLineLab	4029
#define CID_PanXHeightLab	4030

#define CID_TNLangSort		5001
#define CID_TNStringSort	5002
#define CID_TNVScroll		5003
#define CID_TNHScroll		5004
#define CID_TNames		5005

#define CID_Language		5006	/* Used by AskForLangNames */

#define CID_Gasp		5100
#define CID_GaspVersion		5101
#define CID_HeadClearType	5102

#define CID_Comment		6001

#define CID_MarkClasses		7101
#define CID_MarkNew		7102
#define CID_MarkEdit		7103

#define CID_TeXText		8001
#define CID_TeXMathSym		8002
#define CID_TeXMathExt		8003
#define CID_MoreParams		8005
#define CID_TeXExtraSpLabel	8006
#define CID_TeX			8007	/* through 8014 */

#define CID_DesignSize		8301
#define CID_DesignBottom	8302
#define CID_DesignTop		8303
#define CID_StyleID		8304
#define CID_StyleName		8305
#define CID_StyleNameNew	8306
#define CID_StyleNameDel	8307
#define CID_StyleNameRename	8308

#define CID_Tabs		10001
#define CID_OK			10002
#define CID_Cancel		10003
#define CID_MainGroup		10004

#define CID_Lookups		11000
#define CID_LookupTop		11001
#define CID_LookupUp		11002
#define CID_LookupDown		11003
#define CID_LookupBottom	11004
#define CID_AddLookup		11005
#define CID_AddSubtable		11006
#define CID_EditMetadata		11007
#define CID_EditSubtable	11008
#define CID_DeleteLookup	11009
#define CID_MergeLookup		11010
#define CID_RevertLookups	11011
#define CID_LookupSort		11012
#define CID_ImportLookups	11013
#define CID_LookupWin		11020		/* (GSUB, add 1 for GPOS) */
#define CID_LookupVSB		11022		/* (GSUB, add 1 for GPOS) */
#define CID_LookupHSB		11024		/* (GSUB, add 1 for GPOS) */
#define CID_SaveLookup		11026
#define CID_SaveFeat		11027
#define CID_AddAllAlternates	11028
#define CID_AddDFLT		11029

#define CID_MacAutomatic	16000
#define CID_MacStyles		16001
#define CID_MacFOND		16002

#define CID_Unicode		16100
#define CID_UnicodeEmpties	16101

const char *TTFNameIds(int id) {
    int i;

    FontInfoInit();
    for ( i=0; ttfnameids[i].text!=NULL; ++i )
	if ( ttfnameids[i].userdata == (void *) (intpt) id )
return( (char *) ttfnameids[i].text );

    if ( id==6 )
return( "Postscript" );

return( _("Unknown") );
}

const char *MSLangString(int language) {
    int i;

    FontInfoInit();
    for ( i=0; mslanguages[i].text!=NULL; ++i )
	if ( mslanguages[i].userdata == (void *) (intpt) language )
return( (char *) mslanguages[i].text );

    language &= 0xff;
    for ( i=0; mslanguages[i].text!=NULL; ++i )
	if ( ((intpt) mslanguages[i].userdata & 0xff) == language )
return( (char *) mslanguages[i].text );

return( _("Unknown") );
}


struct psdict *PSDictCopy(struct psdict *dict) {
    struct psdict *ret;
    int i;

    if ( dict==NULL )
return( NULL );

    ret = gcalloc(1,sizeof(struct psdict));
    ret->cnt = dict->cnt; ret->next = dict->next;
    ret->keys = gcalloc(ret->cnt,sizeof(char *));
    ret->values = gcalloc(ret->cnt,sizeof(char *));
    for ( i=0; i<dict->next; ++i ) {
	ret->keys[i] = copy(dict->keys[i]);
	ret->values[i] = copy(dict->values[i]);
    }

return( ret );
}

int PSDictFindEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( i );

return( -1 );
}

char *PSDictHasEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( NULL );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( dict->values[i] );

return( NULL );
}

int PSDictRemoveEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( false );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next )
return( false );
    free( dict->keys[i]);
    free( dict->values[i] );
    --dict->next;
    while ( i<dict->next ) {
	dict->keys[i] = dict->keys[i+1];
	dict->values[i] = dict->values[i+1];
	++i;
    }

return( true );
}

int PSDictChangeEntry(struct psdict *dict, char *key, char *newval) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next ) {
	if ( dict->next>=dict->cnt ) {
	    dict->cnt += 10;
	    dict->keys = grealloc(dict->keys,dict->cnt*sizeof(char *));
	    dict->values = grealloc(dict->values,dict->cnt*sizeof(char *));
	}
	dict->keys[dict->next] = copy(key);
	dict->values[dict->next] = NULL;
	++dict->next;
    }
    free(dict->values[i]);
    dict->values[i] = copy(newval);
return( i );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
/* These are Postscript names, and as such should not be translated */
enum { pt_number, pt_boolean, pt_array, pt_code };
static struct { const char *name; short type, arr_size, present; } KnownPrivates[] = {
    { "BlueValues", pt_array, 14 },
    { "OtherBlues", pt_array, 10 },
    { "BlueFuzz", pt_number },
    { "FamilyBlues", pt_array, 14 },
    { "FamilyOtherBlues", pt_array, 10 },
    { "BlueScale", pt_number },
    { "BlueShift", pt_number },
    { "StdHW", pt_array, 1 },
    { "StdVW", pt_array, 1 },
    { "StemSnapH", pt_array, 12 },
    { "StemSnapV", pt_array, 12 },
    { "ForceBold", pt_boolean },
    { "LanguageGroup", pt_number },
    { "RndStemUp", pt_number },
    { "lenIV", pt_number },
    { "ExpansionFactor", pt_number },
    { "Erode", pt_code },
/* I am deliberately not including Subrs and OtherSubrs */
/* The first could not be entered (because it's a set of binary strings) */
/* And the second has special meaning to us and must be handled with care */
    { NULL }
};

struct ask_data {
    int ret;
    int done;
};

static int Ask_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    struct ask_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int Ask_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    struct ask_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = d->ret = true;
    }
return( true );
}

static int ask_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct ask_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static char *AskKey(SplineFont *sf) {
    int i,j, cnt=0;
    GTextInfo *ti;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8], boxes[3], *varray[9], *harray[7];
    GTextInfo label[8];
    struct ask_data d;
    char *ret;
    int ptwidth;

    if ( sf->private==NULL )
	for ( i=0; KnownPrivates[i].name!=NULL; ++i ) {
	    KnownPrivates[i].present = 0;
	    ++cnt;
	}
    else {
	for ( i=0; KnownPrivates[i].name!=NULL; ++i ) {
	    for ( j=0; j<sf->private->next; ++j )
		if ( strcmp(KnownPrivates[i].name,sf->private->keys[j])==0 )
	    break;
	    if ( !(KnownPrivates[i].present = (j<sf->private->next)) )
		++cnt;
	}
    }
    if ( cnt==0 )
	ti = NULL;
    else {
	ti = gcalloc(cnt+1,sizeof(GTextInfo));
	for ( i=cnt=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( !KnownPrivates[i].present )
		ti[cnt++].text = uc_copy(KnownPrivates[i].name);
    }

    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Private Key");
    pos.x = pos.y = 0;
    ptwidth = 2*GIntGetResource(_NUM_Buttonsize)+GGadgetScale(60);
    pos.width =GDrawPointsToPixels(NULL,ptwidth);
    pos.height = GDrawPointsToPixels(NULL,90);
    gw = GDrawCreateTopWindow(NULL,&pos,ask_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _("Key (in Private dictionary)");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = 18; gcd[1].gd.pos.width = ptwidth-20;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].creator = GTextFieldCreate;
    if ( ti!=NULL ) {
	gcd[1].gd.u.list = ti;
	gcd[1].creator = GListFieldCreate;
    }
    varray[2] = &gcd[1]; varray[3] = NULL;
    varray[4] = GCD_Glue; varray[5] = NULL;

    gcd[2].gd.pos.x = 20-3; gcd[2].gd.pos.y = 90-35-3;
    gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[2].text = (unichar_t *) _("_OK");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = Ask_OK;
    gcd[2].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[2]; harray[2] = GCD_Glue;

    gcd[3].gd.pos.x = -20; gcd[3].gd.pos.y = 90-35;
    gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _("_Cancel");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = Ask_Cancel;
    gcd[3].creator = GButtonCreate;
    harray[3] = GCD_Glue; harray[4] = &gcd[3]; harray[5] = GCD_Glue;
    harray[6] = NULL;
    varray[6] = &boxes[2]; varray[7] = NULL;
    varray[8] = NULL;

    gcd[4].gd.pos.x = 2; gcd[4].gd.pos.y = 2;
    gcd[4].gd.pos.width = pos.width-4; gcd[4].gd.pos.height = pos.height-2;
    gcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[4].creator = GGroupCreate;

    memset(boxes,0,sizeof(boxes));
    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    ret = NULL;
    if ( d.ret )
	ret = cu_copy(_GGadgetGetTitle(gcd[1].ret));
    GTextInfoListFree(ti);
    GDrawDestroyWindow(gw);
return( ret );
}

static GTextInfo *PI_ListSet(SplineFont *sf) {
    GTextInfo *ti = gcalloc((sf->private==NULL?0:sf->private->next)+1,sizeof( GTextInfo ));
    int i=0;

    if ( sf->private!=NULL ) {
	for ( i=0; i<sf->private->next; ++i ) {
	    ti[i].text = uc_copy(sf->private->keys[i]);
	}
    }
    if ( i!=0 )
	ti[0].selected = true;
return( ti );
}

static GTextInfo **PI_ListArray(struct psdict *private) {
    GTextInfo **ti = gcalloc((private==NULL?0:private->next)+1,sizeof( GTextInfo *));
    int i=0;

    if ( private!=NULL ) {
	for ( i=0; i<private->next; ++i ) {
	    ti[i] = gcalloc(1,sizeof(GTextInfo));
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	    ti[i]->text = uc_copy(private->keys[i]);
	}
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
    if ( i!=0 )
	ti[0]->selected = true;
return( ti );
}

static void PIPrivateCheck(struct gfi_data *d) {
    if ( d->private==NULL ) {
	if ( d->sf->private==NULL ) {
	    d->private = gcalloc(1,sizeof(struct psdict));
	    d->private->cnt = 10;
	    d->private->keys = gcalloc(10,sizeof(char *));
	    d->private->values = gcalloc(10,sizeof(char *));
	} else
	    d->private = PSDictCopy(d->sf->private);
    }
}

static int PIFinishFormer(struct gfi_data *d) {
    unichar_t *end;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif

    if ( d->old_sel < 0 )
return( true );
    if ( d->private==NULL && d->sf->private!=NULL ) {
	const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_PrivateValues));
	if ( uc_strcmp(val,d->sf->private->values[d->old_sel])==0 )
return( true );			/* Didn't change */
	PIPrivateCheck(d);
    }
    if ( d->private!=NULL && d->old_sel>=0 && d->old_sel!=d->private->next ) {
	const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_PrivateValues));
	const unichar_t *pt = val;
	int i;

	/* does the type appear reasonable? */
	while ( isspace(*pt)) ++pt;
	for ( i=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( strcmp(KnownPrivates[i].name,d->private->keys[d->old_sel])==0 )
	break;
	if ( KnownPrivates[i].name!=NULL ) {
	    if ( KnownPrivates[i].type==pt_array ) {
		if ( *pt!='[' && gwwv_ask(_("Bad type"),(const char **) buts,0,1,_("Expected array.\nProceed anyway?"))==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_boolean ) {
		if ( uc_strcmp(pt,"true")!=0 && uc_strcmp(pt,"false")!=0 &&
			gwwv_ask(_("Bad type"),(const char **) buts,0,1,_("Expected boolean.\nProceed anyway?"))==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_code ) {
		if ( *pt!='{' && gwwv_ask(_("Bad type"),(const char **) buts,0,1,_("Expected code.\nProceed anyway?"))==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_number ) {
		u_strtod(pt,&end);
		while ( isspace(*end)) ++end;
		if ( *end!='\0' && gwwv_ask(_("Bad type"),(const char **) buts,0,1,_("Expected number.\nProceed anyway?"))==1 )
return( false );
	    }
	}

	/* Ok then set it */
	free(d->private->values[d->old_sel]);
	d->private->values[d->old_sel] = cu_copy(val);
	d->old_sel = -1;
    }
return( true );
}

static void ProcessListSel(struct gfi_data *d) {
    GGadget *list = GWidgetGetControl(d->gw,CID_PrivateEntries);
    int sel = GGadgetGetFirstListSelectedItem(list);
    unichar_t *temp;
    static const unichar_t nullstr[] = { 0 };
    SplineFont *sf = d->sf;
    struct psdict *private;

    if ( d->old_sel==sel )
return;

    if ( !PIFinishFormer(d)) {
	/*GGadgetSelectListItem(list,sel,false);*/
	GGadgetSelectListItem(list,d->old_sel,true);
return;
    }
    private = d->private ? d->private : sf->private;
    if ( sel==-1 ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),false);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),nullstr);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),true);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 ||
		strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 ||
		strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 ) {
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),true);
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),true);
	} else {
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),false);
	}
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),true);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( private->values[sel]));
	free( temp );
	GTextFieldShow(GWidgetGetControl(d->gw,CID_PrivateValues),0);
    }
    d->old_sel = sel;
}

static int PI_Add(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int i;
    char *newkey;
    GTextInfo **ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	if ( !PIFinishFormer(d))
return(true);
	newkey = AskKey(d->sf);
	if ( newkey==NULL )
return( true );
	PIPrivateCheck(d);
	if (( i = PSDictFindEntry(d->private,newkey))==-1 )
	    i = PSDictChangeEntry(d->private,newkey,"");
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	ti = PI_ListArray(d->private);
	if ( i>0 ) {
	    ti[0]->selected = false;
	    ti[i]->selected = true;
	}
	GGadgetSetList(list,ti,false);
	d->old_sel = -1;
	ProcessListSel(d);
	free(newkey);
    }
return( true );
}

static void arraystring(char *buffer,real *array,int cnt) {
    int i, ei;

    for ( ei=cnt; ei>1 && array[ei-1]==0; --ei );
    *buffer++ = '[';
    for ( i=0; i<ei; ++i ) {
	sprintf(buffer, "%d ", (int) array[i]);
	buffer += strlen(buffer);
    }
    if ( buffer[-1] ==' ' ) --buffer;
    *buffer++ = ']'; *buffer='\0';
}

static void SnapSet(struct psdict *private,real stemsnap[12], real snapcnt[12],
	char *name1, char *name2 ) {
    int i, mi;
    char buffer[211];

    mi = -1;
    for ( i=0; stemsnap[i]!=0 && i<12; ++i )
	if ( mi==-1 ) mi = i;
	else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
    if ( mi==-1 )
return;
    sprintf( buffer, "[%d]", (int) stemsnap[mi]);
    PSDictChangeEntry(private,name1,buffer);
    arraystring(buffer,stemsnap,12);
    PSDictChangeEntry(private,name2,buffer);
}

static int PI_Guess(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnap[12];
    char buffer[211];
    unichar_t *temp;
    struct psdict *private;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
	private = d->private ? d->private : sf->private;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 ) {
	    if ( gwwv_ask(_("Guess"),(const char **) buts,0,1,_("This will change both BlueValues and OtherBlues.\nDo you want to continue?"))==1 )
return( true );
	    PIPrivateCheck(d);
	    private = d->private;
	    FindBlues(sf,bluevalues,otherblues);
	    arraystring(buffer,bluevalues,14);
	    PSDictChangeEntry(private,"BlueValues",buffer);
	    if ( otherblues[0]!=0 || otherblues[1]!=0 ) {
		arraystring(buffer,otherblues,10);
		PSDictChangeEntry(private,"OtherBlues",buffer);
	    }
	} else if ( strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 ) {
	    if ( gwwv_ask(_("Guess"),(const char **) buts,0,1,_("This will change both StdHW and StemSnapH.\nDo you want to continue?"))==1 )
return( true );
	    FindHStems(sf,stemsnap,snapcnt);
	    PIPrivateCheck(d);
	    SnapSet(d->private,stemsnap,snapcnt,"StdHW","StemSnapH");
	} else if ( strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 ) {
	    if ( gwwv_ask(_("Guess"),(const char **) buts,0,1,_("This will change both StdVW and StemSnapV.\nDo you want to continue?"))==1 )
return( true );
	    FindVStems(sf,stemsnap,snapcnt);
	    PIPrivateCheck(d);
	    SnapSet(d->private,stemsnap,snapcnt,"StdVW","StemSnapV");
	}
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( d->private->values[sel]));
	free( temp );
    }
return( true );
}

static int PI_Hist(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    struct psdict *private;
    enum hist_type h;
    unichar_t *temp;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
	PIPrivateCheck(d);
	private = d->private ? d->private : sf->private;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 )
	    h = hist_blues;
	else if ( strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 )
	    h = hist_hstem;
	else if ( strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 )
	    h = hist_vstem;
	else
return( true );		/* can't happen */
	SFHistogram(sf,private,NULL,NULL,h);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( d->private->values[sel]));
	free( temp );
    }
return( true );
}

static int PI_Delete(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    GTextInfo **ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	PIPrivateCheck(d);
	sf = d->sf;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	PSDictRemoveEntry(d->private, d->private->keys[sel]);
	sf->changed = true;
	ti = PI_ListArray(d->private);
	--sel;
	if ( d->private!=NULL && sel>=d->private->next )
	    sel = d->private->next-1;
	if ( sel>0 ) {
	    ti[0]->selected = false;
	    ti[sel]->selected = true;
	}
	GGadgetSetList(list,ti,false);
	d->old_sel = -2;
	ProcessListSel(d);
    }
return( true );
}

static int PI_ListSel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	ProcessListSel(GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

/* Use URW 4 letter abbreviations */
static char *knownweights[] = { "Demi", "Bold", "Regu", "Medi", "Book", "Thin",
	"Ligh", "Heav", "Blac", "Ultr", "Nord", "Norm", "Gras", "Stan", "Halb",
	"Fett", "Mage", "Mitt", "Buch", NULL };
static char *realweights[] = { "Demi", "Bold", "Regular", "Medium", "Book", "Thin",
	"Light", "Heavy", "Black", "Ultra", "Nord", "Normal", "Gras", "Standard", "Halbfett",
	"Fett", "Mager", "Mittel", "Buchschrift", NULL};
#ifndef LUA_FF_LIB
static char *moreweights[] = { "ExtraLight", "VeryLight", NULL };
static char **noticeweights[] = { moreweights, realweights, knownweights, NULL };
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int GFI_NameChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *gfi = GDrawGetUserData(gw);
	const unichar_t *uname = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Fontname));
	unichar_t ubuf[50];
	int i,j;
	for ( j=0; noticeweights[j]!=NULL; ++j ) {
	    for ( i=0; noticeweights[j][i]!=NULL; ++i ) {
		if ( uc_strstrmatch(uname,noticeweights[j][i])!=NULL ) {
		    uc_strcpy(ubuf, noticeweights[j]==knownweights ?
			    realweights[i] : noticeweights[j][i]);
		    GGadgetSetTitle(GWidgetGetControl(gw,CID_Weight),ubuf);
	    break;
		}
	    }
	    if ( noticeweights[j][i]!=NULL )
	break;
	}
	if ( gfi->human_untitled )
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Human),uname);
	if ( gfi->family_untitled ) {
	    const unichar_t *ept = uname+u_strlen(uname); unichar_t *temp;
	    for ( i=0; knownweights[i]!=NULL; ++i ) {
		if (( temp = uc_strstrmatch(uname,knownweights[i]))!=NULL && temp<ept && temp!=uname )
		    ept = temp;
	    }
	    if (( temp = uc_strstrmatch(uname,"ital"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"obli"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"kurs"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"slanted"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = u_strchr(uname,'-'))!=NULL && temp!=uname )
		ept = temp;
	    temp = u_copyn(uname,ept-uname);
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Family),temp);
	}
    }
return( true );
}

static int GFI_FamilyChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	gfi->family_untitled = false;
    }
return( true );
}

static int GFI_DefBaseChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	GGadgetSetChecked(GWidgetGetControl(gfi->gw,*_GGadgetGetTitle(g)!='\0'?CID_HasDefBase:CID_SameAsFontname),
		true);
    }
return( true );
}

static int GFI_HumanChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	gfi->human_untitled = false;
    }
return( true );
}

static int GFI_VMetricsCheck(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	const unichar_t *vo = _GGadgetGetTitle(GWidgetGetControl(gw,CID_VOrigin));
	int checked = GGadgetIsChecked(g);
	if ( checked && *vo=='\0' ) {
	    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	    char space[10]; unichar_t uspace[10];
	    sprintf( space, "%d", d->sf->ascent );
	    uc_strcpy(uspace,space);
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_VOrigin),uspace);
	}
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_VOrigin),checked);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_VOriginLab),checked);
	GGadgetSetEnabled(GWidgetGetControl(GDrawGetParentWindow(gw),CID_VLineGap),checked);
	GGadgetSetEnabled(GWidgetGetControl(GDrawGetParentWindow(gw),CID_VLineGapLab),checked);
    }
return( true );
}

static int GFI_EmChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	char buf[20]; unichar_t ubuf[20];
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(g); unichar_t *end;
	int val = u_strtol(ret,&end,10), ascent, descent;
	if ( *end )
return( true );
	switch ( GGadgetGetCid(g)) {
	  case CID_Em:
	    ascent = rint( ((double) val)*d->sf->ascent/(d->sf->ascent+d->sf->descent) );
	    descent = val - ascent;
	  break;
	  case CID_Ascent:
	    ascent = val;
	    ret = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	    descent = u_strtol(ret,&end,10);
	    if ( *end )
return( true );
	  break;
	  case CID_Descent:
	    descent = val;
	    ret = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	    ascent = u_strtol(ret,&end,10);
	    if ( *end )
return( true );
	  break;
	}
	sprintf( buf, "%d", ascent ); if ( ascent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Ascent),ubuf);
	sprintf( buf, "%d", descent ); if ( descent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Descent),ubuf);
	sprintf( buf, "%d", ascent+descent ); if ( ascent+descent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Em),ubuf);
    }
return( true );
}

static int GFI_GuessItalic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	double val = SFGuessItalicAngle(d->sf);
	char buf[30]; unichar_t ubuf[30];
	sprintf( buf, "%.1f", val);
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_ItalicAngle),ubuf);
    }
return( true );
}

static void MCD_Close(struct markclassdlg *mcd);

static void GFI_Close(struct gfi_data *d) {
    FontView *fvs;
    SplineFont *sf = d->sf;

    if ( d->ccd )
	CCD_Close(d->ccd);
    if ( d->smd )
	SMD_Close(d->smd);
    if ( d->mcd )
	MCD_Close(d->mcd );

    PSDictFree(d->private);

    GDrawDestroyWindow(d->gw);
    if ( d->sf->fontinfo == d )
	d->sf->fontinfo = NULL;
    for ( fvs = d->sf->fv; fvs!=NULL; fvs = fvs->nextsame ) {
	GDrawRequestExpose(sf->fv->v,NULL,false);
    }
    d->done = true;
    /* d will be freed by destroy event */;
}

static void GFI_CancelClose(struct gfi_data *d) {
    int isgpos,i,j;

    MacFeatListFree(GGadgetGetUserData((GWidgetGetControl(
	    d->gw,CID_Features))));
    MarkClassFree(d->mark_class_cnt,d->mark_classes,d->mark_class_names);
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	struct lkdata *lk = &d->tables[isgpos];
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].new )
		SFRemoveLookup(d->sf,lk->all[i].lookup);
	    else for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].new )
		    SFRemoveLookupSubTable(d->sf,lk->all[i].subtables[j].subtable);
	    }
	    free(lk->all[i].subtables);
	}
	free(lk->all);
    }
    GFI_Close(d);
}

static GTextInfo *MarkClassesList(SplineFont *sf) {
    int cnt;
    GTextInfo *ti;

    if ( sf->mark_class_cnt==0 )
return( NULL );

    ti = gcalloc(sf->mark_class_cnt+1,sizeof(GTextInfo));
    for ( cnt=1; cnt<sf->mark_class_cnt; ++cnt ) {
	ti[cnt-1].text = uc_copy(sf->mark_class_names[cnt]);
	ti[cnt-1].fg = ti[cnt-1].bg = COLOR_DEFAULT;
    }
return( ti );
}

#define CID_MCD_Name		1001
#define CID_MCD_Set		1002
#define CID_MCD_Select		1003
#define CID_MCD_GlyphList	1004

#define MCD_Width	250
#define MCD_Height	210

typedef struct markclassdlg {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int which;
} MarkClassDlg;

static void MCD_Close(MarkClassDlg *mcd) {
    mcd->d->mcd = NULL;
    GDrawDestroyWindow(mcd->gw);
    free(mcd);
}

static void MCD_DoCancel(MarkClassDlg *mcd) {
    MCD_Close(mcd);
}

static int MCD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MarkClassDlg *mcd = GDrawGetUserData(GGadgetGetWindow(g));
	MCD_DoCancel(mcd);
    }
return( true );
}

static int MCD_InvalidClassList(const char *ret,char **classes, char **names,
	int nclass, int which) {
    const char *pt, *end;
    char *tpt, *tend;
    int i;

    for ( pt = ret; *pt; pt = end ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	end = strchr(pt,' ');
	if ( end==NULL ) end = pt+strlen(pt);
	for ( i=1; (i < nclass) && (classes[i]!=NULL); ++i ) {
	    if ( which==i )
	continue;
	    for ( tpt=classes[i]; *tpt; tpt = tend ) {
		while ( *tpt==' ' ) ++tpt;
		tend = strchr(tpt,' ');
		if ( tend==NULL ) tend = tpt+strlen(tpt);
		if ( tend-tpt==end-pt && strncmp(pt,tpt,end-pt)==0 ) {
		    char *dupname = copyn(pt,end-pt);
		    gwwv_post_error(_("Bad Class"),_("No glyphs from another class may appear here, but %.30s appears here and in class %.30s"), dupname, names[i]);
		    free(dupname);
return( true );
		}
	    }
	}
    }
return( false );
}

static int MCD_OK(GGadget *g, GEvent *e) {
    char *newname;
    char *glyphs;
    struct gfi_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MarkClassDlg *mcd = GDrawGetUserData(GGadgetGetWindow(g));
	newname = GGadgetGetTitle8(GWidgetGetControl(mcd->gw,CID_MCD_Name));
	glyphs = GGadgetGetTitle8(GWidgetGetControl(mcd->gw,CID_MCD_GlyphList));
	d = mcd->d;

	if ( !CCD_NameListCheck(d->sf,glyphs,true,_("Bad Class")) ||
		MCD_InvalidClassList(glyphs,d->mark_classes,d->mark_class_names,
			d->mark_class_cnt,mcd->which )) {
	    free(newname); free(glyphs);
return( true );
	}

	if ( mcd->which==-1 ) {		/* New */
	    
	    if ( d->mark_class_cnt==0 ) {
		d->mark_class_cnt = 2;		/* Class 0 is magic */
		d->mark_classes = gcalloc(2,sizeof(char *));
		d->mark_class_names = gcalloc(2,sizeof(unichar_t *));
	    } else {
		++d->mark_class_cnt;
		d->mark_classes = grealloc(d->mark_classes,d->mark_class_cnt*sizeof(char*));
		d->mark_class_names = grealloc(d->mark_class_names,d->mark_class_cnt*sizeof(unichar_t*));
	    }
	    d->mark_classes[d->mark_class_cnt-1] = copy(glyphs);
	    d->mark_class_names[d->mark_class_cnt-1] = copy(newname);
	    GListAppendLine8(mcd->list,newname,false);
	} else {
	    free(d->mark_classes[mcd->which]); d->mark_classes[mcd->which] = copy(glyphs);
	    free(d->mark_class_names[mcd->which]); d->mark_class_names[mcd->which] = copy(newname);
	    GListChangeLine8(mcd->list,mcd->which,newname);
	}
	MCD_Close(mcd);
	free(newname); free(glyphs);
    }
return( true );
}

static int MCD_ToSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MarkClassDlg *mcd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(mcd->gw,CID_MCD_GlyphList));
	SplineFont *sf = mcd->d->sf;
	FontView *fv = sf->fv;
	const unichar_t *end;
	int pos, found=-1;
	char *nm;

	GDrawSetVisible(fv->gw,true);
	GDrawRaise(fv->gw);
	memset(fv->selected,0,fv->map->enccount);
	while ( *ret ) {
	    end = u_strchr(ret,' ');
	    if ( end==NULL ) end = ret+u_strlen(ret);
	    nm = cu_copybetween(ret,end);
	    for ( ret = end; isspace(*ret); ++ret);
	    if (( pos = SFFindSlot(sf,fv->map,-1,nm))!=-1 ) {
		if ( found==-1 ) found = pos;
		if ( pos!=-1 )
		    fv->selected[pos] = true;
	    }
	    free(nm);
	}

	if ( found!=-1 )
	    FVScrollToChar(fv,found);
	GDrawRequestExpose(fv->v,NULL,false);
    }
return( true );
}

static int MCD_FromSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MarkClassDlg *mcd = GDrawGetUserData(GGadgetGetWindow(g));
	SplineFont *sf = mcd->d->sf;
	FontView *fv = sf->fv;
	unichar_t *vals, *pt;
	int i, len, max, gid;
	SplineChar *sc, dummy;
    
	for ( i=len=max=0; i<fv->map->enccount; ++i ) if ( fv->selected[i]) {
	    gid = fv->map->map[i];
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		sc = sf->glyphs[gid];
	    else
		sc = SCBuildDummy(&dummy,sf,fv->map,i);
	    len += strlen(sc->name)+1;
	    if ( fv->selected[i]>max ) max = fv->selected[i];
	}
	pt = vals = galloc((len+1)*sizeof(unichar_t));
	*pt = '\0';
	/* in a class the order of selection is irrelevant */
	for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i]) {
	    gid = fv->map->map[i];
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		sc = sf->glyphs[gid];
	    else
		sc = SCBuildDummy(&dummy,sf,fv->map,i);
	    uc_strcpy(pt,sc->name);
	    pt += u_strlen(pt);
	    *pt++ = ' ';
	}
	if ( pt>vals ) pt[-1]='\0';
    
	GGadgetSetTitle(GWidgetGetControl(mcd->gw,CID_MCD_GlyphList),vals);
	free(vals);
    }
return( true );
}

void DropChars2Text(GWindow gw, GGadget *glyphs,GEvent *event) {
    char *cnames;
    const unichar_t *old;
    unichar_t *new;
    int32 len;

    if ( !GDrawSelectionHasType(gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    old = _GGadgetGetTitle(glyphs);
    if ( old==NULL || *old=='\0' ) {
	new = uc_copy(cnames);
    } else {
	new = galloc(strlen(cnames)+u_strlen(old)+5);
	u_strcpy(new,old);
	if ( new[u_strlen(new)-1]!=' ' )
	    uc_strcat(new," ");
	uc_strcat(new,cnames);
    }
    GGadgetSetTitle(glyphs,new);
    free( cnames );
    free( new );
}

static void MCD_Drop(MarkClassDlg *mcd,GEvent *event) {
    DropChars2Text(mcd->gw,GWidgetGetControl(mcd->gw,CID_MCD_GlyphList),event);
}

static int mcd_e_h(GWindow gw, GEvent *event) {
    MarkClassDlg *mcd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	MCD_DoCancel(mcd);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("fontinfo.html#MarkClass");
return( true );
	}
return( false );
      break;
      case et_drop:
	MCD_Drop(mcd,event);
      break;
    }
return( true );
}

static void CreateMarkClassDlg(struct gfi_data *d, GGadget *list, int which) {
    MarkClassDlg *mcd;
    GWindow gw;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[10];
    int k;
    unichar_t *freeme = NULL;

    if ( d->mcd!=NULL ) {
	GDrawSetVisible(d->mcd->gw,true);
	GDrawRaise(d->mcd->gw);
return;
    }

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    mcd = gcalloc(1,sizeof(MarkClassDlg));
    mcd->d = d; mcd->list = list; mcd->which = which;

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Mark Classes");
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,MCD_Width));
    pos.height = GDrawPointsToPixels(NULL,MCD_Height);
    mcd->gw = gw = GDrawCreateTopWindow(NULL,&pos,mcd_e_h,mcd,&wattrs);

    k = 0;

    label[k].text = (unichar_t *) _("Class Name:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 10;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    if ( which!=-1 ) {
	gcd[k].gd.label = &label[k];
	label[k].text = (unichar_t *) d->mark_class_names[which];
	label[k].text_is_1byte = true;
    }
    gcd[k].gd.pos.x = 70; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_MCD_Name;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Set From Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+28;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set this glyph list to be the glyphs selected in the fontview");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = MCD_FromSelection;
    gcd[k].gd.cid = CID_MCD_Set;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Select In Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the fontview's selection to be the glyphs named here");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = MCD_ToSelection;
    gcd[k].gd.cid = CID_MCD_Select;
    gcd[k++].creator = GButtonCreate;

    if ( which!=-1 ) {
	gcd[k].gd.label = &label[k];
	label[k].text = freeme = uc_copy(d->mark_classes[which]);
    }
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.pos.width = MCD_Width-20; gcd[k].gd.pos.height = 8*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;	/* Just ASCII text for glyph names, no need for xim */
    gcd[k].gd.cid = CID_MCD_GlyphList;
    gcd[k++].creator = GTextAreaCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = MCD_Height-30-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = MCD_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30+3; gcd[k].gd.pos.y = MCD_Height-30;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = MCD_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(mcd->gw,gcd);
    GDrawSetVisible(mcd->gw,true);

    free(freeme);
}

static int GFI_MarkNew(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	CreateMarkClassDlg(d, GWidgetGetControl(GGadgetGetWindow(g),CID_MarkClasses), -1);
    }
return( true );
}

static int GFI_MarkEdit(GGadget *g, GEvent *e) {
    int i;
    GGadget *list;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_MarkClasses);
	if ( (i = GGadgetGetFirstListSelectedItem(list))==-1 && i+1 < d->mark_class_cnt )
return( true );
	CreateMarkClassDlg(d, list, i+1);
    }
return( true );
}

static int GFI_MarkSelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MarkEdit),sel!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	GFI_MarkEdit(g,e);
    }
return( true );
}

static char *OtfNameToText(int lang, const char *name) {
    const char *langname;
    char *text;
    int i;

    for ( i=sizeof(mslanguages)/sizeof(mslanguages[0])-1; i>=0 ; --i )
	if ( mslanguages[i].userdata == (void *) (intpt) lang )
    break;
    if ( i==-1 )
	for ( i=sizeof(mslanguages)/sizeof(mslanguages[0])-1; i>=0 ; --i )
	    if ( ((intpt) mslanguages[i].userdata&0xff) == (lang&0xff) )
	break;
    if ( i==-1 )
	langname = "";
    else
	langname = (char*) (mslanguages[i].text);

    text = galloc((strlen(langname)+strlen(name)+4));
    strcpy(text,name);
    strcat(text," | ");
    strcat(text,langname);
return( text );
}

static GTextInfo **StyleNames(struct otfname *otfn) {
    int cnt;
    struct otfname *on;
    GTextInfo **tis;
    char *cname;

    for ( cnt=0, on=otfn; on!=NULL; on=on->next )
	++cnt;
    tis = galloc((cnt+1)*sizeof(GTextInfo *));
    for ( cnt=0, on=otfn; on!=NULL; on=on->next, ++cnt ) {
	tis[cnt] = gcalloc(1,sizeof(GTextInfo));
	tis[cnt]->fg = tis[cnt]->bg = COLOR_DEFAULT;
	tis[cnt]->userdata = (void *) (intpt) otfn->lang;
	cname = OtfNameToText(on->lang,on->name);
	tis[cnt]->text = utf82u_copy(cname);
	free(cname);
    }
    tis[cnt] = gcalloc(1,sizeof(GTextInfo));
return( tis );
}

static struct otfname *OtfNameFromStyleNames(GGadget *list) {
    int32 len; int i;
    GTextInfo **old = GGadgetGetList(list,&len);
    struct otfname *head=NULL, *last, *cur;
    unichar_t *pt, *temp;

    for ( i=0; i<len; ++i ) {
	cur = chunkalloc(sizeof(struct otfname));
	cur->lang = (intpt) old[i]->userdata;
	pt = uc_strstr(old[i]->text," | ");
	temp = u_copyn(old[i]->text,pt-old[i]->text);
	cur->name = u2utf8_copy(temp);
	free(temp);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

static int sn_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	int *d = GDrawGetUserData(gw);
	*d = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	int *d = GDrawGetUserData(gw);
	*d = GGadgetGetCid(event->u.control.g)+1;
    }
return( true );
}

static void AskForLangName(GGadget *list,int sel) {
    int32 len; int i;
    GTextInfo **old = GGadgetGetList(list,&len);
    unichar_t *name, *pt;
    char *cname;
    int lang_index;
    GGadgetCreateData gcd[7];
    GTextInfo label[5];
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    int done = 0;
    int k;
    GTextInfo **ti;
    char *temp;

    for ( i=sizeof(mslanguages)/sizeof(mslanguages[0])-1; i>=0 ; --i )
	mslanguages[i].fg = mslanguages[i].bg = COLOR_DEFAULT;
    if ( sel==-1 ) {
	for ( i=0; i<len; ++i )
	    if ( old[i]->userdata == (void *) 0x409 )
	break;
	if ( i==len ) {
	    for ( i=sizeof(mslanguages)/sizeof(mslanguages[0])-1; i>=0 ; --i )
		if ( mslanguages[i].userdata == (void *) 0x409 )
	    break;
	    lang_index = i;
	} else {
	    for ( lang_index=sizeof(mslanguages)/sizeof(mslanguages[0])-1; lang_index>=0 ; --lang_index ) {
		for ( i=0; i<len; ++i )
		    if ( mslanguages[lang_index].userdata == old[i]->userdata )
		break;
		if ( i==len )
	    break;
	    }
	}
	if ( lang_index < 0 )
	    lang_index = 0;
	name = uc_copy("");
    } else {
	for ( lang_index=sizeof(mslanguages)/sizeof(mslanguages[0])-1; lang_index>=0 ; --lang_index )
	    if ( mslanguages[lang_index].userdata == old[sel]->userdata )
	break;
	if ( lang_index < 0 )
	    lang_index = 0;
	pt = uc_strstr(old[sel]->text," | ");
	name = u_copyn(old[sel]->text,pt-old[sel]->text);
    }

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 7;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
    gcd[0].gd.cid = CID_Language;
    gcd[0].gd.u.list = mslanguages;
    gcd[0].creator = GListButtonCreate;
    for ( i=0; mslanguages[i].text!=NULL; ++i )
	mslanguages[i].selected = false;
    mslanguages[lang_index].selected = true;

    k = 1;
    label[k].text = (unichar_t *) _("_Name:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = name;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 50; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 122;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_StyleName;
    gcd[k++].creator = GTextFieldCreate;

    gcd[k].gd.pos.x = 25-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = true;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -25; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = false;
    gcd[k++].creator = GButtonCreate;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Style Name:");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(180));
    pos.height = GDrawPointsToPixels(NULL,2*26+45);
    gw = GDrawCreateTopWindow(NULL,&pos,sn_e_h,&done,&wattrs);

    GGadgetsCreate(gw,gcd);
    free(name);
    ti = GGadgetGetList(gcd[0].ret,&len);
    for ( i=0; i<len; ++i )
	if ( ti[i]->userdata == mslanguages[lang_index].userdata ) {
	    GGadgetSelectOneListItem(gcd[0].ret,i);
    break;
	}
    GDrawSetVisible(gw,true);

    while ( !done )
	GDrawProcessOneEvent(NULL);

    if ( done==2 ) {
	lang_index = GGadgetGetFirstListSelectedItem(gcd[0].ret);
	cname = OtfNameToText((intpt) ti[lang_index]->userdata,
		(temp = GGadgetGetTitle8(GWidgetGetControl(gw,CID_StyleName))));
	free(temp);
	if ( sel==-1 )
	    GListAppendLine8(list,cname,false)->userdata =
		    ti[lang_index]->userdata;
	else
	    GListChangeLine8(list,sel,cname)->userdata =
		    ti[lang_index]->userdata;
	free(name);
    }

    GDrawDestroyWindow(gw);
}

static int GFI_StyleNameNew(GGadget *g, GEvent *e) {
    GGadget *list;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_StyleName);
	AskForLangName(list,-1);
    }
return( true );
}

static int GFI_StyleNameDel(GGadget *g, GEvent *e) {
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_StyleName);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_StyleNameDel),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_StyleNameRename),false);
    }
return( true );
}

static int GFI_StyleNameRename(GGadget *g, GEvent *e) {
    GGadget *list;
    int sel;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_StyleName);
	if ( (sel=GGadgetGetFirstListSelectedItem(list))==-1 )
return( true );
	AskForLangName(list,sel);
    }
return( true );
}

static int GFI_StyleNameSelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_StyleNameDel),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_StyleNameRename),sel!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	GFI_StyleNameRename(g,e);
    }
return( true );
}

void GListMoveSelected(GGadget *list,int offset) {
    int32 len; int i,j;
    GTextInfo **old, **new;

    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    j = (offset<0 ) ? 0 : len-1;
    for ( i=0; i<len; ++i ) if ( old[i]->selected ) {
	if ( offset==0x80000000 || offset==0x7fffffff )
	    /* Do Nothing */;
	else if ( offset<0 ) {
	    if ( (j= i+offset)<0 ) j=0;
	    while ( new[j] ) ++j;
	} else {
	    if ( (j= i+offset)>=len ) j=len-1;
	    while ( new[j] ) --j;
	}
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	if ( offset<0 ) ++j; else --j;
    }
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	while ( new[j] ) ++j;
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	++j;
    }
    new[len] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

void GListDelSelected(GGadget *list) {
    int32 len; int i,j;
    GTextInfo **old, **new;

    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	++j;
    }
    new[j] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

GTextInfo *GListChangeLine(GGadget *list,int pos, const unichar_t *line) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	if ( i!=pos )
	    new[i]->text = u_copy(new[i]->text);
	else
	    new[i]->text = u_copy(line);
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,pos);
return( new[pos]);
}

GTextInfo *GListAppendLine(GGadget *list,const unichar_t *line,int select) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+2,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	new[i]->text = u_copy(new[i]->text);
	if ( select ) new[i]->selected = false;
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
    new[i]->userdata = NULL;
    new[i]->text = u_copy(line);
    new[i]->selected = select;
    new[i+1] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,i);
return( new[i]);
}

GTextInfo *GListChangeLine8(GGadget *list,int pos, const char *line) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	if ( i!=pos )
	    new[i]->text = u_copy(new[i]->text);
	else
	    new[i]->text = utf82u_copy(line);
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,pos);
return( new[pos]);
}

GTextInfo *GListAppendLine8(GGadget *list,const char *line,int select) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+2,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	new[i]->text = u_copy(new[i]->text);
	if ( select ) new[i]->selected = false;
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
    new[i]->userdata = NULL;
    new[i]->text = utf82u_copy(line);
    new[i]->selected = select;
    new[i+1] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,i);
return( new[i]);
}

static int GFI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GFI_CancelClose(d);
    }
return( true );
}

static int AskLoseUndoes() {
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif
return( gwwv_ask(_("Losing Undoes"),(const char **) buts,0,1,_("Changing the order of the splines in the font will lose all undoes.\nContinue anyway?")) );
}

static void BadFamily() {
    gwwv_post_error(_("Bad Family Name"),_("Bad Family Name, must begin with an alphabetic character."));
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static char *modifierlist[] = { "Ital", "Obli", "Kursive", "Cursive", "Slanted",
	"Expa", "Cond", NULL };
static char *modifierlistfull[] = { "Italic", "Oblique", "Kursive", "Cursive", "Slanted",
    "Expanded", "Condensed", NULL };
static char **mods[] = { knownweights, modifierlist, NULL };
static char **fullmods[] = { realweights, modifierlistfull, NULL };

char *_GetModifiers(char *fontname, char *familyname,char *weight) {
    char *pt, *fpt;
    int i, j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt == NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }
    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( strcmp(fpt,mods[i][j])==0 )
return( fullmods[i][j]);
	}
	if ( strcmp(fpt,"BoldItal")==0 )
return( "BoldItalic" );
	else if ( strcmp(fpt,"BoldObli")==0 )
return( "BoldOblique" );

return( fpt );
    }

return( weight==NULL || *weight=='\0' ? "Regular": weight );
}

char *SFGetModifiers(SplineFont *sf) {
return( _GetModifiers(sf->fontname,sf->familyname,sf->weight));
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static const unichar_t *_uGetModifiers(const unichar_t *fontname, const unichar_t *familyname,
	const unichar_t *weight) {
    const unichar_t *pt, *fpt;
    static unichar_t regular[] = { 'R','e','g','u','l','a','r', 0 };
    static unichar_t space[20];
    int i,j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=u_strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt==NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = uc_strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }

    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( uc_strcmp(fpt,mods[i][j])==0 ) {
		uc_strcpy(space,fullmods[i][j]);
return( space );
	    }
	}
	if ( uc_strcmp(fpt,"BoldItal")==0 ) {
	    uc_strcpy(space,"BoldItalic");
return( space );
	} else if ( uc_strcmp(fpt,"BoldObli")==0 ) {
	    uc_strcpy(space,"BoldOblique");
return( space );
	}
return( fpt );
    }

return( weight==NULL || *weight=='\0' ? regular: weight );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void SFSetFontName(SplineFont *sf, char *family, char *mods,char *full) {
    char *n;
    char *pt, *tpt;

    n = galloc(strlen(family)+strlen(mods)+2);
    strcpy(n,family); strcat(n," "); strcat(n,mods);
    if ( full==NULL || *full == '\0' )
	full = copy(n);
    for ( pt=tpt=n; *pt; ) {
	if ( !Isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
#if 0
    for ( pt=tpt=family; *pt; ) {
	if ( !Isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
#endif

    free(sf->fullname); sf->fullname = copy(full);

    /* In the URW world fontnames aren't just a simple concatenation of */
    /*  family name and modifiers, so neither the family name nor the modifiers */
    /*  changed, then don't change the font name */
    if ( strcmp(family,sf->familyname)==0 && strcmp(n,sf->fontname)==0 )
	/* Don't change the fontname */;
	/* or anything else */
    else {
	free(sf->fontname); sf->fontname = n;
	free(sf->familyname); sf->familyname = copy(family);
	free(sf->weight); sf->weight = NULL;
	if ( strstrmatch(mods,"extralight")!=NULL || strstrmatch(mods,"extra-light")!=NULL )
	    sf->weight = copy("ExtraLight");
	else if ( strstrmatch(mods,"demilight")!=NULL || strstrmatch(mods,"demi-light")!=NULL )
	    sf->weight = copy("DemiLight");
	else if ( strstrmatch(mods,"demibold")!=NULL || strstrmatch(mods,"demi-bold")!=NULL )
	    sf->weight = copy("DemiBold");
	else if ( strstrmatch(mods,"semibold")!=NULL || strstrmatch(mods,"semi-bold")!=NULL )
	    sf->weight = copy("SemiBold");
	else if ( strstrmatch(mods,"demiblack")!=NULL || strstrmatch(mods,"demi-black")!=NULL )
	    sf->weight = copy("DemiBlack");
	else if ( strstrmatch(mods,"extrabold")!=NULL || strstrmatch(mods,"extra-bold")!=NULL )
	    sf->weight = copy("ExtraBold");
	else if ( strstrmatch(mods,"extrablack")!=NULL || strstrmatch(mods,"extra-black")!=NULL )
	    sf->weight = copy("ExtraBlack");
	else if ( strstrmatch(mods,"book")!=NULL )
	    sf->weight = copy("Book");
	else if ( strstrmatch(mods,"regular")!=NULL )
	    sf->weight = copy("Regular");
	else if ( strstrmatch(mods,"roman")!=NULL )
	    sf->weight = copy("Roman");
	else if ( strstrmatch(mods,"normal")!=NULL )
	    sf->weight = copy("Normal");
	else if ( strstrmatch(mods,"demi")!=NULL )
	    sf->weight = copy("Demi");
	else if ( strstrmatch(mods,"medium")!=NULL )
	    sf->weight = copy("Medium");
	else if ( strstrmatch(mods,"bold")!=NULL )
	    sf->weight = copy("Bold");
	else if ( strstrmatch(mods,"heavy")!=NULL )
	    sf->weight = copy("Heavy");
	else if ( strstrmatch(mods,"black")!=NULL )
	    sf->weight = copy("Black");
	else if ( strstrmatch(mods,"Nord")!=NULL )
	    sf->weight = copy("Nord");
/* Sigh. URW uses 4 letter abreviations... */
	else if ( strstrmatch(mods,"Regu")!=NULL )
	    sf->weight = copy("Regular");
	else if ( strstrmatch(mods,"Medi")!=NULL )
	    sf->weight = copy("Medium");
	else if ( strstrmatch(mods,"blac")!=NULL )
	    sf->weight = copy("Black");
	else
	    sf->weight = copy("Medium");
    }

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( sf->fv!=NULL && sf->fv->gw!=NULL ) {
	unichar_t *temp;
	int i;
	GDrawSetWindowTitles(sf->fv->gw,temp = uc_copy(sf->fontname),NULL);
	free(temp);
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->views!=NULL ) {
	    char buffer[300]; unichar_t ubuf[300]; CharView *cv;
	    sprintf( buffer, "%.90s from %.90s", sf->glyphs[i]->name, sf->fontname );
	    uc_strcpy(ubuf,buffer);
	    for ( cv = sf->glyphs[i]->views; cv!=NULL; cv=cv->next )
		GDrawSetWindowTitles(cv->gw,ubuf,NULL);
	}
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI

static int SetFontName(GWindow gw, SplineFont *sf) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
    const unichar_t *ufont = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Fontname));
    const unichar_t *uweight = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Weight));
    const unichar_t *uhum = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Human));
    int diff = uc_strcmp(ufont,sf->fontname)!=0;

    free(sf->familyname);
    free(sf->fontname);
    free(sf->weight);
    free(sf->fullname);
    sf->familyname = cu_copy(ufamily);
    sf->fontname = cu_copy(ufont);
    sf->weight = cu_copy(uweight);
    sf->fullname = cu_copy(uhum);
return( diff );
}

static int CheckNames(struct gfi_data *d) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family));
    const unichar_t *ufont = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname));
    unichar_t *end; const unichar_t *pt;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif

    if ( u_strlen(ufont)>63 ) {
	gwwv_post_error(_("Bad Font Name"),_("A Postscript name should be ASCII\nand must not contain (){}[]<>%%/ or space\nand must be shorter than 63 characters"));
return( false );
    }

    if ( *ufamily=='\0' ) {
	gwwv_post_error(_("A Font Family name is required"),_("A Font Family name is required"));
return( false );
    }
    /* A postscript name cannot be a number. There are two ways it can be a */
    /*  number, it can be a real (which we can check for with strtod) or */
    /*  it can be a "radix number" which is <intval>'#'<intval>. I'll only */
    /*  do a cursory test for that */
    u_strtod(ufamily,&end);
    if ( *end=='\0' || (isdigit(ufamily[0]) && u_strchr(ufamily,'#')!=NULL) ) {
	gwwv_post_error(_("Bad Font Family Name"),_("A Postscript name may not be a number"));
return( false );
    }
    if ( u_strlen(ufamily)>31 ) {
	if ( gwwv_ask(_("Bad Font Family Name"),(const char **) buts,0,1,_("Some versions of Windows will refuse to install postscript fonts if the familyname is longer than 31 characters. Do you want to continue anyway?"))==1 )
return( false );
    } else {
	if ( u_strlen(ufont)>31 ) {
	    if ( gwwv_ask(_("Bad Font Name"),(const char **) buts,0,1,_("Some versions of Windows will refuse to install postscript fonts if the fontname is longer than 31 characters. Do you want to continue anyway?"))==1 )
return( false );
	} else if ( u_strlen(ufont)>29 ) {
	    if ( gwwv_ask(_("Bad Font Name"),(const char **) buts,0,1,_("Adobe's fontname spec (5088.FontNames.pdf) says that fontnames must not be longer than 29 characters. Do you want to continue anyway?"))==1 )
return( false );
	}
    }
    while ( *ufamily ) {
	if ( *ufamily<' ' || *ufamily>=0x7f ) {
	    gwwv_post_error(_("Bad Font Family Name"),_("A Postscript name should be ASCII\nand must not contain (){}[]<>%%/ or space"));
return( false );
	}
	++ufamily;
    }

    u_strtod(ufont,&end);
    if ( (*end=='\0' || (isdigit(ufont[0]) && u_strchr(ufont,'#')!=NULL)) &&
	    *ufont!='\0' ) {
	gwwv_post_error(_("Bad Font Name"),_("A Postscript name may not be a number"));
return( false );
    }
    for ( pt=ufont; *pt; ++pt ) {
	if ( *pt<=' ' || *pt>=0x7f ||
		*pt=='(' || *pt=='[' || *pt=='{' || *pt=='<' ||
		*pt==')' || *pt==']' || *pt=='}' || *pt=='>' ||
		*pt=='%' || *pt=='/' ) {
	    gwwv_post_error(_("Bad Font Name"),_("A Postscript name should be ASCII\nand must not contain (){}[]<>%%/ or space"));
return( false );
	}
    }
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void TTF_PSDupsDefault(SplineFont *sf) {
    struct ttflangname *english;
    char versionbuf[40];

    /* Ok, if we've just loaded a ttf file then we've got a bunch of langnames*/
    /*  we copied some of them (copyright, family, fullname, etc) into equiv */
    /*  postscript entries in the sf. If we then use FontInfo and change the */
    /*  obvious postscript entries we are left with the old ttf entries. If */
    /*  we generate a ttf file and then load it the old values pop up. */
    /* Solution: Anything we can generate by default should be set to NULL */
    for ( english=sf->names; english!=NULL && english->lang!=0x409; english=english->next );
    if ( english==NULL )
return;
    if ( english->names[ttf_family]!=NULL &&
	    strcmp(english->names[ttf_family],sf->familyname)==0 ) {
	free(english->names[ttf_family]);
	english->names[ttf_family]=NULL;
    }
    if ( english->names[ttf_copyright]!=NULL &&
	    strcmp(english->names[ttf_copyright],sf->copyright)==0 ) {
	free(english->names[ttf_copyright]);
	english->names[ttf_copyright]=NULL;
    }
    if ( english->names[ttf_fullname]!=NULL &&
	    strcmp(english->names[ttf_fullname],sf->fullname)==0 ) {
	free(english->names[ttf_fullname]);
	english->names[ttf_fullname]=NULL;
    }
    if ( sf->subfontcnt!=0 || sf->version!=NULL ) {
	if ( sf->subfontcnt!=0 )
	    sprintf( versionbuf, "Version %f", sf->cidversion );
	else
	    sprintf(versionbuf,"Version %.20s ", sf->version);
	if ( english->names[ttf_version]!=NULL &&
		strcmp(english->names[ttf_version],versionbuf)==0 ) {
	    free(english->names[ttf_version]);
	    english->names[ttf_version]=NULL;
	}
    }
    if ( english->names[ttf_subfamily]!=NULL &&
	    strcmp(english->names[ttf_subfamily],SFGetModifiers(sf))==0 ) {
	free(english->names[ttf_subfamily]);
	english->names[ttf_subfamily]=NULL;
    }

    /* User should not be allowed any access to this one, not ever */
    free(english->names[ttf_postscriptname]);
    english->names[ttf_postscriptname]=NULL;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int ttfspecials[] = { ttf_copyright, ttf_family, ttf_fullname,
	ttf_subfamily, ttf_version, -1 };

static char *tn_recalculatedef(struct gfi_data *d,int cur_id) {
    char versionbuf[40], *v;

    switch ( cur_id ) {
      case ttf_copyright:
return( GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Notice)));
      case ttf_family:
return( GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Family)));
      case ttf_fullname:
return( GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Human)));
      case ttf_subfamily:
return( u2utf8_copy(_uGetModifiers(
		_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname)),
		_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family)),
		_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Weight)))));
      case ttf_version:
	sprintf(versionbuf,_("Version %.20s"),
		v=GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Version)));
	free(v);
return( copy(versionbuf));
      default:
return( NULL );
    }
}

static char *TN_DefaultName(GGadget *g, int r, int c) {
    struct gfi_data *d = GGadgetGetUserData(g);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);

    if ( strings==NULL || !strings[3*r+2].user_bits )
return( NULL );

return( tn_recalculatedef(d,strings[3*r+1].u.md_ival ));
}

static const char *langname(int lang,char *buffer) {
    int i;
    for ( i=0; mslanguages[i].text!=NULL; ++i )
	if ( mslanguages[i].userdata == (void *) (intpt) lang )
return( (char *) mslanguages[i].text );

    sprintf( buffer, "%04X", lang );
return( buffer );
}

static int strid_sorter(const void *pt1, const void *pt2) {
    const struct matrix_data *n1 = pt1, *n2 = pt2;
    char buf1[20], buf2[20];
    const char *l1, *l2;

    if ( n1[1].u.md_ival!=n2[1].u.md_ival )
return( n1[1].u.md_ival - n2[1].u.md_ival );

    l1 = langname(n1[0].u.md_ival,buf1);
    l2 = langname(n2[0].u.md_ival,buf2);
return( strcoll(l1,l2));
}

static int lang_sorter(const void *pt1, const void *pt2) {
    const struct matrix_data *n1 = pt1, *n2 = pt2;
    char buf1[20], buf2[20];
    const char *l1, *l2;

    if ( n1[0].u.md_ival==n2[0].u.md_ival )
return( n1[1].u.md_ival - n2[1].u.md_ival );

    l1 = langname(n1[0].u.md_ival,buf1);
    l2 = langname(n2[0].u.md_ival,buf2);
return( strcoll(l1,l2));
}

static int ms_thislocale = 0;
static int specialvals(const struct matrix_data *n) {
    if ( n[0].u.md_ival == ms_thislocale )
return( -10000000 );
    else if ( (n[0].u.md_ival&0x3ff) == (ms_thislocale&0x3ff) )
return( -10000000 + (n[0].u.md_ival&~0x3ff) );
    if ( n[0].u.md_ival == 0x409 )	/* English */
return( -1000000 );
    else if ( (n[0].u.md_ival&0x3ff) == 9 )
return( -1000000 + (n[0].u.md_ival&~0x3ff) );

return( 1 );
}

static int speciallang_sorter(const void *pt1, const void *pt2) {
    const struct matrix_data *n1 = pt1, *n2 = pt2;
    char buf1[20], buf2[20];
    const char *l1, *l2;
    int pos1=1, pos2=1;

    /* sort so that entries for the current language are first, then English */
    /*  then alphabetical order */
    if ( n1[0].u.md_ival==n2[0].u.md_ival )
return( n1[1].u.md_ival - n2[1].u.md_ival );

    pos1 = specialvals(n1); pos2 = specialvals(n2);
    if ( pos1<0 || pos2<0 )
return( pos1-pos2 );
    l1 = langname(n1[0].u.md_ival,buf1);
    l2 = langname(n2[0].u.md_ival,buf2);
return( strcoll(l1,l2));
}

static void TTFNames_Resort(struct gfi_data *d) {
    int(*compar)(const void *, const void *);
    GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(edit, &rows);

    if ( strings==NULL )
return;

    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TNLangSort)) )
	compar = lang_sorter;
    else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TNStringSort)) )
	compar = strid_sorter;
    else
	compar = speciallang_sorter;
    ms_thislocale = d->langlocalecode;
    qsort(strings,rows,3*sizeof(struct matrix_data),compar);
}

static void DefaultLanguage(struct gfi_data *d) {
    const char *lang=NULL;
    int i, langlen;
    static char *envs[] = { "LC_ALL", "LC_MESSAGES", "LANG", NULL };
    char langcountry[8], language[4];
    int langcode, langlocalecode;

    for ( i=0; envs[i]!=NULL; ++i ) {
	lang = getenv(envs[i]);
	if ( lang!=NULL ) {
	    langlen = strlen(lang);
	    if (( langlen>5 && lang[5]=='.' && lang[2]=='_' ) ||
		    (langlen==5 && lang[2]=='_' ) ||
		    (langlen==2) ||
		    (langlen==3))	/* Some obscure languages have a 3 letter code */
		/* I understand this language */
    break;
	}
    }
    if ( lang==NULL )
	lang = "en_US";
    strncpy(langcountry,lang,5); langcountry[5] = '\0';
    strncpy(language,lang,3); language[3] = '\0';
    if ( language[2]=='_' ) language[2] = '\0';
    langlen = strlen(language);

    langcode = langlocalecode = -1;
    for ( i=0; ms_2_locals[i].loc_name!=NULL; ++i ) {
	if ( strmatch(langcountry,ms_2_locals[i].loc_name)==0 ) {
	    langlocalecode = ms_2_locals[i].local_id;
	    langcode = langlocalecode&0x3ff;
    break;
	} else if ( strncmp(language,ms_2_locals[i].loc_name,langlen)==0 )
	    langcode = ms_2_locals[i].local_id&0x3ff;
    }
    if ( langcode==-1 )		/* Default to English */
	langcode = 0x9;
    d->langlocalecode = langlocalecode==-1 ? (langcode|0x400) : langlocalecode;
}

static int GFI_Char(struct gfi_data *d,GEvent *event) {
    if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	help("fontinfo.html");
return( true );
    } else if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) ) {
	MenuSaveAll(NULL,NULL,NULL);
return( true );
    } else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	if ( event->u.chr.state&ksm_shift ) {
	    GFI_CancelClose(d);
	} else
	    MenuExit(NULL,NULL,NULL);
return( true );
    }
return( false );
}

static int CheckActiveStyleTranslation(struct gfi_data *d,
	struct matrix_data *strings,int r, int rows) {
    int i,j, eng_pos, other_pos;
    char *english, *new=NULL, *temp, *pt;
    int other_lang = strings[3*r].u.md_ival;
    int changed = false;

    for ( i=rows-1; i>=0 ; --i )
	if ( strings[3*i+1].u.md_ival==ttf_subfamily &&
		strings[3*i].u.md_ival == 0x409 )
    break;
    if ( i<0 || (english = strings[3*i+2].u.md_str)==NULL )
	new = tn_recalculatedef(d,ttf_subfamily);
    else
	new = copy(english);
    for ( i=0; stylelist[i]!=NULL; ++i ) {
	eng_pos = other_pos = -1;
	for ( j=0; stylelist[i][j].str!=NULL; ++j ) {
	    if ( stylelist[i][j].lang == other_lang ) {
		other_pos = j;
	break;
	    }
	}
	if ( other_pos==-1 )
    continue;
	for ( j=0; stylelist[i][j].str!=NULL; ++j ) {
	    if ( stylelist[i][j].lang == 0x409 &&
		    (pt = strstrmatch(new,stylelist[i][j].str))!=NULL ) {
		if ( pt==new && strlen(stylelist[i][j].str)==strlen(new) ) {
		    free(new);
		    free(strings[3*r+2].u.md_str);
		    if ( other_lang==0x415 ) {
			/* polish needs a word before the translation */
			strings[3*r+2].u.md_str = galloc(strlen("odmiana ")+strlen(stylelist[i][other_pos].str)+1);
			strcpy(strings[3*r+2].u.md_str,"odmiana ");
			strcat(strings[3*r+2].u.md_str,stylelist[i][other_pos].str);
		    } else
			strings[3*r+2].u.md_str = copy(stylelist[i][other_pos].str);
return( true );
		}
		temp = galloc((strlen(new)
			+ strlen(stylelist[i][other_pos].str)
			- strlen(stylelist[i][j].str)
			+1));
		strncpy(temp,new,pt-new);
		strcpy(temp+(pt-new),stylelist[i][other_pos].str);
		strcat(temp+(pt-new),pt+strlen(stylelist[i][j].str));
		free(new);
		new = temp;
		changed = true;
	continue;
	    }
	}
    }
    if ( changed ) {
	free(strings[3*r+2].u.md_str);
	if ( other_lang==0x415 ) {
	    /* polish needs a word before the translation */
	    strings[3*r+2].u.md_str = galloc(strlen("odmiana ")+strlen(new)+1);
	    strcpy(strings[3*r+2].u.md_str,"odmiana ");
	    strcat(strings[3*r+2].u.md_str,new);
	    free(new);
	} else
	    strings[3*r+2].u.md_str = new;
    } else
	free(new);
return( changed );
}

#define MID_ToggleBase	1
#define MID_MultiEdit	2
#define MID_Delete	3

static void TN_StrPopupDispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(GDrawGetParentWindow(gw));
    GGadget *g = GWidgetGetControl(d->gw,CID_TNames);

    switch ( mi->mid ) {
      case MID_ToggleBase: {
	int rows;
	struct matrix_data *strings = GMatrixEditGet(g, &rows);
	strings[3*d->tn_active+2].frozen = !strings[3*d->tn_active+2].frozen;
	if ( strings[3*d->tn_active+2].frozen ) {
	    free( strings[3*d->tn_active+2].u.md_str );
	    strings[3*d->tn_active+2].u.md_str = NULL;
	} else {
	    strings[3*d->tn_active+2].u.md_str = tn_recalculatedef(d,strings[3*d->tn_active+1].u.md_ival);
	}
	GGadgetRedraw(g);
      } break;
      case MID_MultiEdit:
	GMatrixEditStringDlg(g,d->tn_active,2);
      break;
      case MID_Delete:
	GMatrixEditDeleteRow(g,d->tn_active);
      break;
    }
}

static int menusort(const void *m1, const void *m2) {
    const GMenuItem *mi1 = m1, *mi2 = m2;

    /* Should do a strcoll here, but I never wrote one */
    if ( mi1->ti.text_is_1byte && mi2->ti.text_is_1byte )
return( strcoll( (char *) (mi1->ti.text), (char *) (mi2->ti.text)) );
    else
return( u_strcmp(mi1->ti.text,mi2->ti.text));
}

static void TN_StrIDEnable(GGadget *g,GMenuItem *mi, int r, int c) {
    int rows, i, j;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);

    for ( i=0; mi[i].ti.text!=NULL; ++i ) {
	int strid = (intpt) mi[i].ti.userdata;
	for ( j=0; j<rows; ++j ) if ( j!=r )
	    if ( strings[3*j].u.md_ival == strings[3*r].u.md_ival &&
		    strings[3*j+1].u.md_ival == strid ) {
		mi[i].ti.disabled = true;
	break;
	    }
    }
    qsort(mi,i,sizeof(mi[0]),menusort);
}    

static void TN_LangEnable(GGadget *g,GMenuItem *mi, int r, int c) {
    int i;

    for ( i=0; mi[i].ti.text!=NULL; ++i )
    qsort(mi,i,sizeof(mi[0]),menusort);
}    

static void TN_NewName(GGadget *g,int row) {
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);

    strings[3*row+1].u.md_ival = ttf_subfamily;
}

static void TN_FinishEdit(GGadget *g,int row,int col,int wasnew) {
    int i,rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    uint8 found[ttf_namemax];
    struct gfi_data *d = (struct gfi_data *) GGadgetGetUserData(g);
    int ret = false;

    if ( col==2 ) {
	if ( strings[3*row+2].u.md_str==NULL || *strings[3*row+2].u.md_str=='\0' ) {
	    GMatrixEditDeleteRow(g,row);
	    ret = true;
	}
    } else {
	if ( col==0 ) {
	    memset(found,0,sizeof(found));
	    found[ttf_idontknow] = true;	/* reserved name id */
	    for ( i=0; i<rows; ++i ) if ( i!=row ) {
		if ( strings[3*i].u.md_ival == strings[3*row].u.md_ival )	/* Same language */
		    found[strings[3*i+1].u.md_ival] = true;
	    }
	    if ( found[ strings[3*row+1].u.md_ival ] ) {
		/* This language already has an entry for this strid */
		/* pick another */
		if ( !found[ttf_subfamily] ) {
		    strings[3*row+1].u.md_ival = ttf_subfamily;
		    ret = true;
		} else {
		    for ( i=0; i<ttf_namemax; ++i )
			if ( !found[i] ) {
			    strings[3*row+1].u.md_ival = i;
			    ret = true;
		    break;
			}
		}
	    }
	}
	if ( (strings[3*row+2].u.md_str==NULL || *strings[3*row+2].u.md_str=='\0') ) {
	    for ( i=0; i<rows; ++i ) if ( i!=row ) {
		if ( strings[3*row+1].u.md_ival == strings[3*i+1].u.md_ival &&
			(strings[3*row].u.md_ival&0xff) == (strings[3*i].u.md_ival&0xff)) {
		    /* Same string, same language, different locale */
		    /* first guess is the same as the other string. */
		    if ( strings[3*i+2].u.md_str==NULL )
			strings[3*row+2].u.md_str = tn_recalculatedef(d,strings[3*row+1].u.md_ival );
		    else
			strings[3*row+2].u.md_str = copy(strings[3*i+2].u.md_str);
		    ret = true;
	    break;
		}
	    }
	    /* If we didn't find anything above, and if we've got a style */
	    /*  (subfamily) see if we can guess a translation from the english */
	    if ( i==rows && strings[3*row+1].u.md_ival == ttf_subfamily )
		ret |= CheckActiveStyleTranslation(d,strings,row,rows);
	}
    }
    if ( ret )
	GGadgetRedraw(g);
}

static int TN_CanDelete(GGadget *g,int row) {
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    if ( strings==NULL )
return( false );

return( !strings[3*row+2].user_bits );
}

static void TN_PopupMenu(GGadget *g,GEvent *event,int r,int c) {
    struct gfi_data *d = (struct gfi_data *) GGadgetGetUserData(g);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    GMenuItem mi[5];
    int i;

    if ( strings==NULL )
return;

    d->tn_active = r;

    memset(mi,'\0',sizeof(mi));
    for ( i=0; i<3; ++i ) {
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i+1;
	mi[i].invoke = TN_StrPopupDispatch;
	mi[i].ti.text_is_1byte = true;
    }
    mi[MID_Delete-1].ti.disabled = strings[3*r+2].user_bits;
    mi[MID_ToggleBase-1].ti.disabled = !strings[3*r+2].user_bits;
    if ( strings[3*r+2].frozen ) {
	mi[MID_MultiEdit-1].ti.disabled = true;
	mi[MID_ToggleBase-1].ti.text = (unichar_t *) _("Detach from PostScript Names");
    } else {
	char *temp;
	mi[MID_ToggleBase-1].ti.text = (unichar_t *) _("Same as PostScript Names");
	temp = tn_recalculatedef(d,strings[3*r+1].u.md_ival);
	mi[MID_ToggleBase-1].ti.disabled = (temp==NULL);
	free(temp);
    }
    if ( c!=2 )
	mi[MID_MultiEdit-1].ti.disabled = true;
    mi[MID_MultiEdit-1].ti.text = (unichar_t *) _("Multi-line edit");
    mi[MID_Delete-1].ti.text = (unichar_t *) _("Delete");
    GMenuCreatePopupMenu(event->w,event, mi);
}

static int TN_PassChar(GGadget *g,GEvent *e) {
return( GFI_Char(GGadgetGetUserData(g),e));
}

static char *TN_BigEditTitle(GGadget *g,int r, int c) {
    char buf[100], buf2[20];
    const char *lang;
    int k;
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);

    lang = langname(strings[3*r].u.md_ival,buf2);
    for ( k=0; ttfnameids[k].text!=NULL && ttfnameids[k].userdata!=(void *) (intpt) strings[3*r+1].u.md_ival;
	    ++k );
    snprintf(buf,sizeof(buf),_("%1$.30s string for %2$.30s"),
	    lang, (char *) ttfnameids[k].text );
return( copy( buf ));
}

static void TNMatrixInit(struct matrixinit *mi,struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i,j,k,cnt;
    uint8 sawEnglishUS[ttf_namemax];
    struct ttflangname *tln;
    struct matrix_data *md;

    DefaultLanguage(d);

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 3;
    mi->col_init = ci;

    md = NULL;
    for ( k=0; k<2; ++k ) {
	memset(sawEnglishUS,0,sizeof(sawEnglishUS));
	cnt = 0;
	for ( tln = sf->names; tln!=NULL; tln = tln->next ) {
	    for ( i=0; i<ttf_namemax; ++i ) if ( i!=ttf_postscriptname && tln->names[i]!=NULL ) {
		if ( md!=NULL ) {
		    md[3*cnt  ].u.md_ival = tln->lang;
		    md[3*cnt+1].u.md_ival = i;
		    md[3*cnt+2].u.md_str = copy(tln->names[i]);
		}
		++cnt;
		if ( tln->lang==0x409 )
		    sawEnglishUS[i] = true;
	    }
	}
	for ( i=0; ttfspecials[i]!=-1; ++i ) if ( !sawEnglishUS[ttfspecials[i]] ) {
	    if ( md!=NULL ) {
		md[3*cnt  ].u.md_ival = 0x409;
		md[3*cnt+1].u.md_ival = ttfspecials[i];
		md[3*cnt+2].u.md_str = NULL;
/* if frozen is set then can't remove or edit. (old basedon bit) */
		md[3*cnt].frozen = md[3*cnt+1].frozen = md[3*cnt+2].frozen = true;
/* if user_bits is set then can't remove. (old cantremove bit) */
		md[3*cnt].user_bits = md[3*cnt+1].user_bits = md[3*cnt+2].user_bits = true;
	    }
	    ++cnt;
	}
	if ( md==NULL )
	    md = gcalloc(3*(cnt+10),sizeof(struct matrix_data));
    }
    for ( i=0; i<cnt; ++i ) if ( md[3*cnt].u.md_ival==0x409 ) {
	for ( j=0; ttfspecials[j]!=-1 && ttfspecials[j]!=md[3*cnt+1].u.md_ival; ++j );
	md[3*i].user_bits = md[3*i+1].user_bits = md[3*i+2].user_bits = 
		( ttfspecials[j]!=-1 );
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;

    mi->initrow = TN_NewName;
    mi->finishedit = TN_FinishEdit;
    mi->candelete = TN_CanDelete;
    mi->popupmenu = TN_PopupMenu;
    mi->handle_key = TN_PassChar;
    mi->bigedittitle = TN_BigEditTitle;
}

static int GFI_HelpOFL(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	help("http://scripts.sil.org/OFL");
    }
return( true );
}

static int GFI_AddOFL(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *tng = GWidgetGetControl(GGadgetGetWindow(g),CID_TNames);
	int rows;
	struct matrix_data *tns, *newtns;
	int i,j,k,l,m, extras, len;
	char *all, *pt, **data;
	char buffer[1024], *bpt;
	const char *author = GetAuthor();
	char *reservedname, *fallback;
	time_t now;
	struct tm *tm;

	time(&now);
	tm = localtime(&now);

	tns = GMatrixEditGet(tng, &rows); newtns = NULL;
	for ( k=0; k<2; ++k ) {
	    extras = 0;
	    for ( i=0; ofl_str_lang_data[i].data!=NULL; ++i ) {
		for ( j=rows-1; j>=0; --j ) {
		    if ( tns[j*3+1].u.md_ival==ofl_str_lang_data[i].strid &&
			    tns[j*3+0].u.md_ival==ofl_str_lang_data[i].lang ) {
			if ( k ) {
			    free(newtns[j*3+2].u.md_str);
			    newtns[j*3+2].u.md_str = NULL;
			}
		break;
		    }
		}
		if ( j<0 )
		    j = rows + extras++;
		if ( k ) {
		    newtns[j*3+1].u.md_ival = ofl_str_lang_data[i].strid;
		    newtns[j*3+0].u.md_ival = ofl_str_lang_data[i].lang;
		    data = ofl_str_lang_data[i].data;
		    reservedname = fallback = NULL;
		    for ( m=0; m<rows; ++m ) {
			if ( newtns[j*3+1].u.md_ival==ttf_family ) {
			    if ( newtns[j*3+0].u.md_ival==0x409 )
				fallback = newtns[3*j+2].u.md_str;
			    else if ( newtns[j*3+0].u.md_ival==ofl_str_lang_data[i].lang )
				reservedname = newtns[3*j+2].u.md_str;
			}
		    }
		    if ( reservedname==NULL )
			reservedname = fallback;
		    if ( reservedname==NULL )
			reservedname = d->sf->familyname;
		    for ( m=0; m<2; ++m ) {
			len = 0;
			for ( l=0; data[l]!=NULL; ++l ) {
			    if ( l==0 || l==1 ) {
				sprintf( buffer, data[l], tm->tm_year+1900, author, reservedname );
			        bpt = buffer;
			    } else
				bpt = data[l];
			    if ( m ) {
				strcpy( pt, bpt );
			        pt += strlen( bpt );
			        *pt++ = '\n';
			    } else
				len += strlen( bpt ) + 1;		/* for a new line */
			}
			if ( !m )
			    newtns[j*3+2].u.md_str = all = pt = galloc(len+2);
		    }
		    if ( pt>all ) pt[-1] = '\0';
		    else *pt = '\0';
		}
	    }
	    if ( !k ) {
		newtns = gcalloc((rows+extras)*3,sizeof(struct matrix_data));
		memcpy(newtns,tns,rows*3*sizeof(struct matrix_data));
		for ( i=0; i<rows; ++i )
		    newtns[3*i+2].u.md_str = copy(newtns[3*i+2].u.md_str);
	    }
	}
	GMatrixEditSet(tng, newtns, rows+extras, false);
	gwwv_post_notice(_("Please read the OFL"),_(
	    "You should read the OFL and its FAQ \n"
	    "at http://scripts.sil.org/OFL."
	    "\n"
	    "If you are not very familiar with English,\n"
	    "please check if there is a translation of the \n"
	    "FAQ or an unofficial translation of the license \n"
	    "in your mother tongue or preferred language. \n"
	    "\n"
	    "Fontforge does not know about your email or URL,\n"
	    "you will need to add them manually. \n"
	    "Please fill in the copyright notice in the license\n"
	    "header along with any Reserved Font Name(s).\n"
	    "If you are branching from an existing font make sure\n"
	    "you have the right to do so and remember to add your\n"
	    "additional copyright notice along with any Reserved Font Name(s).\n" ));
    }
return( true );
}
	
static int Gasp_Default(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *gg = GWidgetGetControl(GGadgetGetWindow(g),CID_Gasp);
	int rows;
	struct matrix_data *gasp;

	if ( !SFHasInstructions(d->sf)) {
	    rows = 1;
	    gasp = gcalloc(5,sizeof(struct matrix_data));
	    gasp[0].u.md_ival = 65535;
	    gasp[1].u.md_ival = 0;	/* no grid fit (we have no instructions, we can't grid fit) */
	    gasp[2].u.md_ival = 1;	/* do anti-alias */
	    gasp[2].u.md_ival = 0;	/* do symmetric smoothing */
	    gasp[2].u.md_ival = 0;	/* do no grid fit w/ sym smooth */
	} else {
	    rows = 3;
	    gasp = gcalloc(5,sizeof(struct matrix_data));
	    gasp[0].u.md_ival = 8;     gasp[1].u.md_ival = 0; gasp[2].u.md_ival = 1;
		    gasp[3].u.md_ival = 0; gasp[4].u.md_ival = 0;
	    gasp[5].u.md_ival = 16;    gasp[6].u.md_ival = 1; gasp[7].u.md_ival = 0;
		    gasp[8].u.md_ival = 0; gasp[9].u.md_ival = 0;
	    gasp[10].u.md_ival = 65535; gasp[11].u.md_ival = 1; gasp[12].u.md_ival = 1;
		    gasp[13].u.md_ival = 0; gasp[14].u.md_ival = 0;
	}
	GMatrixEditSet(gg,gasp,rows,false);
    }
return( true );
}

static int Gasp_CanDelete(GGadget *g,int row) {
    int rows;
    struct matrix_data *gasp = GMatrixEditGet(g, &rows);
    if ( gasp==NULL )
return( false );

    /* Only allow them to delete the sentinal entry if that would give us an */
    /* empty gasp table */
return( gasp[5*row].u.md_ival!=0xffff || rows==1 );
}

static int gasp_comp(const void *_md1, const void *_md2) {
    const struct matrix_data *md1 = _md1, *md2 = _md2;
return( md1->u.md_ival - md2->u.md_ival );
}

static void Gasp_FinishEdit(GGadget *g,int row,int col,int wasnew) {
    int rows;
    struct matrix_data *gasp = GMatrixEditGet(g, &rows);

    if ( col==0 ) {
	qsort(gasp,rows,3*sizeof(struct matrix_data),gasp_comp);
	GGadgetRedraw(g);
    }
}

static void GaspMatrixInit(struct matrixinit *mi,struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i;
    struct matrix_data *md;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 5;
    mi->col_init = gaspci;

    if ( sf->gasp_cnt==0 ) {
	md = gcalloc(5,sizeof(struct matrix_data));
	mi->initial_row_cnt = 0;
    } else {
	md = gcalloc(5*sf->gasp_cnt,sizeof(struct matrix_data));
	for ( i=0; i<sf->gasp_cnt; ++i ) {
	    md[5*i  ].u.md_ival = sf->gasp[i].ppem;
	    md[5*i+1].u.md_ival = (sf->gasp[i].flags&1)?1:0;
	    md[5*i+2].u.md_ival = (sf->gasp[i].flags&2)?1:0;
	    md[5*i+3].u.md_ival = (sf->gasp[i].flags&4)?1:0;
	    md[5*i+4].u.md_ival = (sf->gasp[i].flags&8)?1:0;
	}
	mi->initial_row_cnt = sf->gasp_cnt;
    }
    mi->matrix_data = md;

    mi->finishedit = Gasp_FinishEdit;
    mi->candelete = Gasp_CanDelete;
    mi->handle_key = TN_PassChar;
}

static int GFI_GaspVersion(GGadget *g, GEvent *e) {
    if ( e->u.control.subtype == et_listselected ) {
	int version = GGadgetGetFirstListSelectedItem(g);
	GGadget *gasp = GWidgetGetControl(GGadgetGetWindow(g),CID_Gasp);
	if ( version == 0 ) {
	    GMatrixEditEnableColumn(gasp,3,false);
	    GMatrixEditEnableColumn(gasp,4,false);
	} else {
	    GMatrixEditEnableColumn(gasp,3,true);
	    GMatrixEditEnableColumn(gasp,4,true);
	}
	GGadgetRedraw(gasp);
    }
return( true );
}

static int GFI_SortBy(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = (struct gfi_data *) GDrawGetUserData(GGadgetGetWindow(g));
	TTFNames_Resort(d);
	GGadgetRedraw(GWidgetGetControl(d->gw,CID_TNames));
    }
return( true );
}

static void BDFsSetAsDs(SplineFont *sf) {
    BDFFont *bdf;
    real scale;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	scale = bdf->pixelsize / (real) (sf->ascent+sf->descent);
	bdf->ascent = rint(sf->ascent*scale);
	bdf->descent = bdf->pixelsize-bdf->ascent;
    }
}

static char *texparams[] = { N_("Slant:"), N_("Space:"), N_("Stretch:"),
	N_("Shrink:"), N_("XHeight:"), N_("Quad:"),
/* GT: Extra Space, see below for a full comment */
	N_("Extra Sp:"), NULL };
static char *texpopups[] = { N_("In an italic font the horizontal change per unit vertical change"),
    N_("The amount of space between words when using this font"),
    N_("The amount of strechable space between words when using this font"),
    N_("The amount the space between words may shrink when using this font"),
    N_("The height of the lower case letters with flat tops"),
    N_("The width of one em"),
    N_("Either:\nThe amount of extra space to be added after a sentence\nOr the space to be used within math formulae"),
    NULL};

static int ParseTeX(struct gfi_data *d) {
    int i, err=false;
    double em = (d->sf->ascent+d->sf->descent), val;

    for ( i=0; texparams[i]!=0 ; ++i ) {
	val = GetReal8(d->gw,CID_TeX+i,texparams[i],&err);
	d->texdata.params[i] = rint( val/em * (1<<20) );
    }
    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXText)) )
	d->texdata.type = tex_text;
    else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMathSym)) )
	d->texdata.type = tex_math;
    else
	d->texdata.type = tex_mathext;
return( !err );
}

static int ttfmultuniqueids(SplineFont *sf,struct gfi_data *d) {
    struct ttflangname *tln;
    int found = false;
    int i;

    if ( d->names_set ) {
	GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
	int rows;
	struct matrix_data *strings = GMatrixEditGet(edit, &rows);
	for ( i=0; i<rows; ++i )
	    if ( strings[3*i+1].u.md_ival==ttf_uniqueid ) {
		if ( found )
return( true );
		found = true;
	    }
    } else {
	for ( tln = sf->names; tln!=NULL; tln=tln->next )
	    if ( tln->names[ttf_uniqueid]!=NULL ) {
		if ( found )
return( true );
		found = true;
	    }
    }
return( false );
}

static int ttfuniqueidmatch(SplineFont *sf,struct gfi_data *d) {
    struct ttflangname *tln;
    int i;

    if ( sf->names==NULL )
return( false );

    if ( !d->names_set ) {
	for ( tln = sf->names; tln!=NULL; tln=tln->next )
	    if ( tln->names[ttf_uniqueid]!=NULL )
return( true );
    } else {
	GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
	int rows;
	struct matrix_data *strings = GMatrixEditGet(edit, &rows);

	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;		/* Not set, so if it has been given a new value */
				/*  that's a change, and if it hasn't that's ok */
	    for ( i=0; i<rows; ++i )
		if ( strings[3*i+1].u.md_ival==ttf_uniqueid && strings[3*i].u.md_ival==tln->lang )
	    break;
	    if ( i==rows )
	continue;		/* removed. That's a change */
	    if ( strcmp(tln->names[ttf_uniqueid],strings[3*i+2].u.md_str )==0 )
return( true );		/* name unchanged */
	}
    }
return( false );
}

static void ttfuniqueidfixup(SplineFont *sf,struct gfi_data *d) {
    struct ttflangname *tln;
    char *changed = NULL;
    int i;

    if ( sf->names==NULL )
return;

    if ( !d->names_set ) {
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    free( tln->names[ttf_uniqueid]);
	    tln->names[ttf_uniqueid] = NULL;
	}
    } else {
	GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
	int rows;
	struct matrix_data *strings = GMatrixEditGet(edit, &rows);

	/* see if any instances of the name have changed */
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;
	    for ( i=0; i<rows; ++i )
		if ( strings[3*i+1].u.md_ival==ttf_uniqueid && strings[3*i].u.md_ival==tln->lang )
	    break;
	    if ( i==rows )
	continue;
	    if ( strcmp(tln->names[ttf_uniqueid],strings[3*i+2].u.md_str )!=0 )
		changed = copy(strings[3*i+2].u.md_str );
	break;
	}
	/* All unique ids should be the same, if any changed set the unchanged */
	/*  ones to the one that did (or the first of many if several changed) */
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;
	    for ( i=0; i<rows; ++i )
		if ( strings[3*i+1].u.md_ival==ttf_uniqueid && strings[3*i].u.md_ival==tln->lang )
	    break;
	    if ( i==rows )
	continue;
	    if ( strcmp(tln->names[ttf_uniqueid],strings[3*i+2].u.md_str)==0 ) {
		free(strings[3*i+2].u.md_str);
		strings[3*i+2].u.md_str = changed!=NULL
			? copy( changed )
			: NULL;
	    }
	}
    }
}

static void StoreTTFNames(struct gfi_data *d) {
    struct ttflangname *tln;
    SplineFont *sf = d->sf;
    int i;
    GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(edit, &rows);

    TTFLangNamesFree(sf->names); sf->names = NULL;

    for ( i=0; i<rows; ++i ) {
	for ( tln=sf->names; tln!=NULL && tln->lang!=strings[3*i].u.md_ival; tln=tln->next );
	if ( tln==NULL ) {
	    tln = chunkalloc(sizeof(struct ttflangname));
	    tln->lang = strings[3*i].u.md_ival;
	    tln->next = sf->names;
	    sf->names = tln;
	}
	tln->names[strings[3*i+1].u.md_ival] = copy(strings[3*i+2].u.md_str );
    }
    TTF_PSDupsDefault(sf);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

/* If we change the ascent/descent of a sub font then consider changing the */
/*  as/ds of the master font. I used to think this irrelevant, but as the */
/*  typoAscent/Descent is based on the master's ascent/descent it actually */
/*  is meaningful. Set the master to the subfont with the most glyphs */
void CIDMasterAsDes(SplineFont *sf) {
    SplineFont *cidmaster = sf->cidmaster;
    SplineFont *best;
    int i, cid, cnt, bcnt;

    if ( cidmaster==NULL )
return;
    best = NULL; bcnt = 0;
    for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	for ( cid=cnt=0; cid<sf->glyphcnt; ++cid )
	    if ( sf->glyphs[cid]!=NULL )
		++cnt;
	if ( cnt>bcnt ) {
	    best = sf;
	    bcnt = cnt;
	}
    }
    if ( best==NULL && cidmaster->subfontcnt>0 )
	best = cidmaster->subfonts[0];
    if ( best!=NULL ) {
	double ratio = 1000.0/(best->ascent+best->descent);
	int ascent = rint(best->ascent*ratio);
	if ( cidmaster->ascent!=ascent || cidmaster->descent!=1000-ascent ) {
	    cidmaster->ascent = ascent;
	    cidmaster->descent = 1000-ascent;
	}
    }
}

void SFSetModTime(SplineFont *sf) {
    time_t now;
    time(&now);
    sf->modificationtime = now;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void GFI_ApplyLookupChanges(struct gfi_data *gfi) {
    int i,j, isgpos;
    OTLookup *last;
    SplineFont *sf = gfi->sf;
    struct lookup_subtable *sublast;

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	struct lkdata *lk = &gfi->tables[isgpos];
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
		SFRemoveLookup(gfi->sf,lk->all[i].lookup);
	    else for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
		    SFRemoveLookupSubTable(gfi->sf,lk->all[i].subtables[j].subtable);
	    }
	}
	last = NULL;
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( !lk->all[i].deleted ) {
		if ( last!=NULL )
		    last->next = lk->all[i].lookup;
		else if ( isgpos )
		    sf->gpos_lookups = lk->all[i].lookup;
		else
		    sf->gsub_lookups = lk->all[i].lookup;
		last = lk->all[i].lookup;
		sublast = NULL;
		for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		    if ( !lk->all[i].subtables[j].deleted ) {
			if ( sublast!=NULL )
			    sublast->next = lk->all[i].subtables[j].subtable;
			else
			    last->subtables = lk->all[i].subtables[j].subtable;
			sublast = lk->all[i].subtables[j].subtable;
		    }
		}
		if ( sublast!=NULL )
		    sublast->next = NULL;
		else
		    last->subtables = NULL;
	    }
	    free(lk->all[i].subtables);
	}
	if ( last!=NULL )
	    last->next = NULL;
	else if ( isgpos )
	    sf->gpos_lookups = NULL;
	else
	    sf->gsub_lookups = NULL;
	free(lk->all);
    }
}

static int GFI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *d = GDrawGetUserData(gw);
	SplineFont *sf = d->sf, *_sf;
	int interp;
	int reformat_fv=0, retitle_fv=false;
	int upos, uwid, as, des, err = false, weight=0;
	int uniqueid, linegap=0, vlinegap=0, winascent=0, windescent=0;
	int tlinegap=0, tascent=0, tdescent=0, hascent=0, hdescent=0;
	int winaoff=true, windoff=true;
	int taoff=true, tdoff=true, haoff = true, hdoff = true;
	real ia, cidversion;
	const unichar_t *txt, *fond; unichar_t *end;
	int i,j, mcs;
	int vmetrics, vorigin, namechange, order2;
	int xuidchanged = false;
	GTextInfo *pfmfam, *ibmfam, *fstype, *nlitem;
	int32 len;
	GTextInfo **ti;
	int subs[4], super[4], strike[2];
	int design_size, size_top, size_bottom, styleid;
	int strokedfont = false;
	real strokewidth;
#ifdef FONTFORGE_CONFIG_TYPE3
	int multilayer = false;
#endif
	char os2_vendor[4];
	NameList *nl;
	extern int allow_utf8_glyphnames;
	int os2version;
	int rows, gasprows;
	struct matrix_data *strings = GMatrixEditGet(GWidgetGetControl(d->gw,CID_TNames), &rows);
	struct matrix_data *gasp    = GMatrixEditGet(GWidgetGetControl(d->gw,CID_Gasp), &gasprows);

	if ( strings==NULL || gasp==NULL )
return( true );
	if ( gasprows>0 && gasp[5*gasprows-5].u.md_ival!=65535 ) {
	    gwwv_post_error(_("Bad Grid Fiting table"),_("The 'gasp' (Grid Fit) table must end with a pixel entry of 65535"));
return( true );
	}
	if ( !CheckNames(d))
return( true );
	if ( !PIFinishFormer(d))
return( true );
	if ( d->ccd )
	    CCD_Close(d->ccd);
	if ( d->smd )
	    SMD_Close(d->smd);

	if ( ttfmultuniqueids(sf,d)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    char *buts[3];
	    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
	    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif
	    if ( gwwv_ask(_("Too many Unique Font IDs"),(const char **) buts,0,1,_("You should only specify the TrueType Unique Font Identification string in one language. This font has more. Do you want to continue anyway?"))==1 )
return( true );
	}
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
	if ( !isalpha(*txt)) {
	    BadFamily();
return( true );
	}
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_ItalicAngle));
	ia = u_strtod(txt,&end);
	if ( *end!='\0' ) {
	    Protest8(_("_Italic Angle:"));
return(true);
	}
	order2 = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsOrder2));
	strokedfont = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsStrokedFont));
	strokewidth = GetReal8(gw,CID_StrokeWidth,_("Stroke _Width:"),&err);
#ifdef FONTFORGE_CONFIG_TYPE3
	multilayer = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsMultiLayer));
#endif
	vmetrics = GGadgetIsChecked(GWidgetGetControl(gw,CID_HasVerticalMetrics));
	upos = GetReal8(gw,CID_UPos, _("Underline _Position:"),&err);
	uwid = GetReal8(gw,CID_UWidth,S_("Underline|_Height:"),&err);
	GetInt8(gw,CID_Em,_("_Em Size:"),&err);	/* just check for errors. redundant info */
	as = GetInt8(gw,CID_Ascent,_("_Ascent:"),&err);
	des = GetInt8(gw,CID_Descent,_("_Descent:"),&err);
	uniqueid = GetInt8(gw,CID_UniqueID,_("UniqueID"),&err);
	design_size = rint(10*GetReal8(gw,CID_DesignSize,_("De_sign Size:"),&err));
	size_bottom = rint(10*GetReal8(gw,CID_DesignBottom,_("_Bottom"),&err));
	size_top = rint(10*GetReal8(gw,CID_DesignTop,_("_Top"),&err));
	styleid = GetInt8(gw,CID_StyleID,_("Style _ID:"),&err);
	if ( err )
return(true);
	if ( sf->subfontcnt!=0 ) {
	    cidversion = GetReal8(gw,CID_Version,_("_Version"),&err);
	    if ( err )
return(true);
	}
	if ( vmetrics )
	    vorigin = GetInt8(gw,CID_VOrigin,_("Vertical _Origin:"),&err);
	os2version = sf->os2_version;
	if ( d->ttf_set ) {
	    char *os2v = GGadgetGetTitle8(GWidgetGetControl(gw,CID_OS2Version));
	    if ( strcasecmp(os2v,_( (char *) os2versions[0].text ))== 0 )
		os2version = 0;
	    else
		os2version = GetInt8(gw,CID_OS2Version,_("Weight, Width, Slope Only"),&err);
	    free(os2v);
	    /* Only use the normal routine if we get no value, because */
	    /*  "400 Book" is a reasonable setting, but would cause GetInt */
	    /*  to complain */
	    weight = u_strtol(_GGadgetGetTitle(GWidgetGetControl(gw,CID_WeightClass)),NULL,10);
	    if ( weight == 0 )
		weight = GetInt8(gw,CID_WeightClass,_("_Weight Class"),&err);
	    linegap = GetInt8(gw,CID_LineGap,_("HHead _Line Gap:"),&err);
	    tlinegap = GetInt8(gw,CID_TypoLineGap,_("Typo Line _Gap:"),&err);
	    if ( vmetrics )
		vlinegap = GetInt8(gw,CID_VLineGap,_("VHead _Column Spacing:"),&err);
	    winaoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_WinAscentIsOff));
	    windoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_WinDescentIsOff));
	    winascent  = GetInt8(gw,CID_WinAscent,winaoff ? _("Win _Ascent Offset:") : _("Win Ascent:"),&err);
	    windescent = GetInt8(gw,CID_WinDescent,windoff ? _("Win _Descent Offset:") : _("Win Descent:"),&err);
	    taoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_TypoAscentIsOff));
	    tdoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_TypoDescentIsOff));
	    tascent  = GetInt8(gw,CID_TypoAscent,taoff ? _("_Typo Ascent Offset:") : _("Typo Ascent:"),&err);
	    tdescent = GetInt8(gw,CID_TypoDescent,tdoff ? _("T_ypo Descent Offset:") : _("Typo Descent:"),&err);
	    haoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_HHeadAscentIsOff));
	    hdoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_HHeadDescentIsOff));
	    hascent  = GetInt8(gw,CID_HHeadAscent,haoff ? _("_HHead Ascent Offset:") : _("HHead Ascent:"),&err);
	    hdescent = GetInt8(gw,CID_HHeadDescent,hdoff ? _("HHead De_scent Offset:") : _("HHead Descent:"),&err);
	    if ( err )
return(true);

	    if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_SubSuperDefault)) ) {
		for ( i=0; i<4; ++i )
		    subs[i] = GetInt8(gw,CID_SubXSize+i,_("Subscript"),&err);
		for ( i=0; i<4; ++i )
		    super[i] = GetInt8(gw,CID_SuperXSize+i,_("Superscript"),&err);
		for ( i=0; i<2; ++i )
		    strike[i] = GetInt8(gw,CID_StrikeoutSize+i,_("Strikeout"),&err);
	    }
	    txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Vendor));
	    if ( u_strlen(txt)>4 || txt[0]>0x7e || (txt[0]!='\0' && (txt[1]>0x7e ||
		    (txt[1]!='\0' && (txt[2]>0x7e || (txt[2]!='\0' && txt[3]>0x7e))))) ) {
		if ( u_strlen(txt)>4 )
		    gwwv_post_error(_("Bad IBM Family"),_("Tag must be 4 characters long"));
		else
		    gwwv_post_error(_("Bad IBM Family"),_("A tag must be 4 ASCII characters"));
return( true );
	    }
	    os2_vendor[0] = txt[0]==0 ? ' ' : txt[0];
	    os2_vendor[1] = txt[0]==0 || txt[1]=='\0' ? ' ' : txt[1];
	    os2_vendor[2] = txt[0]==0 || txt[1]=='\0' || txt[2]=='\0' ? ' ' : txt[2];
	    os2_vendor[3] = txt[0]==0 || txt[1]=='\0' || txt[2]=='\0' || txt[3]=='\0' ? ' ' : txt[3];
	}
	if ( err )
return(true);
	if ( d->tex_set ) {
	    if ( !ParseTeX(d))
return( true );
	}
	if ( as+des>16384 || des<0 || as<0 ) {
	    gwwv_post_error(_("Bad Ascent/Descent"),_("Ascent and Descent must be positive and their sum less than 16384"));
return( true );
	}
	mcs = -1;
	if ( !GGadgetIsChecked(GWidgetGetControl(d->gw,CID_MacAutomatic)) ) {
	    mcs = 0;
	    ti = GGadgetGetList(GWidgetGetControl(d->gw,CID_MacStyles),&len);
	    for ( i=0; i<len; ++i )
		if ( ti[i]->selected )
		    mcs |= (int) (intpt) ti[i]->userdata;
	    if ( (mcs&sf_condense) && (mcs&sf_extend)) {
		gwwv_post_error(_("Bad Style"),_("A style may not have both condense and extend set (it makes no sense)"));
return( true );
	    }
	}
	if ( order2!=sf->order2 && sf->changed && AskLoseUndoes())
return( true );
	if ( order2!=sf->order2 && !SFCloseAllInstrs(sf))
return( true );

	nlitem = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_Namelist));
	if ( nlitem==NULL )
	    nl = DefaultNameListForNewFonts();
	else {
	    char *name = u2utf8_copy(nlitem->text);
	    nl = NameListByName(name);
	    free(name);
	}
	if ( nl->uses_unicode && !allow_utf8_glyphnames ) {
	    gwwv_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set,\nbut there are names in this namelist which use characters outside\nthat range."));
return(true);
	}
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( strokedfont!=sf->strokedfont || multilayer!=sf->multilayer ) {
	    if ( sf->strokedfont && multilayer )
		SFSetLayerWidthsStroked(sf,sf->strokewidth);
	    else if ( sf->multilayer )
		SFSplinesFromLayers(sf,strokedfont);
	    SFReinstanciateRefs(sf);
	    if ( multilayer!=sf->multilayer ) {
		sf->multilayer = multilayer;
		SFLayerChange(sf);
	    }
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
		sf->glyphs[i]->changedsincelasthinted = !strokedfont && !multilayer;
	}
#else
	if ( strokedfont!=sf->strokedfont )
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
		sf->glyphs[i]->changedsincelasthinted = !strokedfont;
#endif
	sf->strokedfont = strokedfont;
	sf->strokewidth = strokewidth;
	GDrawSetCursor(gw,ct_watch);
	namechange = SetFontName(gw,sf);
	if ( namechange ) retitle_fv = true;
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_XUID));
	xuidchanged = (sf->xuid==NULL && *txt!='\0') ||
			(sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0);
	if ( namechange &&
		((uniqueid!=0 && uniqueid==sf->uniqueid) ||
		 (sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0) ||
		 ttfuniqueidmatch(sf,d)) ) {
	    char *buts[4];
	    int ans;
	    buts[0] = _("Change");
	    buts[1] = _("Retain");
#if defined(FONTFORGE_CONFIG_GDRAW)
	    buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
	    buts[2] = GTK_STOCK_CANCEL;
#endif
	    buts[3] = NULL;
	    ans = gwwv_ask(_("Change UniqueID?"),(const char **) buts,0,2,_("You have changed this font's name without changing the UniqueID (or XUID).\nThis is probably not a good idea, would you like me to\ngenerate a random new value?"));
	    if ( ans==2 ) {
		GDrawSetCursor(gw,ct_pointer);
return(true);
	    }
	    if ( ans==0 ) {
		if ( uniqueid!=0 && uniqueid==sf->uniqueid )
		    uniqueid = 4000000 + (rand()&0x3ffff);
		if ( sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0 ) {
		    SFRandomChangeXUID(sf);
		    xuidchanged = true;
		}
	    }
	    if ( ttfuniqueidmatch(sf,d))
		ttfuniqueidfixup(sf,d);
	} else {
	    free(sf->xuid);
	    sf->xuid = *txt=='\0'?NULL:cu_copy(txt);
	}

	free(sf->gasp);
	sf->gasp_cnt = gasprows;
	if ( gasprows==0 )
	    sf->gasp = NULL;
	else {
	    sf->gasp = galloc(gasprows*sizeof(struct gasp));
	    sf->gasp_version = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_GaspVersion));
	    for ( i=0; i<gasprows; ++i ) {
		sf->gasp[i].ppem = gasp[5*i].u.md_ival;
		if ( sf->gasp_version==0 )
		    sf->gasp[i].flags = gasp[5*i+1].u.md_ival |
			    (gasp[5*i+2].u.md_ival<<1);
		else
		    sf->gasp[i].flags = gasp[5*i+1].u.md_ival |
			    (gasp[5*i+2].u.md_ival<<1) |
			    (gasp[5*i+2].u.md_ival<<2) |
			    (gasp[5*i+2].u.md_ival<<3);
	    }
	}
	sf->head_optimized_for_cleartype = GGadgetIsChecked(GWidgetGetControl(gw,CID_HeadClearType));

	OtfNameListFree(sf->fontstyle_name);
	sf->fontstyle_name = OtfNameFromStyleNames(GWidgetGetControl(gw,CID_StyleName));
	sf->design_size = design_size;
	sf->design_range_bottom = size_bottom;
	sf->design_range_top = size_top;
	sf->fontstyle_id = styleid;

	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Notice));
	free(sf->copyright); sf->copyright = cu_copy(txt);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Comment));
	free(sf->comments); sf->comments = cu_copy(*txt?txt:NULL);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_DefBaseName));
	if ( *txt=='\0' || GGadgetIsChecked(GWidgetGetControl(gw,CID_SameAsFontname)) )
	    txt = NULL;
	free(sf->defbasefilename); sf->defbasefilename = u2utf8_copy(txt);
	if ( sf->subfontcnt!=0 ) {
	    sf->cidversion = cidversion;
	} else {
	    txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Version));
	    free(sf->version); sf->version = cu_copy(txt);
	}
	fond = _GGadgetGetTitle(GWidgetGetControl(gw,CID_MacFOND));
	free(sf->fondname); sf->fondname = NULL;
	if ( *fond )
	    sf->fondname = cu_copy(fond);
	sf->macstyle = mcs;
	sf->italicangle = ia;
	sf->upos = upos;
	sf->uwidth = uwid;
	sf->uniqueid = uniqueid;
	sf->texdata = d->texdata;

	interp = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_Interpretation));
	if ( interp==-1 ) sf->uni_interp = ui_none;
	else sf->uni_interp = (intpt) interpretations[interp].userdata;

	sf->for_new_glyphs = nl;

	if ( sf->hasvmetrics!=vmetrics )
	    CVPaletteDeactivate();		/* Force a refresh later */
	_sf = sf->cidmaster?sf->cidmaster:sf;
	_sf->hasvmetrics = vmetrics;
	for ( j=0; j<_sf->subfontcnt; ++j )
	    _sf->subfonts[j]->hasvmetrics = vmetrics;
	if ( vmetrics ) {
	    _sf->vertical_origin = vorigin;
	    for ( j=0; j<_sf->subfontcnt; ++j )
		_sf->subfonts[j]->vertical_origin = vorigin;
	}

	if ( d->private!=NULL ) {
	    PSDictFree(sf->private);
	    sf->private = d->private;
	    d->private = NULL;
	}
	if ( d->names_set )
	    StoreTTFNames(d);
	if ( d->ttf_set ) {
	    sf->os2_version = os2version;
	    sf->use_typo_metrics = GGadgetIsChecked(GWidgetGetControl(gw,CID_UseTypoMetrics));
	    sf->weight_width_slope_only = GGadgetIsChecked(GWidgetGetControl(gw,CID_WeightWidthSlopeOnly));
	    sf->pfminfo.weight = weight;
	    sf->pfminfo.width = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_WidthClass))+1;
	    pfmfam = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PFMFamily));
	    if ( pfmfam!=NULL )
		sf->pfminfo.pfmfamily = (intpt) (pfmfam->userdata);
	    else
		sf->pfminfo.pfmfamily = 0x11;
	    ibmfam = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_IBMFamily));
	    if ( pfmfam!=NULL )
		sf->pfminfo.os2_family_class = (intpt) (ibmfam->userdata);
	    else
		sf->pfminfo.os2_family_class = 0x00;
	    memcpy(sf->pfminfo.os2_vendor,os2_vendor,sizeof(os2_vendor));
	    fstype = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_FSType));
	    if ( fstype!=NULL )
		sf->pfminfo.fstype = (intpt) (fstype->userdata);
	    else
		sf->pfminfo.fstype = 0xc;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_NoSubsetting)))
		sf->pfminfo.fstype |=0x100;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_OnlyBitmaps)))
		sf->pfminfo.fstype |=0x200;
	    for ( i=0; i<10; ++i )
		sf->pfminfo.panose[i] = (intpt) (GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PanFamily+i))->userdata);
	    sf->pfminfo.panose_set = !GGadgetIsChecked(GWidgetGetControl(gw,CID_PanDefault));
	    sf->pfminfo.os2_typolinegap = tlinegap;
	    sf->pfminfo.linegap = linegap;
	    if ( vmetrics )
		sf->pfminfo.vlinegap = vlinegap;
	    sf->pfminfo.os2_winascent = winascent;
	    sf->pfminfo.os2_windescent = windescent;
	    sf->pfminfo.winascent_add = winaoff;
	    sf->pfminfo.windescent_add = windoff;
	    sf->pfminfo.os2_typoascent = tascent;
	    sf->pfminfo.os2_typodescent = tdescent;
	    sf->pfminfo.typoascent_add = taoff;
	    sf->pfminfo.typodescent_add = tdoff;
	    sf->pfminfo.hhead_ascent = hascent;
	    sf->pfminfo.hhead_descent = hdescent;
	    sf->pfminfo.hheadascent_add = haoff;
	    sf->pfminfo.hheaddescent_add = hdoff;
	    sf->pfminfo.pfmset = true;

	    sf->pfminfo.subsuper_set = !GGadgetIsChecked(GWidgetGetControl(gw,CID_PanDefault));
	    if ( sf->pfminfo.subsuper_set ) {
		sf->pfminfo.os2_subxsize = subs[0];
		sf->pfminfo.os2_subysize = subs[1];
		sf->pfminfo.os2_subxoff = subs[2];
		sf->pfminfo.os2_subyoff = subs[3];
		sf->pfminfo.os2_supxsize = super[0];
		sf->pfminfo.os2_supysize = super[1];
		sf->pfminfo.os2_supxoff = super[2];
		sf->pfminfo.os2_supyoff = super[3];
		sf->pfminfo.os2_strikeysize = strike[0];
		sf->pfminfo.os2_strikeypos = strike[1];
	    }
	}
	/* must come after all scaleable fields (linegap, etc.) */
	if ( as!=sf->ascent || des!=sf->descent ) {
	    if ( as+des != sf->ascent+sf->descent && GGadgetIsChecked(GWidgetGetControl(gw,CID_Scale)) )
		SFScaleToEm(sf,as,des);
	    else {
		sf->ascent = as;
		sf->descent = des;
	    }
	    BDFsSetAsDs(sf);
	    reformat_fv = true;
	    CIDMasterAsDes(sf);
	}
	if ( order2!=sf->order2 ) {
	    if ( order2 )
		SFConvertToOrder2(sf);
	    else
		SFConvertToOrder3(sf);
	}
	GFI_ApplyLookupChanges(d);
	if ( retitle_fv ) { FontView *fvs;
	    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame )
		FVSetTitle(fvs);
	}
	if ( reformat_fv )
	    FontViewReformatAll(sf);
	sf->changed = true;
	SFSetModTime(sf);
	sf->changed_since_autosave = true;
	sf->changed_since_xuidchanged = !xuidchanged;
	/* Just in case they changed the blue values and we are showing blues */
	/*  in outline views... */
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    CharView *cv;
	    for ( cv = sf->glyphs[i]->views; cv!=NULL; cv=cv->next ) {
		cv->back_img_out_of_date = true;
		GDrawRequestExpose(cv->v,NULL,false);
	    }
	}
	MacFeatListFree(sf->features);
	sf->features = GGadgetGetUserData(GWidgetGetControl(d->gw,CID_Features));
	last_aspect = d->old_aspect;

	MarkClassFree(sf->mark_class_cnt,sf->mark_classes,sf->mark_class_names);
	sf->mark_class_cnt = d->mark_class_cnt;
	sf->mark_classes = d->mark_classes;
	sf->mark_class_names = d->mark_class_names;

	GFI_Close(d);

	SFReplaceFontnameBDFProps(sf);
    }
return( true );
}

static void GFI_AsDsLab(struct gfi_data *d, int cid) {
    int isoffset = GGadgetIsChecked(GWidgetGetControl(d->gw,cid));
    DBounds b;
    int ocid, labcid;
    double val;
    char buf[40];
    char *offt, *baret;
    int ismax=0;
    unichar_t *end;

    switch ( cid ) {
      case CID_WinAscentIsOff:
	offt = _("Win Ascent Offset:"); baret = _("Win Ascent:");
	ocid = CID_WinAscent; labcid = CID_WinAscentLab;
	ismax = true;
      break;
      case CID_WinDescentIsOff:
	offt = _("Win Descent Offset:"); baret = _("Win Descent:");
	ocid = CID_WinDescent; labcid = CID_WinDescentLab;
      break;
      case CID_TypoAscentIsOff:
	offt = _("Typo Ascent Offset:"); baret = _("Typo Ascent:");
	ocid = CID_TypoAscent; labcid = CID_TypoAscentLab;
	ismax = true;
      break;
      case CID_TypoDescentIsOff:
	offt = _("Typo Descent Offset:"); baret = _("Typo Descent:");
	ocid = CID_TypoDescent; labcid = CID_TypoDescentLab;
      break;
      case CID_HHeadAscentIsOff:
	offt = _("HHead Ascent Offset:"); baret = _("HHead Ascent:");
	ocid = CID_HHeadAscent; labcid = CID_HHeadAscentLab;
	ismax = true;
      break;
      case CID_HHeadDescentIsOff:
	offt = _("HHead Descent Offset:"); baret = _("HHead Descent:");
	ocid = CID_HHeadDescent; labcid = CID_HHeadDescentLab;
      break;
      default:
return;
    }

    GGadgetSetTitle8(GWidgetGetControl(d->gw,labcid),
	    isoffset?offt:baret);
    if ( cid == CID_TypoAscentIsOff ) {
	const unichar_t *as = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	double av=u_strtod(as,&end);
	b.maxy = *end=='\0' ? av : d->sf->ascent;
    } else if ( cid == CID_TypoDescentIsOff ) {
	const unichar_t *ds = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	double dv=u_strtod(ds,&end);
	b.miny = *end=='\0' ? -dv : -d->sf->descent;
    } else {
	CIDFindBounds(d->sf,&b);
	if ( cid == CID_WinDescentIsOff ) b.miny = -b.miny;
    }

    val = u_strtod(_GGadgetGetTitle(GWidgetGetControl(d->gw,ocid)),NULL);
    if ( isoffset )
	sprintf( buf,"%g",rint( val-(ismax ? b.maxy : b.miny)) );
    else
	sprintf( buf,"%g",rint( val+(ismax ? b.maxy : b.miny)) );
    GGadgetSetTitle8(GWidgetGetControl(d->gw,ocid),buf);
}

static int GFI_AsDesIsOff(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GFI_AsDsLab(d,GGadgetGetCid(g));
    }
return( true );
}

static void _GFI_PanoseDefault(struct gfi_data *d) {
    int i;
    int isdefault = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_PanDefault));

    for ( i=0; i<10; ++i ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PanFamily+i),!isdefault);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PanFamilyLab+i),!isdefault);
    }
    if ( isdefault ) {
	char *n = cu_copy(_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname)));
	struct pfminfo info;
	memset(&info,0,sizeof(info));
	SFDefaultOS2Info(&info,d->sf,n);
	free(n);
	for ( i=0; i<10; ++i )
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PanFamily+i),info.panose[i]);
    }
}

static int GFI_PanoseDefault(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	_GFI_PanoseDefault(d);
    }
return( true );
}

static void GFI_SubSuperSet(struct gfi_data *d, struct pfminfo *info) {
    char buffer[40];
    unichar_t ubuf[40];

    sprintf( buffer, "%d", info->os2_subxsize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SubXSize),ubuf);

    sprintf( buffer, "%d", info->os2_subysize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SubYSize),ubuf);

    sprintf( buffer, "%d", info->os2_subxoff );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SubXOffset),ubuf);

    sprintf( buffer, "%d", info->os2_subyoff );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SubYOffset),ubuf);


    sprintf( buffer, "%d", info->os2_supxsize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SuperXSize),ubuf);

    sprintf( buffer, "%d", info->os2_supysize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SuperYSize),ubuf);

    sprintf( buffer, "%d", info->os2_supxoff );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SuperXOffset),ubuf);

    sprintf( buffer, "%d", info->os2_supyoff );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SuperYOffset),ubuf);


    sprintf( buffer, "%d", info->os2_strikeysize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_StrikeoutSize),ubuf);

    sprintf( buffer, "%d", info->os2_strikeypos );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_StrikeoutPos),ubuf);
}

static void _GFI_SubSuperDefault(struct gfi_data *d) {
    int i;
    int isdefault = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_SubSuperDefault));

    for ( i=0; i<10; ++i )
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SubXSize+i),!isdefault);
    if ( isdefault ) {
	const unichar_t *as = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	const unichar_t *ds = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	const unichar_t *ia = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	unichar_t *aend, *dend, *iend;
	double av=u_strtod(as,&aend),dv=u_strtod(ds,&dend),iav=u_strtod(ia,&iend);
	struct pfminfo info;
	if ( *aend!='\0' ) av = d->sf->ascent;
	if ( *dend!='\0' ) dv = d->sf->descent;
	if ( *iend!='\0' ) iav = d->sf->italicangle;
	memset(&info,0,sizeof(info));
	SFDefaultOS2SubSuper(&info,(int) (dv+av), iav);
	GFI_SubSuperSet(d,&info);
    }
}

static int GFI_SubSuperDefault(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	_GFI_SubSuperDefault(d);
    }
return( true );
}

static void TTFSetup(struct gfi_data *d) {
    struct pfminfo info;
    char buffer[10]; unichar_t ubuf[10];
    int i, lg, vlg, tlg;
    const unichar_t *as = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
    const unichar_t *ds = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
    unichar_t *aend, *dend;
    double av=u_strtod(as,&aend),dv=u_strtod(ds,&dend);

    info = d->sf->pfminfo;
    if ( !info.pfmset ) {
	/* Base this stuff on the CURRENT name */
	/* if the user just created a font, and named it *Bold, then the sf */
	/*  won't yet have Bold in its name, and basing the weight on it would*/
	/*  give the wrong answer. That's why we don't do this init until we */
	/*  get to one of the ttf aspects, it gives the user time to set the */
	/*  name properly */
	/* And on CURRENT values of ascent and descent */
	char *n = cu_copy(_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname)));
	if ( *aend=='\0' && *dend=='\0' ) {
	    if ( info.linegap==0 )
		info.linegap = rint(.09*(av+dv));
	    if ( info.vlinegap==0 )
		info.vlinegap = info.linegap;
	    if ( info.os2_typolinegap==0 )
		info.os2_typolinegap = info.linegap;
	}
	lg = info.linegap; vlg = info.vlinegap; tlg = info.os2_typolinegap;
	SFDefaultOS2Info(&info,d->sf,n);
	if ( lg != 0 )
	    info.linegap = lg;
	if ( vlg!= 0 )
	    info.vlinegap = vlg;
	if ( tlg!=0 )
	    info.os2_typolinegap = tlg;
	free(n);
    }

    if ( info.weight>0 && info.weight<=900 && info.weight%100==0 )
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_WeightClass),
		(char *) weightclass[info.weight/100-1].text);
    else {
	sprintf( buffer, "%d", info.weight );
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_WeightClass),buffer);
    }
    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_WidthClass),info.width-1);
    for ( i=0; pfmfamily[i].text!=NULL; ++i )
	if ( (intpt) (pfmfamily[i].userdata)==info.pfmfamily ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PFMFamily),i);
    break;
	}

    if ( d->sf->os2_version>=0 && d->sf->os2_version<=4 )
	GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_OS2Version),d->sf->os2_version);
    else {
	sprintf( buffer,"%d", d->sf->os2_version );
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_OS2Version),buffer);
    }
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_UseTypoMetrics),d->sf->use_typo_metrics);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WeightWidthSlopeOnly),d->sf->weight_width_slope_only);
    
    for ( i=0; ibmfamily[i].text!=NULL; ++i )
	if ( (intpt) (ibmfamily[i].userdata)==info.os2_family_class ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_IBMFamily),i);
    break;
	}
    if ( info.os2_vendor[0]!='\0' ) {
	ubuf[0] = info.os2_vendor[0];
	ubuf[1] = info.os2_vendor[1];
	ubuf[2] = info.os2_vendor[2];
	ubuf[3] = info.os2_vendor[3];
	ubuf[4] = 0;
    } else if ( TTFFoundry!=NULL )
	uc_strncpy(ubuf,TTFFoundry,4);
    else
	uc_strcpy(ubuf,"PfEd");
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Vendor),ubuf);


    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_PanDefault),!info.panose_set);
    _GFI_PanoseDefault(d);
    if ( info.panose_set )
	for ( i=0; i<10; ++i )
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PanFamily+i),info.panose[i]);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_SubSuperDefault),!info.subsuper_set);
    if ( info.subsuper_set )
	GFI_SubSuperSet(d,&info);
    _GFI_SubSuperDefault(d);

    d->ttf_set = true;
    /* FSType is already set */
    sprintf( buffer, "%d", info.linegap );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_LineGap),ubuf);
    sprintf( buffer, "%d", info.vlinegap );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_VLineGap),ubuf);
    sprintf( buffer, "%d", info.os2_typolinegap );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TypoLineGap),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WinAscentIsOff),info.winascent_add);
    GFI_AsDsLab(d,CID_WinAscentIsOff);
    sprintf( buffer, "%d", info.os2_winascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WinDescentIsOff),info.windescent_add);
    GFI_AsDsLab(d,CID_WinDescentIsOff);
    sprintf( buffer, "%d", info.os2_windescent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinDescent),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TypoAscentIsOff),info.typoascent_add);
    GFI_AsDsLab(d,CID_TypoAscentIsOff);
    sprintf( buffer, "%d", info.os2_typoascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TypoAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TypoDescentIsOff),info.typodescent_add);
    GFI_AsDsLab(d,CID_TypoDescentIsOff);
    sprintf( buffer, "%d", info.os2_typodescent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TypoDescent),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_HHeadAscentIsOff),info.hheadascent_add);
    GFI_AsDsLab(d,CID_HHeadAscentIsOff);
    sprintf( buffer, "%d", info.hhead_ascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_HHeadAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_HHeadDescentIsOff),info.hheaddescent_add);
    GFI_AsDsLab(d,CID_HHeadDescentIsOff);
    sprintf( buffer, "%d", info.hhead_descent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_HHeadDescent),ubuf);
}

static char *mathparams[] = {
/* GT: TeX parameters for math fonts. "Num" means numerator, "Denom" */
/* GT: means denominator, "Sup" means superscript, "Sub" means subscript */
    N_("Num1:"),
    N_("Num2:"),  N_("Num3:"), N_("Denom1:"),
    N_("Denom2:"), N_("Sup1:"), N_("Sup2:"), N_("Sup3:"), N_("Sub1:"), N_("Sub2:"),
    N_("SupDrop:"), N_("SubDrop:"), N_("Delim1:"), N_("Delim2:"), N_("Axis Ht:"),
    0 };
static char *mathpopups[] = { N_("Amount to raise baseline for numerators in display styles"),
    N_("Amount to raise baseline for numerators in non-display styles"),
    N_("Amount to raise baseline for numerators in non-display atop styles"),
    N_("Amount to lower baseline for denominators in display styles"),
    N_("Amount to lower baseline for denominators in non-display styles"),
    N_("Amount to raise baseline for superscripts in display styles"),
    N_("Amount to raise baseline for superscripts in non-display styles"),
    N_("Amount to raise baseline for superscripts in modified styles"),
    N_("Amount to lower baseline for subscripts in display styles"),
    N_("Amount to lower baseline for subscripts in non-display styles"),
    N_("Amount above top of large box to place baseline of superscripts"),
    N_("Amount below bottom of large box to place baseline of subscripts"),
    N_("Size of comb delimiters in display styles"), 
    N_("Size of comb delimiters in non-display styles"),
    N_("Height of fraction bar above base line"),
    0 };
/* GT: Default Rule Thickness. A rule being a typographic term for a straight */
/* GT: black line on a printed page. */
static char *extparams[] = { N_("Def Rule Thick:"),
/* GT: I don't really understand these "Big Op Space" things. They have */
/* GT: something to do with TeX and are roughly defined a few strings down */
	N_("Big Op Space1:"),
	N_("Big Op Space2:"),
	N_("Big Op Space3:"),
	N_("Big Op Space4:"),
	N_("Big Op Space5:"), 0 };
static char *extpopups[] = { N_("Default thickness of over and overline bars"),
	N_("The minimum glue space above a large displayed operator"),
	N_("The minimum glue space below a large displayed operator"),
	N_("The minimum distance between a limit's baseline and a large displayed\noperator when the limit is above the operator"),
	N_("The minimum distance between a limit's baseline and a large displayed\noperator when the limit is below the operator"),
	N_("The extra glue place above and below displayed limits"),
	0 };

static int mp_e_h(GWindow gw, GEvent *event) {
    int i;

    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	d->mpdone = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)) {
	    int err=false;
	    double em = (d->sf->ascent+d->sf->descent), val;
	    char **params;
	    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMathSym)) )
		params = mathparams;
	    else
		params = extparams;
	    for ( i=0; params[i]!=0 && !err; ++i ) {
		val = GetReal8(gw,CID_TeX+i,params[i],&err);
		if ( !err )
		    d->texdata.params[i+7] = rint( val/em * (1<<20) );
	    }
	    if ( !err )
		d->mpdone = true;
	} else
	    d->mpdone = true;
    }
return( true );
}

static int GFI_MoreParams(GGadget *g, GEvent *e) {
    int tot;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData txgcd[35];
    GTextInfo txlabel[35];
    int i,y,k;
    char **params, **popups;
    char values[20][20];

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXText)) )
return( true );
	else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMathSym)) ) {
	    tot = 22-7;
	    params = mathparams;
	    popups = mathpopups;
	} else {
	    tot = 13-7;
	    params = extparams;
	    popups = extpopups;
	}

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.is_dlg = true;
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
/* GT: More Parameters */
	wattrs.utf8_window_title = _("More Params");
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,GGadgetScale(180));
	pos.height = GDrawPointsToPixels(NULL,tot*26+60);
	gw = GDrawCreateTopWindow(NULL,&pos,mp_e_h,d,&wattrs);

	memset(&txlabel,0,sizeof(txlabel));
	memset(&txgcd,0,sizeof(txgcd));

	k=0; y = 10;
	for ( i=0; params[i]!=0; ++i ) {
	    txlabel[k].text = (unichar_t *) params[i];
	    txlabel[k].text_is_1byte = true;
	    txgcd[k].gd.label = &txlabel[k];
	    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = y+4;
	    txgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    txgcd[k].gd.popup_msg = (unichar_t *) popups[i];
	    txgcd[k++].creator = GLabelCreate;

	    sprintf( values[i], "%g", d->texdata.params[i+7]*(double) (d->sf->ascent+d->sf->descent)/(double) (1<<20));
	    txlabel[k].text = (unichar_t *) values[i];
	    txlabel[k].text_is_1byte = true;
	    txgcd[k].gd.label = &txlabel[k];
	    txgcd[k].gd.pos.x = 85; txgcd[k].gd.pos.y = y;
	    txgcd[k].gd.pos.width = 75;
	    txgcd[k].gd.flags = gg_visible | gg_enabled;
	    txgcd[k].gd.cid = CID_TeX + i;
	    txgcd[k++].creator = GTextFieldCreate;
	    y += 26;
	}

	txgcd[k].gd.pos.x = 30-3; txgcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
	txgcd[k].gd.pos.width = -1; txgcd[k].gd.pos.height = 0;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
	txlabel[k].text = (unichar_t *) _("_OK");
	txlabel[k].text_is_1byte = true;
	txlabel[k].text_in_resource = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.cid = true;
	txgcd[k++].creator = GButtonCreate;

	txgcd[k].gd.pos.x = -30; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y+3;
	txgcd[k].gd.pos.width = -1; txgcd[k].gd.pos.height = 0;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	txlabel[k].text = (unichar_t *) _("_Cancel");
	txlabel[k].text_is_1byte = true;
	txlabel[k].text_in_resource = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.cid = false;
	txgcd[k++].creator = GButtonCreate;

	txgcd[k].gd.pos.x = 2; txgcd[k].gd.pos.y = 2;
	txgcd[k].gd.pos.width = pos.width-4; txgcd[k].gd.pos.height = pos.height-4;
	txgcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	txgcd[k].creator = GGroupCreate;

	GGadgetsCreate(gw,txgcd);
	d->mpdone = false;
	GDrawSetVisible(gw,true);

	while ( !d->mpdone )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int GFI_TeXChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetGetCid(g)==CID_TeXText ) {
	    GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
/* GT: Extra Space */
		    _("Extra Sp:"));
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),false);
	} else {
	    GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
		    _("Math Sp:"));
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),true);
	}
    }
return( true );
}

static void DefaultTeX(struct gfi_data *d) {
    char buffer[20];
    int i;
    SplineFont *sf = d->sf;

    d->tex_set = true;

    if ( sf->texdata.type==tex_unset ) {
	TeXDefaultParams(sf);
	d->texdata = sf->texdata;
    }

    for ( i=0; i<7; ++i ) {
	sprintf( buffer,"%g", d->texdata.params[i]*(sf->ascent+sf->descent)/(double) (1<<20));
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_TeX+i),buffer);
    }
    if ( sf->texdata.type==tex_math )
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXMathSym), true);
    else if ( sf->texdata.type == tex_mathext )
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXMathExt), true);
    else {
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXText), true);
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
/* GT: Extra Space */
		_("Extra Sp:"));
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),false);
    }
}

static int GFI_MacAutomatic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int autom = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_MacAutomatic));
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MacStyles),!autom);
    }
return( true );
}

static void FigureUnicode(struct gfi_data *d) {
    int includeempties = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_UnicodeEmpties));
    GGadget *list = GWidgetGetControl(d->gw,CID_Unicode);
    struct rangeinfo *ri;
    int cnt, i;
    GTextInfo **ti;
    char buffer[200];

    GGadgetClearList(list);
    ri = SFUnicodeRanges(d->sf,(includeempties?ur_includeempty:0)|ur_sortbyunicode);
    if ( ri==NULL ) cnt=0;
    else
	for ( cnt=0; ri[cnt].range!=NULL; ++cnt );

    ti = galloc((cnt+1) * sizeof( GTextInfo * ));
    for ( i=0; i<cnt; ++i ) {
	if ( ri[i].range->first==-1 )
	    snprintf( buffer, sizeof(buffer),
		    "%s  %d/0", _(ri[i].range->name), ri[i].cnt);
	else
	    snprintf( buffer, sizeof(buffer),
		    "%s  U+%04X-U+%04X %d/%d",
		    _(ri[i].range->name),
		    (int) ri[i].range->first, (int) ri[i].range->last,
		    ri[i].cnt, ri[i].range->actual );
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	ti[i]->text = utf82u_copy(buffer);
	ti[i]->userdata = ri[i].range;
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,ti,false);
    free(ri);
}

static int GFI_UnicodeRangeChange(GGadget *g, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    GTextInfo *ti = GGadgetGetListItemSelected(g);
    struct unicoderange *r;
    int gid, first=-1;
    SplineFont *sf = d->sf;
    FontView *fv = sf->fv;
    EncMap *map = fv->map;
    int i, enc;

    if ( ti==NULL )
return( true );
    if ( e->type!=et_controlevent ||
	    (e->u.control.subtype != et_listselected &&e->u.control.subtype != et_listdoubleclick))
return( true );

    r = ti->userdata;

    for ( i=0; i<map->enccount; ++i )
	fv->selected[i] = 0;

    if ( e->u.control.subtype == et_listselected ) {
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	    enc = map->backmap[gid];
	    if ( sf->glyphs[gid]->unicodeenc>=r->first && sf->glyphs[gid]->unicodeenc<=r->last &&
		    enc!=-1 ) {
		if ( first==-1 || enc<first ) first = enc;
		fv->selected[enc] = true;
	    }
	}
    } else if ( e->u.control.subtype == et_listdoubleclick && !r->unassigned ) {
	char *found = gcalloc(r->last-r->first+1,1);
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	    int u = sf->glyphs[gid]->unicodeenc;
	    if ( u>=r->first && u<=r->last ) {
		found[u-r->first] = true;
	    }
	}
	for ( i=0; i<=r->last-r->first; ++i ) {
	    if ( isunicodepointassigned(i+r->first) && !found[i] ) {
		enc = EncFromUni(i+r->first,map->enc);
		if ( enc!=-1 ) {
		    if ( first==-1 || enc<first ) first = enc;
		    fv->selected[enc] = true;
		}
	    }
	}
	free(found);
    }
    if ( first==-1 ) {
	GDrawBeep(NULL);
    } else {
	FVScrollToChar(fv,first);
    }
    GDrawRequestExpose(fv->v,NULL,false);
return( true );
}

static int GFI_UnicodeEmptiesChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	FigureUnicode(d);
    }
return( true );
}

static int GFI_AspectChange(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int new_aspect = GTabSetGetSel(g);
	int rows;

	if ( d->old_aspect == d->tn_aspect )
	    GMatrixEditGet(GWidgetGetControl(d->gw,CID_TNames), &rows);
	if ( !d->ttf_set && new_aspect == d->ttfv_aspect )
	    TTFSetup(d);
	else if ( !d->names_set && new_aspect == d->tn_aspect ) {
	    TTFNames_Resort(d);
	    d->names_set = true;
	} else if ( !d->tex_set && new_aspect == d->tx_aspect )
	    DefaultTeX(d);
	else if ( new_aspect == d->unicode_aspect )
	    FigureUnicode(d);
	d->old_aspect = new_aspect;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	GFI_CancelClose(d);
    } else if ( event->type==et_destroy ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	free(d);
    } else if ( event->type==et_char ) {
return( GFI_Char(GDrawGetUserData(gw),event));
    }
return( true );
}

static void GFI_InitMarkClasses(struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i;

    d->mark_class_cnt = sf->mark_class_cnt;
    if ( d->mark_class_cnt!=0 ) {
	d->mark_classes = galloc(d->mark_class_cnt*sizeof(char *));
	d->mark_class_names = galloc(d->mark_class_cnt*sizeof(unichar_t *));
	d->mark_classes[0] = NULL; d->mark_class_names[0] = NULL;
	for ( i=1; i<d->mark_class_cnt; ++i ) {
	    d->mark_classes[i] = copy(sf->mark_classes[i]);
	    d->mark_class_names[i] = copy(sf->mark_class_names[i]);
	}
    }
}

static void LookupSetup(struct lkdata *lk,OTLookup *lookups) {
    int cnt, subcnt;
    OTLookup *otl;
    struct lookup_subtable *sub;

    for ( cnt=0, otl=lookups; otl!=NULL; ++cnt, otl=otl->next );
    lk->cnt = cnt; lk->max = cnt+10;
    lk->all = gcalloc(lk->max,sizeof(struct lkinfo));
    for ( cnt=0, otl=lookups; otl!=NULL; ++cnt, otl=otl->next ) {
	lk->all[cnt].lookup = otl;
	for ( subcnt=0, sub=otl->subtables; sub!=NULL; ++subcnt, sub=sub->next );
	lk->all[cnt].subtable_cnt = subcnt; lk->all[cnt].subtable_max = subcnt+10;
	lk->all[cnt].subtables = gcalloc(lk->all[cnt].subtable_max,sizeof(struct lksubinfo));
	for ( subcnt=0, sub=otl->subtables; sub!=NULL; ++subcnt, sub=sub->next )
	    lk->all[cnt].subtables[subcnt].subtable = sub;
    }
}

static void LookupInfoFree(struct lkdata *lk) {
    int cnt;

    for ( cnt=0; cnt<lk->cnt; ++cnt )
	free(lk->all[cnt].subtables);
    free(lk->all);
}

#define LK_MARGIN 2

struct selection_bits {
    int lookup_cnt, sub_cnt;	/* Number of selected lookups, and selected sub tables */
    int a_lookup, a_sub;	/* The index of one of those lookups, or subtables */
    int a_sub_lookup;		/*  the index of the lookup containing a_sub */
    int any_first, any_last;	/* Whether any of the selected items is first or last in its catagory */
    int sub_table_mergeable;	/* Can we merge the selected subtables? */
    int lookup_mergeable;	/* Can we merge the selected lookups? */
};

static void LookupParseSelection(struct lkdata *lk, struct selection_bits *sel) {
    int lookup_cnt, sub_cnt, any_first, any_last, all_one_lookup;
    int a_lookup, a_sub, a_sub_lookup;
    int sub_mergeable, lookup_mergeable;
    int i,j;

    lookup_cnt = sub_cnt = any_first = any_last = 0;
    all_one_lookup = a_lookup = a_sub = a_sub_lookup = -1;
    sub_mergeable = lookup_mergeable = true;
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( lk->all[i].selected ) {
	    ++lookup_cnt;
	    if ( a_lookup==-1 )
		a_lookup = i;
	    else if ( lk->all[i].lookup->lookup_type!=lk->all[a_lookup].lookup->lookup_type ||
		    lk->all[i].lookup->lookup_flags!=lk->all[a_lookup].lookup->lookup_flags )
		lookup_mergeable = false;
	    if ( i==0 ) any_first=true;
	    if ( i==lk->cnt-1 ) any_last=true;
	}
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( lk->all[i].subtables[j].selected ) {
		    ++sub_cnt;
		    if ( a_sub==-1 ) {
			a_sub = j; a_sub_lookup = i;
		    }
		    if ( j==0 ) any_first = true;
		    if ( j==lk->all[i].subtable_cnt-1 ) any_last = true;
		    if ( lk->all[i].subtables[j].subtable->kc!=NULL ||
			    lk->all[i].subtables[j].subtable->fpst!=NULL ||
			    lk->all[i].subtables[j].subtable->sm!=NULL )
			sub_mergeable = false;
		    if ( all_one_lookup==-1 )
			all_one_lookup = i;
		    else if ( all_one_lookup!=i )
			all_one_lookup = -2;
		}
	    }
	}
    }

    sel->lookup_cnt = lookup_cnt;
    sel->sub_cnt = sub_cnt;
    sel->a_lookup = a_lookup;
    sel->a_sub = a_sub;
    sel->a_sub_lookup = a_sub_lookup;
    sel->any_first = any_first;
    sel->any_last = any_last;
    sel->sub_table_mergeable = sub_mergeable && all_one_lookup && sub_cnt>=2 && lookup_cnt==0;
    sel->lookup_mergeable = lookup_mergeable && lookup_cnt>=2 && sub_cnt==0;
}

void GFI_LookupEnableButtons(struct gfi_data *gfi, int isgpos) {
    struct lkdata *lk = &gfi->tables[isgpos];
    struct selection_bits sel;
    FontView *ofv;

    LookupParseSelection(lk,&sel);

    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupTop),!sel.any_first &&
	    sel.lookup_cnt+sel.sub_cnt==1);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupUp),!sel.any_first &&
	    sel.lookup_cnt+sel.sub_cnt!=0);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupDown),!sel.any_last &&
	    sel.lookup_cnt+sel.sub_cnt!=0);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupBottom),!sel.any_last &&
	    sel.lookup_cnt+sel.sub_cnt==1);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_AddLookup),
	    (sel.lookup_cnt+sel.sub_cnt<=1));
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_AddSubtable),
	    (sel.lookup_cnt==1 && sel.sub_cnt<=1) || (sel.lookup_cnt==0 && sel.sub_cnt==1));
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_EditMetadata),
	    (sel.lookup_cnt==1 && sel.sub_cnt==0) ||
	    (sel.lookup_cnt==0 && sel.sub_cnt==1));
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_EditSubtable),sel.lookup_cnt==0 &&
	    sel.sub_cnt==1);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_DeleteLookup),sel.lookup_cnt!=0 ||
	    sel.sub_cnt!=0);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_MergeLookup),
	    (sel.lookup_cnt>=2 && sel.sub_cnt==0 && sel.lookup_mergeable) ||
	    (sel.lookup_cnt==0 && sel.sub_cnt>=2 && sel.sub_table_mergeable));
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_RevertLookups),true);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupSort),lk->cnt>1 );

    for ( ofv=fv_list; ofv!=NULL; ofv = ofv->next ) {
	SplineFont *osf = ofv->sf;
	if ( osf->cidmaster ) osf = osf->cidmaster;
	if ( osf==gfi->sf || gfi->sf->cidmaster==osf )
    continue;
	if ( (isgpos && osf->gpos_lookups!=NULL) || (!isgpos && osf->gsub_lookups!=NULL) )
    break;
    }
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_ImportLookups),ofv!=NULL);
}

void GFI_LookupScrollbars(struct gfi_data *gfi, int isgpos, int refresh) {
    int lcnt, i,j;
    int width=0, wmax;
    GWindow gw = GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos));
    struct lkdata *lk = &gfi->tables[isgpos];
    GGadget *vsb = GWidgetGetControl(gfi->gw,CID_LookupVSB+isgpos);
    GGadget *hsb = GWidgetGetControl(gfi->gw,CID_LookupHSB+isgpos);
    int off_top, off_left;

    GDrawSetFont(gw,gfi->font);
    lcnt = 0;
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	++lcnt;
	wmax = GDrawGetText8Width(gw,lk->all[i].lookup->lookup_name,-1,NULL);
	if ( wmax > width ) width = wmax;
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		++lcnt;
		wmax = gfi->fh+GDrawGetText8Width(gw,lk->all[i].subtables[j].subtable->subtable_name,-1,NULL);
		if ( wmax > width ) width = wmax;
	    }
	}
    }
    width += gfi->fh;
    GScrollBarSetBounds(vsb,0,lcnt,(gfi->lkheight-2*LK_MARGIN)/gfi->fh);
    GScrollBarSetBounds(hsb,0,width,gfi->lkwidth-2*LK_MARGIN);
    off_top = lk->off_top;
    if ( off_top+((gfi->lkheight-2*LK_MARGIN)/gfi->fh) > lcnt )
	off_top = lcnt - (gfi->lkheight-2*LK_MARGIN)/gfi->fh;
    if ( off_top<0 )
	off_top  = 0;
    off_left = lk->off_left;
    if ( off_left+gfi->lkwidth-2*LK_MARGIN > width )
	off_left = width-(gfi->lkwidth-2*LK_MARGIN);
    if ( off_left<0 )
	off_left  = 0;
    if ( off_top!=lk->off_top || off_left!=lk->off_left ) {
	lk->off_top = off_top; lk->off_left = off_left;
	GScrollBarSetPos(vsb,off_top);
	GScrollBarSetPos(hsb,off_left);
	refresh = true;
    }
    if ( refresh )
	GDrawRequestExpose(gw,NULL,true);
}

static int LookupsHScroll(GGadget *g,GEvent *event) {
    struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
    int isgpos = GGadgetGetCid(g)-CID_LookupHSB;
    struct lkdata *lk = &gfi->tables[isgpos];
    int newpos = lk->off_left;
    int32 sb_min, sb_max, sb_pagesize;

    if ( event->type!=et_controlevent || event->u.control.subtype != et_scrollbarchange )
return( true );

    GScrollBarGetBounds(event->u.control.g,&sb_min,&sb_max,&sb_pagesize);
    switch( event->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*sb_pagesize/10;
      break;
      case et_sb_up:
        newpos -= sb_pagesize/15;
      break;
      case et_sb_down:
        newpos += sb_pagesize/15;
      break;
      case et_sb_downpage:
        newpos += 9*sb_pagesize/10;
      break;
      case et_sb_bottom:
        newpos = sb_max-sb_pagesize;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = event->u.control.u.sb.pos;
      break;
      case et_sb_halfup:
        newpos -= sb_pagesize/30;
      break;
      case et_sb_halfdown:
        newpos += sb_pagesize/30;
      break;
    }
    if ( newpos>sb_max-sb_pagesize )
        newpos = sb_max-sb_pagesize;
    if ( newpos<0 ) newpos = 0;
    if ( newpos!=lk->off_left ) {
	lk->off_left = newpos;
	GScrollBarSetPos(event->u.control.g,newpos);
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,true);
    }
return( true );
}

static int LookupsVScroll(GGadget *g,GEvent *event) {
    struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
    int isgpos = GGadgetGetCid(g)-CID_LookupVSB;
    struct lkdata *lk = &gfi->tables[isgpos];
    int newpos = lk->off_top;
    int32 sb_min, sb_max, sb_pagesize;

    if ( event->type!=et_controlevent || event->u.control.subtype != et_scrollbarchange )
return( true );

    GScrollBarGetBounds(event->u.control.g,&sb_min,&sb_max,&sb_pagesize);
    switch( event->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*sb_pagesize/10;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += 9*sb_pagesize/10;
      break;
      case et_sb_bottom:
        newpos = (sb_max-sb_pagesize);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = event->u.control.u.sb.pos;
      break;
    }
    if ( newpos>(sb_max-sb_pagesize) )
        newpos = (sb_max-sb_pagesize);
    if ( newpos<0 ) newpos = 0;
    if ( newpos!=lk->off_top ) {
	/*int diff = newpos-lk->off_top;*/
	lk->off_top = newpos;
	GScrollBarSetPos(event->u.control.g,newpos);
	/*GDrawScroll(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,0,diff*gfi->fh);*/
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,true);
    }
return( true );
}

static int GFI_LookupOrder(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j,k;
	struct lkinfo temp;
	struct lksubinfo temp2;
	int cid = GGadgetGetCid(g);
	GWindow gw = GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos));

	if ( cid==CID_LookupTop ) {
	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected ) {
		    temp = lk->all[i];
		    for ( k=i-1; k>=0; --k )
			lk->all[k+1] = lk->all[k];
		    lk->all[0] = temp;
    goto done;
		}
		if ( lk->all[i].open ) {
		    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
			if ( lk->all[i].subtables[j].deleted )
		    continue;
			if ( lk->all[i].subtables[j].selected ) {
			    temp2 = lk->all[i].subtables[j];
			    for ( k=j-1; k>=0; --k )
				lk->all[i].subtables[k+1] = lk->all[i].subtables[k];
			    lk->all[i].subtables[0] = temp2;
    goto done;
			}
		    }
		}
	    }
	} else if ( cid==CID_LookupBottom ) {
	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected ) {
		    temp = lk->all[i];
		    for ( k=i; k<lk->cnt-1; --k )
			lk->all[k] = lk->all[k+1];
		    lk->all[lk->cnt-1] = temp;
    goto done;
		}
		if ( lk->all[i].open ) {
		    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
			if ( lk->all[i].subtables[j].deleted )
		    continue;
			if ( lk->all[i].subtables[j].selected ) {
			    temp2 = lk->all[i].subtables[j];
			    for ( k=j; k<lk->all[i].subtable_cnt-1; --k )
				lk->all[i].subtables[k] = lk->all[i].subtables[k+1];
			    lk->all[i].subtables[lk->all[i].subtable_cnt-1] = temp2;
    goto done;
			}
		    }
		}
	    }
	} else if ( cid==CID_LookupUp ) {
	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected && i!=0 ) {
		    temp = lk->all[i];
		    lk->all[i] = lk->all[i-1];
		    lk->all[i-1] = temp;
		}
		if ( lk->all[i].open ) {
		    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
			if ( lk->all[i].subtables[j].deleted )
		    continue;
			if ( lk->all[i].subtables[j].selected && j!=0 ) {
			    temp2 = lk->all[i].subtables[j];
			    lk->all[i].subtables[j] = lk->all[i].subtables[j-1];
			    lk->all[i].subtables[j-1] = temp2;
			}
		    }
		}
	    }
	} else if ( cid==CID_LookupDown ) {
	    for ( i=lk->cnt-1; i>=0; --i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected && i!=lk->cnt-1 ) {
		    temp = lk->all[i];
		    lk->all[i] = lk->all[i+1];
		    lk->all[i+1] = temp;
		}
		if ( lk->all[i].open ) {
		    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
			if ( lk->all[i].subtables[j].deleted )
		    continue;
			if ( lk->all[i].subtables[j].selected && j!=lk->all[i].subtable_cnt-1 ) {
			    temp2 = lk->all[i].subtables[j];
			    lk->all[i].subtables[j] = lk->all[i].subtables[j+1];
			    lk->all[i].subtables[j+1] = temp2;
			}
		    }
		}
	    }
	}
    done:
	GFI_LookupEnableButtons(gfi,isgpos);
	GDrawRequestExpose(gw,NULL,true);
    }
return( true );
}

static int GFI_LookupSort(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	struct lkinfo temp;
	int i,j;

	for ( i=0; i<lk->cnt; ++i ) {
	    int order = FeatureOrderId(isgpos,lk->all[i].lookup->features);
	    for ( j=i+1; j<lk->cnt; ++j ) {
		int jorder = FeatureOrderId(isgpos,lk->all[j].lookup->features);
		if ( order>jorder) {
		    temp = lk->all[i];
		    lk->all[i] = lk->all[j];
		    lk->all[j] = temp;
		    order = jorder;
		}
	    }
	}
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

/* ??? *//* How about a series of buttons to show only by lookup_type, feat-tag, script-tag */

void GFI_CCDEnd(struct gfi_data *d) {

    d->ccd = NULL;
}

void GFI_FinishContextNew(struct gfi_data *d,FPST *fpst, int success) {
    OTLookup *otl;
    struct lookup_subtable *sub, *prev;
    FPST *ftest, *fprev;

    if ( !success ) {
	/* We can't allow incomplete FPSTs to float around */
	/* If they didn't fill it in, delete it */
	otl = fpst->subtable->lookup;
	prev = NULL;
	for ( sub=otl->subtables; sub!=NULL && sub!=fpst->subtable; prev = sub, sub=sub->next );
	if ( sub!=NULL ) {
	    if ( prev==NULL )
		otl->subtables = sub->next;
	    else
		prev->next = sub->next;
	    free(sub->subtable_name);
	    chunkfree(sub,sizeof(struct lookup_subtable));
	}
	fprev = NULL;
	for ( ftest=d->sf->possub; ftest!=NULL && ftest!=fpst; fprev = ftest, ftest=ftest->next );
	if ( ftest!=NULL ) {
	    if ( fprev==NULL )
		d->sf->possub = fpst->next;
	    else
		fprev->next = fpst->next;
	}

	chunkfree(fpst,sizeof(FPST));
    }
}

void GFI_SMDEnd(struct gfi_data *d) {

    d->smd = NULL;
}

void GFI_FinishSMNew(struct gfi_data *d,ASM *sm, int success, int isnew) {
    OTLookup *otl;
    struct lookup_subtable *sub, *prev;
    ASM *smtest, *smprev;

    if ( !success && isnew ) {
	/* We can't allow incomplete state machines floating around */
	/* If they didn't fill it in, delete it */
	otl = sm->subtable->lookup;
	prev = NULL;
	for ( sub=otl->subtables; sub!=NULL && sub!=sm->subtable; prev = sub, sub=sub->next );
	if ( sub!=NULL ) {
	    if ( prev==NULL )
		otl->subtables = sub->next;
	    else
		prev->next = sub->next;
	    free(sub->subtable_name);
	    chunkfree(sub,sizeof(struct lookup_subtable));
	}
	smprev = NULL;
	for ( smtest=d->sf->sm; smtest!=NULL && smtest!=sm; smprev = smtest, smtest=smtest->next );
	if ( smtest!=NULL ) {
	    if ( smprev==NULL )
		d->sf->sm = sm->next;
	    else
		smprev->next = sm->next;
	}
	chunkfree(sm,sizeof(ASM));
    }
}

static void LookupSubtableContents(struct gfi_data *gfi,int isgpos) {
    struct lkdata *lk = &gfi->tables[isgpos];
    int i,j;

    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( lk->all[i].subtables[j].selected ) {
		    _LookupSubtableContents(gfi->sf,lk->all[i].subtables[j].subtable,NULL);
return;
		}
	    }
	}
    }
}

static int GFI_LookupEditSubtableContents(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	LookupSubtableContents(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupAddLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j,k,lcnt;
	OTLookup *otl = chunkalloc(sizeof(OTLookup));

	if ( !EditLookup(otl,isgpos,gfi->sf)) {
	    chunkfree(otl,sizeof(OTLookup));
return( true );
	}
	for ( i=lk->cnt-1; i>=0; --i ) {
	    if ( !lk->all[i].deleted && lk->all[i].selected ) {
		lk->all[i].selected = false;
	break;
	    }
	    if ( !lk->all[i].deleted && lk->all[i].open ) {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j )
		    if ( !lk->all[i].subtables[j].deleted &&
			    lk->all[i].subtables[j].selected ) {
			lk->all[i].subtables[j].selected = false;
		break;
		    }
		if ( j<lk->all[i].subtable_cnt )
	break;
	    }
	}
	if ( lk->cnt>=lk->max )
	    lk->all = grealloc(lk->all,(lk->max+=10)*sizeof(struct lkinfo));
	for ( k=lk->cnt; k>i+1; --k )
	    lk->all[k] = lk->all[k-1];
	memset(&lk->all[k],0,sizeof(struct lkinfo));
	lk->all[k].lookup = otl;
	lk->all[k].new = true;
	lk->all[k].selected = true;
	++lk->cnt;
	if ( isgpos ) {
	    otl->next = gfi->sf->gpos_lookups;
	    gfi->sf->gpos_lookups = otl;
	} else {
	    otl->next = gfi->sf->gsub_lookups;
	    gfi->sf->gsub_lookups = otl;
	}

	/* Make sure the window is scrolled to display the new lookup */
	lcnt=0;
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    if ( i==k )
	break;
	    ++lcnt;
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		++lcnt;
	    }
	}
	if ( lcnt<lk->off_top || lcnt>=lk->off_top+(gfi->lkheight-2*LK_MARGIN)/gfi->fh )
	    lk->off_top = lcnt;

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupAddSubtable(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j,k,lcnt;
	struct lookup_subtable *sub;

	lcnt = 0;
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    j = -1;
	    ++lcnt;
	    if ( lk->all[i].selected )
	break;
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		++lcnt;
		if ( lk->all[i].subtables[j].selected )
	goto break_2_loops;
	    }
	}
	break_2_loops:
	if ( i==lk->cnt )
return( true );

	sub = chunkalloc(sizeof(struct lookup_subtable));
	sub->lookup = lk->all[i].lookup;
	if ( !EditSubtable(sub,isgpos,gfi->sf,NULL)) {
	    chunkfree(sub,sizeof(struct lookup_subtable));
return( true );
	}
	if ( lk->all[i].subtable_cnt>=lk->all[i].subtable_max )
	    lk->all[i].subtables = grealloc(lk->all[i].subtables,(lk->all[i].subtable_max+=10)*sizeof(struct lksubinfo));
	for ( k=lk->all[i].subtable_cnt; k>j+1; --k )
	    lk->all[i].subtables[k] = lk->all[i].subtables[k-1];
	memset(&lk->all[i].subtables[k],0,sizeof(struct lksubinfo));
	lk->all[i].subtables[k].subtable = sub;
	lk->all[i].subtables[k].new = true;
	sub->next = lk->all[i].lookup->subtables;
	lk->all[i].lookup->subtables = sub;
	++lk->all[i].subtable_cnt;

	/* Make sure the window is scrolled to display the new subtable */
	if ( lcnt<lk->off_top || lcnt>=lk->off_top+(gfi->lkheight-2*LK_MARGIN)/gfi->fh )
	    lk->off_top = lcnt;

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupEditMetadata(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j;

	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    if ( lk->all[i].selected ) {
		EditLookup(lk->all[i].lookup,isgpos,gfi->sf);
return( true );
	    } else if ( lk->all[i].open ) {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		    if ( lk->all[i].subtables[j].deleted )
		continue;
		    if ( lk->all[i].subtables[j].selected ) {
			EditSubtable(lk->all[i].subtables[j].subtable,isgpos,gfi->sf,NULL);
return( true );
		    }
		}
	    }
	}
    }
return( true );
}

static int GFI_LookupMergeLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	char *buts[3];
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	struct selection_bits sel;
	int i,j;
	struct lkinfo *lkfirst;
	struct lksubinfo *sbfirst;
	struct lookup_subtable *sub;

	LookupParseSelection(lk,&sel);
	if ( !sel.sub_table_mergeable && !sel.lookup_mergeable )
return( true );
	
	buts[0] = _("Do it");
	buts[1] = _("_Cancel");
	buts[2] = NULL;
	if ( gwwv_ask(_("Cannot be Undone"),(const char **) buts,0,1,_("The Merge operation cannot be reverted.\nDo it anyway?"))==1 )
return( true );
	if ( sel.lookup_mergeable ) {
	    lkfirst = NULL;
	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].selected && !lk->all[i].deleted ) {
		    if ( lkfirst==NULL )
			lkfirst = &lk->all[i];
		    else {
			FLMerge(lkfirst->lookup,lk->all[i].lookup);
			if ( lkfirst->subtable_cnt+lk->all[i].subtable_cnt >= lkfirst->subtable_max )
			    lkfirst->subtables = grealloc(lkfirst->subtables,(lkfirst->subtable_max+=lk->all[i].subtable_cnt)*sizeof(struct lksubinfo));
			memcpy(lkfirst->subtables+lkfirst->subtable_cnt,
				lk->all[i].subtables,lk->all[i].subtable_cnt*sizeof(struct lksubinfo));
			lkfirst->subtable_cnt += lk->all[i].subtable_cnt;
			for ( j=0; j<lk->all[i].subtable_cnt; ++j )
			    lk->all[i].subtables[j].subtable->lookup = lkfirst->lookup;
			if ( lk->all[i].lookup->subtables!=NULL ) {
			    for ( sub = lk->all[i].lookup->subtables; sub->next!=NULL; sub = sub->next );
			    sub->next = lkfirst->lookup->subtables;
			    lkfirst->lookup->subtables = lk->all[i].lookup->subtables;
			    lk->all[i].lookup->subtables = NULL;
			}
			lk->all[i].subtable_cnt = 0;
			lk->all[i].deleted = true;
			lk->all[i].open = false;
			lk->all[i].selected = false;
		    }
		}
	    }
	} else if ( sel.sub_table_mergeable ) {
	    sbfirst = NULL;
	    for ( i=0; i<lk->cnt; ++i ) if ( !lk->all[i].deleted && lk->all[i].open ) {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j ) if ( !lk->all[i].subtables[j].deleted ) {
		    if ( lk->all[i].subtables[j].selected ) {
			if ( sbfirst == NULL )
			    sbfirst = &lk->all[i].subtables[j];
			else {
			    SFSubTablesMerge(gfi->sf,sbfirst->subtable,lk->all[i].subtables[j].subtable);
			    lk->all[i].subtables[j].deleted = true;
			    lk->all[i].subtables[j].selected = false;
			}
		    }
		}
		if ( sbfirst!=NULL )	/* Can only merge subtables within a lookup, so if we found anything, in a lookup that's everything */
	    break;
	    }
	}
	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupDeleteLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j;

	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    if ( lk->all[i].selected )
		lk->all[i].deleted = true;
	    else if ( lk->all[i].open ) {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		    if ( lk->all[i].subtables[j].deleted )
		continue;
		    if ( lk->all[i].subtables[j].selected )
			lk->all[i].subtables[j].deleted = true;
		}
	    }
	}

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }

return( true );
}

static int GFI_LookupRevertLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j;

	/* First remove any new lookups, subtables */
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].new )
		SFRemoveLookup(gfi->sf,lk->all[i].lookup);
	    else {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j )
		    if ( lk->all[i].subtables[j].new )
			SFRemoveLookupSubTable(gfi->sf,lk->all[i].subtables[j].subtable);
	    }
	}

	/* Now since we didn't actually delete anything we don't need to do */
	/*  anything to resurrect them */

	/* Finally we need to restore the original order. */
	/* But that just means regenerating the lk structure. So free it and */
	/*  regenerate it */

	LookupInfoFree(lk);
	LookupSetup(lk,isgpos?gfi->sf->gpos_lookups:gfi->sf->gsub_lookups);

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int import_e_h(GWindow gw, GEvent *event) {
    int *done = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	*done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("fontinfo.html#Lookups");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	switch ( GGadgetGetCid(event->u.control.g)) {
	  case CID_OK:
	    *done = 2;
	  break;
	  case CID_Cancel:
	    *done = true;
	  break;
	}
    }
return( true );
}

static int GFI_LookupImportLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	FontView *ofv;
	SplineFont *osf;
	OTLookup *otl;
	int i, j, cnt;
	GTextInfo *ti;
	GGadgetCreateData gcd[7], *varray[8], *harray[7];
	GTextInfo label[7];
	GWindowAttrs wattrs;
	GRect pos;
	GWindow gw;
	int done = 0;

	/* Figure out what lookups can be imported from which (open) fonts */
	ti = NULL;
	for ( j=0; j<2; ++j ) {
	    for ( ofv=fv_list; ofv!=NULL; ofv=ofv->next ) {
		osf = ofv->sf;
		if ( osf->cidmaster ) osf = osf->cidmaster;
		osf->ticked = false;
	    }
	    cnt = 0;
	    for ( ofv=fv_list; ofv!=NULL; ofv=ofv->next ) {
		osf = ofv->sf;
		if ( osf->cidmaster ) osf = osf->cidmaster;
		if ( osf->ticked || osf==gfi->sf || osf==gfi->sf->cidmaster ||
			( isgpos && osf->gpos_lookups==NULL) ||
			(!isgpos && osf->gsub_lookups==NULL) )
	    continue;
		osf->ticked = true;
		if ( cnt!=0 ) {
		    if ( ti )
			ti[cnt].line = true;
		    ++cnt;
		}
		if ( ti ) {
		    ti[cnt].text = (unichar_t *) copy( osf->fontname );
		    ti[cnt].text_is_1byte = true;
		    ti[cnt].disabled = true;
		    ti[cnt].userdata = osf;
		}
		++cnt;
		for ( otl = isgpos ? osf->gpos_lookups : osf->gsub_lookups; otl!=NULL; otl=otl->next ) {
		    if ( ti ) {
			ti[cnt].text = (unichar_t *) strconcat( " ", otl->lookup_name );
			ti[cnt].text_is_1byte = true;
			ti[cnt].userdata = otl;
		    }
		    ++cnt;
		}
	    }
	    if ( ti==NULL )
		ti = gcalloc((cnt+1),sizeof(GTextInfo));
	}

	memset(gcd,0,sizeof(gcd));
	memset(label,0,sizeof(label));

	i = 0;
	label[i].text = (unichar_t *) _("Select lookups from other fonts");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = 6+6; 
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i].creator = GLabelCreate;
	varray[0] = &gcd[i++]; varray[1] = NULL;

	gcd[i].gd.pos.height = 12*12+6;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_list_multiplesel|gg_utf8_popup;
	gcd[i].gd.u.list = ti;
	gcd[i].creator = GListCreate;
	varray[2] = &gcd[i++]; varray[3] = NULL;

	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_Import");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_OK;
	harray[0] = GCD_Glue; harray[1] = &gcd[i]; harray[2] = GCD_Glue;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_Cancel;
	harray[3] = GCD_Glue; harray[4] = &gcd[i]; harray[5] = GCD_Glue; harray[6] = NULL;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.boxelements = harray;
	gcd[i].creator = GHBoxCreate;
	varray[4] = &gcd[i++]; varray[5] = NULL; varray[6] = NULL;

	gcd[i].gd.pos.x = gcd[i].gd.pos.y = 2;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.boxelements = varray;
	gcd[i].creator = GHVGroupCreate;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Import Lookup...");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
	pos.height = GDrawPointsToPixels(NULL,193);
	gw = GDrawCreateTopWindow(NULL,&pos,import_e_h,&done,&wattrs);

	GGadgetsCreate(gw,&gcd[i]);
	GHVBoxSetExpandableRow(gcd[i].ret,1);
	GHVBoxSetExpandableCol(gcd[i-1].ret,gb_expandgluesame);
	GHVBoxFitWindow(gcd[i].ret);
	GTextInfoListFree(ti);
	GDrawSetVisible(gw,true);
 
	while ( !done )
	    GDrawProcessOneEvent(NULL);
	if ( done==2 ) {
	    int32 len;
	    GTextInfo **ti = GGadgetGetList(gcd[1].ret,&len);
	    osf = NULL;
	    for ( i=0; i<len; ++i ) {
		if ( ti[i]->disabled )
		    osf = ti[i]->userdata;
		else if ( ti[i]->selected && ti[i]->text!=NULL )
		    OTLookupCopyInto(gfi->sf,osf,(OTLookup *) ti[i]->userdata);
	    }
	}
	GDrawDestroyWindow(gw);

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupAspectChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static void LookupExpose(GWindow pixmap, struct gfi_data *gfi, int isgpos) {
    int lcnt, i,j;
    struct lkdata *lk = &gfi->tables[isgpos];
    GRect r, old;

    r.x = LK_MARGIN; r.width = gfi->lkwidth-2*LK_MARGIN;
    r.y = LK_MARGIN; r.height = gfi->lkheight-2*LK_MARGIN;
    GDrawPushClip(pixmap,&r,&old);
    GDrawSetFont(pixmap,gfi->font);

    lcnt = 0;
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( lcnt>=lk->off_top ) {
	    if ( lk->all[i].selected ) {
		r.x = LK_MARGIN; r.width = gfi->lkwidth-2*LK_MARGIN;
		r.y = (lcnt-lk->off_top)*gfi->fh; r.height = gfi->fh;
		GDrawFillRect(pixmap,&r,0xffff00);
	    }
	    r.x = LK_MARGIN-lk->off_left; r.width = (gfi->as&~1);
	    r.y = LK_MARGIN+(lcnt-lk->off_top)*gfi->fh; r.height = r.width;
	    GDrawDrawRect(pixmap,&r,0x000000);
	    GDrawDrawLine(pixmap,r.x+2,r.y+(r.height/2), r.x+r.width-2,r.y+(r.height/2), 0x000000);
	    if ( !lk->all[i].open )
		GDrawDrawLine(pixmap,r.x+(r.width/2),r.y+2, r.x+(r.width/2),r.y+r.height-2, 0x000000);
	    GDrawDrawText8(pixmap,r.x+gfi->fh, r.y+gfi->as,
		    lk->all[i].lookup->lookup_name,-1,NULL,0x000000);
	}
	++lcnt;
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( lcnt>=lk->off_top ) {
		    if ( lk->all[i].subtables[j].selected ) {
			r.x = LK_MARGIN; r.width = gfi->lkwidth-2*LK_MARGIN;
			r.y = LK_MARGIN+(lcnt-lk->off_top)*gfi->fh; r.height = gfi->fh;
			GDrawFillRect(pixmap,&r,0xffff00);
		    }
		    r.x = LK_MARGIN+2*gfi->fh-lk->off_left;
		    r.y = LK_MARGIN+(lcnt-lk->off_top)*gfi->fh;
		    GDrawDrawText8(pixmap,r.x, r.y+gfi->as,
			    lk->all[i].subtables[j].subtable->subtable_name,-1,NULL,0x000000);
		}
		++lcnt;
	    }
	}
    }
    GDrawPopClip(pixmap,&old);
}

static void LookupDeselect(struct lkdata *lk) {
    int i,j;

    for ( i=0; i<lk->cnt; ++i ) {
	lk->all[i].selected = false;
	for ( j=0; j<lk->all[i].subtable_cnt; ++j )
	    lk->all[i].subtables[j].selected = false;
    }
}

static void LookupPopup(GWindow gw,OTLookup *otl,struct lookup_subtable *sub) {
    extern char *lookup_type_names[2][10];
    static char popup_msg[300];
    int pos;
    char *lookuptype;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int l;

    if ( (otl->lookup_type&0xff)>= 0xf0 ) {
	if ( otl->lookup_type==kern_statemachine )
	    lookuptype = _("Kerning State Machine");
	else if ( otl->lookup_type==morx_indic )
	    lookuptype = _("Indic State Machine");
	else if ( otl->lookup_type==morx_context )
	    lookuptype = _("Contextual State Machine");
	else
	    lookuptype = _("Contextual State Machine");
    } else if ( (otl->lookup_type>>8)<2 && (otl->lookup_type&0xff)<10 )
	lookuptype = _(lookup_type_names[otl->lookup_type>>8][otl->lookup_type&0xff]);
    else
	lookuptype = S_("LookupType|Unknown");
    snprintf(popup_msg,sizeof(popup_msg), "%s\n", lookuptype);
    pos = strlen(popup_msg);

    if ( sub!=NULL && otl->lookup_type==gpos_pair && sub->kc!=NULL ) {
	snprintf(popup_msg+pos,sizeof(popup_msg)-pos,_("(kerning class)\n") );
	pos += strlen( popup_msg+pos );
    }

    if ( otl->features==NULL )
	snprintf(popup_msg+pos,sizeof(popup_msg)-pos,_("Not attached to a feature"));
    else {
	for ( fl=otl->features; fl!=NULL && pos<sizeof(popup_msg)-2; fl=fl->next ) {
	    snprintf(popup_msg+pos,sizeof(popup_msg)-pos,"%c%c%c%c: ",
		    fl->featuretag>>24, fl->featuretag>>16,
		    fl->featuretag>>8, fl->featuretag&0xff );
	    pos += strlen( popup_msg+pos );
	    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
		snprintf(popup_msg+pos,sizeof(popup_msg)-pos,"%c%c%c%c{",
			sl->script>>24, sl->script>>16,
			sl->script>>8, sl->script&0xff );
		pos += strlen( popup_msg+pos );
		for ( l=0; l<sl->lang_cnt; ++l ) {
		    uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		    snprintf(popup_msg+pos,sizeof(popup_msg)-pos,"%c%c%c%c,",
			    lang>>24, lang>>16,
			    lang>>8, lang&0xff );
		    pos += strlen( popup_msg+pos );
		}
		if ( popup_msg[pos-1]==',' )
		    popup_msg[pos-1] = '}';
		else if ( pos<sizeof(popup_msg)-2 )
		    popup_msg[pos++] = '}';
		if ( pos<sizeof(popup_msg)-2 )
		    popup_msg[pos++] = ' ';
	    }
	    if ( pos<sizeof(popup_msg)-2 )
		popup_msg[pos++] = '\n';
	}
    }
    if ( pos>=sizeof(popup_msg) )
	pos = sizeof(popup_msg)-1;
    popup_msg[pos]='\0';
    GGadgetPreparePopup8(gw,popup_msg);
}

static void AddDFLT(OTLookup *otl) {
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int l;

    for ( fl = otl->features; fl!=NULL; fl=fl->next ) {
	int hasDFLT = false, hasdflt=false;
	for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
	    if ( sl->script == DEFAULT_SCRIPT )
		hasDFLT = true;
	    for ( l=0; l<sl->lang_cnt; ++l ) {
		uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		if ( lang==DEFAULT_LANG ) {
		    hasdflt = true;
	    break;
		}
	    }
	    if ( hasdflt && hasDFLT )
	break;
	}
	if ( hasDFLT	/* Already there */ ||
		!hasdflt /* Shouldn't add it */ )
    continue;
	sl = chunkalloc(sizeof(struct scriptlanglist));
	sl->script = DEFAULT_SCRIPT;
	sl->lang_cnt = 1;
	sl->langs[0] = DEFAULT_LANG;
	sl->next = fl->scripts;
	fl->scripts = sl;
    }
}

static void AALTRemoveOld(SplineFont *sf,struct lkdata *lk) {
    int i;
    FeatureScriptLangList *fl, *prev;

    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	prev = NULL;
	for ( fl = lk->all[i].lookup->features; fl!=NULL; prev=fl, fl=fl->next ) {
	    if ( fl->featuretag==CHR('a','a','l','t') ) {
		if ( fl==lk->all[i].lookup->features && fl->next==NULL )
		    lk->all[i].deleted = true;
		else {
		    if ( prev==NULL )
			lk->all[i].lookup->features = fl->next;
		    else
			prev->next = fl->next;
		    fl->next = NULL;
		    FeatureScriptLangListFree(fl);
		}
	break;
	    }
	}
    }
}

		
struct sllk { uint32 script; int cnt, max; OTLookup **lookups; int lcnt, lmax; uint32 *langs; };

static void AddOTLToSllk(struct sllk *sllk, OTLookup *otl, struct scriptlanglist *sl) {
    int i,j,k,l;

    if ( otl->lookup_type==gsub_single || otl->lookup_type==gsub_alternate ) {
	for ( i=0; i<sllk->cnt; ++i )
	    if ( sllk->lookups[i]==otl )
	break;
	if ( i==sllk->cnt ) {
	    if ( sllk->cnt>=sllk->max )
		sllk->lookups = grealloc(sllk->lookups,(sllk->max+=5)*sizeof(OTLookup *));
	    sllk->lookups[sllk->cnt++] = otl;
	    for ( l=0; l<sl->lang_cnt; ++l ) {
		uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		for ( j=0; j<sllk->lcnt; ++j )
		    if ( sllk->langs[j]==lang )
		break;
		if ( j==sllk->lcnt ) {
		    if ( sllk->lcnt>=sllk->lmax )
			sllk->langs = grealloc(sllk->langs,(sllk->lmax+=sl->lang_cnt+MAX_LANG)*sizeof(uint32));
		    sllk->langs[sllk->lcnt++] = lang;
		}
	    }
	}
    } else if ( otl->lookup_type==gsub_context || otl->lookup_type==gsub_contextchain ) {
	struct lookup_subtable *sub;
	for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	    FPST *fpst = sub->fpst;
	    for ( j=0; j<fpst->rule_cnt; ++j ) {
		struct fpst_rule *r = &fpst->rules[j];
		for ( k=0; k<r->lookup_cnt; ++k )
		    AddOTLToSllk(sllk,r->lookups[k].lookup,sl);
	    }
	}
    }
    /* reverse contextual chaining is weird and I shall ignore it. Adobe does too*/
}

static char *ComponentsFromPSTs(PST **psts,int pcnt) {
    char **names=NULL;
    int ncnt=0, nmax=0;
    int i,j,len;
    char *ret;

    /* First find all the names */
    for ( i=0; i<pcnt; ++i ) {
	char *nlist = psts[i]->u.alt.components;
	char *start, *pt, ch;

	for ( start = nlist; ; ) {
	    while ( *start==' ' )
		++start;
	    if ( *start=='\0' )
	break;
	    for ( pt=start; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    for ( j=0; j<ncnt; ++j )
		if ( strcmp( start,names[j])==0 )
	    break;
	    if ( j==ncnt ) {
		if ( ncnt>=nmax )
		    names = grealloc(names,(nmax+=10)*sizeof(char *));
		names[ncnt++] = copy(start);
	    }
	    *pt = ch;
	    start = pt;
	}
    }

    len = 0;
    for ( i=0; i<ncnt; ++i )
	len += strlen(names[i])+1;
    if ( len==0 ) len=1;
    ret = galloc(len);
    len = 0;
    for ( i=0; i<ncnt; ++i ) {
	strcpy(ret+len,names[i]);
	len += strlen(names[i]);
	ret[len++] = ' ';
    }
    if ( len==0 )
	*ret = '\0';
    else
	ret[len-1] = '\0';

    for ( i=0; i<ncnt; ++i )
	free(names[i]);
    free(names);
return( ret );
}

static int SllkMatch(struct sllk *sllk,int s1,int s2) {
    int i;

    if ( sllk[s1].cnt != sllk[s2].cnt )
return( false );

    for ( i=0; i<sllk[s1].cnt; ++i ) {
	if ( sllk[s1].lookups[i] != sllk[s2].lookups[i] )
return( false );
    }

return( true );
}

static void AALTCreateNew(SplineFont *sf, struct lkdata *lk) {
    /* different script/lang combinations may need different 'aalt' lookups */
    /*  well, let's just say different script combinations */
    /* for each script/lang combo find all single/alternate subs for each */
    /*  glyph. Merge those choices and create new lookup with that info */
    struct sllk *sllk = NULL;
    int sllk_cnt=0, sllk_max = 0;
    int i,s;
    OTLookup *otl;
    struct lookup_subtable *sub;
    PST **psts, *pst;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int l,k,j,gid,pcnt;
    SplineFont *_sf;
    SplineChar *sc;

    /* Find all scripts, and all the single/alternate lookups for each */
    /*  and all the languages used for these in each script */
    for ( i=0; i<lk->cnt; ++i ) {
	otl = lk->all[i].lookup;
	for ( fl=otl->features; fl!=NULL; fl=fl->next )
	    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
		for ( s=0; s<sllk_cnt; ++s )
		    if ( sl->script == sllk[s].script )
		break;
		if ( s==sllk_cnt ) {
		    if ( sllk_cnt>=sllk_max )
			sllk = grealloc(sllk,(sllk_max+=10)*sizeof(struct sllk));
		    memset(&sllk[sllk_cnt],0,sizeof(struct sllk));
		    sllk[sllk_cnt++].script = sl->script;
		}
		AddOTLToSllk(&sllk[s], otl,sl);
	    }
    }
    /* Each of these gets its own gsub_alternate lookup which gets inserted */
    /*  at the head of the lookup list. Each lookup has one subtable */
    for ( i=0; i<sllk_cnt; ++i ) {
	if ( sllk[i].cnt==0 )		/* Script used, but provides no alternates */
    continue;
	/* Make the new lookup (and all its supporting data structures) */
	otl = chunkalloc(sizeof(OTLookup));
	otl->lookup_type = gsub_alternate;
	otl->lookup_flags = sllk[i].lookups[0]->lookup_flags & pst_r2l;
	otl->features = fl = chunkalloc(sizeof(FeatureScriptLangList));
	fl->featuretag = CHR('a','a','l','t');
	/* Any other scripts with the same lookup set? */
	for ( j=i; j<sllk_cnt; ++j ) {
	    if ( i==j || SllkMatch(sllk,i,j)) {
		sl = chunkalloc(sizeof(struct scriptlanglist));
		sl->next = fl->scripts;
		fl->scripts = sl;
		sl->script = sllk[j].script;
		sl->lang_cnt = sllk[j].lcnt;
		if ( sl->lang_cnt>MAX_LANG )
		    sl->morelangs = galloc((sl->lang_cnt-MAX_LANG)*sizeof(uint32));
		for ( l=0; l<sl->lang_cnt; ++l )
		    if ( l<MAX_LANG )
			sl->langs[l] = sllk[j].langs[l];
		    else
			sl->morelangs[l-MAX_LANG] = sllk[j].langs[l];
		if ( i!=j ) sllk[j].cnt = 0;	/* Mark as processed */
	    }
	}
	otl->subtables = sub = chunkalloc(sizeof(struct lookup_subtable));
	sub->lookup = otl;
	sub->per_glyph_pst_or_kern = true;

	/* Add it to the various lists it needs to be in */
	if ( lk->cnt>=lk->max )
	    lk->all = grealloc(lk->all,(lk->max+=10)*sizeof(struct lkinfo));
	for ( k=lk->cnt; k>0; --k )
	    lk->all[k] = lk->all[k-1];
	memset(&lk->all[0],0,sizeof(struct lkinfo));
	lk->all[0].lookup = otl;
	lk->all[0].new = true;
	++lk->cnt;
	otl->next = sf->gsub_lookups;
	sf->gsub_lookups = otl;

	/* Now add the new subtable */
	lk->all[0].subtables = gcalloc(1,sizeof(struct lksubinfo));
	lk->all[0].subtable_cnt = lk->all[0].subtable_max = 1;
	lk->all[0].subtables[0].subtable = sub;
	lk->all[0].subtables[0].new = true;

	/* Now look at every glyph in the font, and see if it has any of the */
	/*  lookups we are interested in, and if it does, build a new pst */
	/*  containing all posibilities listed on any of them */
	if ( sf->cidmaster ) sf = sf->cidmaster;
	psts = galloc(sllk[i].cnt*sizeof(PST *));
	k=0;
	do {
	    _sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
	    for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc = _sf->glyphs[gid])!=NULL ) {
		pcnt = 0;
		for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->subtable==NULL )
		continue;
		    for ( j=0; j<sllk[i].cnt; ++j )
			if ( pst->subtable->lookup == sllk[i].lookups[j] )
		    break;
		    if ( j<sllk[i].cnt )
			psts[pcnt++] = pst;
		}
		if ( pcnt==0 )
	    continue;
		pst = chunkalloc(sizeof(PST));
		pst->subtable = sub;
		pst->type = pst_alternate;
		pst->next = sc->possub;
		sc->possub = pst;
		pst->u.alt.components = ComponentsFromPSTs(psts,pcnt);
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
	free(psts);
	NameOTLookup(otl,sf);
    }

    for ( i=0; i<sllk_cnt; ++i ) {
	free( sllk[i].langs );
	free( sllk[i].lookups );
    }
    free(sllk);
}

static void lookupmenu_dispatch(GWindow v, GMenuItem *mi, GEvent *e) {
    GEvent dummy;
    struct gfi_data *gfi = GDrawGetUserData(v);
    int i;
    char *buts[4];

    if ( mi->mid==CID_SaveFeat || mi->mid==CID_SaveLookup ) {
	char *filename, *defname;
	FILE *out;
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	OTLookup *otl = NULL;

	if ( mi->mid==CID_SaveLookup ) {
	    for ( i=0; i<lk->cnt && (!lk->all[i].selected || lk->all[i].deleted); ++i );
	    if ( i==lk->cnt )
return;
	    otl = lk->all[i].lookup;
	}
	defname = strconcat(gfi->sf->fontname,".fea");
	filename = gwwv_save_filename(_("Feature file?"),defname,"*.fea");
	free(defname);
	if ( filename==NULL )
return;
	/* Convert to def encoding !!! */
	out = fopen(filename,"w");
	if ( out==NULL ) {
	    gwwv_post_error(_("Cannot open file"),_("Cannot open %s"), filename );
	    free(filename);
return;
	}
	if ( otl!=NULL )
	    FeatDumpOneLookup( out,gfi->sf,otl );
	else
	    FeatDumpFontLookups( out,gfi->sf );
	if ( ferror(out)) {
	    gwwv_post_error(_("Output error"),_("An error occurred writing %s"), filename );
	    free(filename);
	    fclose(out);
return;
	}
	free(filename);
	fclose(out);
    } else if ( mi->mid==CID_AddAllAlternates ) {
	/* First we want to remove any old aalt-only lookups */
	/*  (and remove the 'aalt' tag from any lookups with multiple features) */
	/* Then add the new ones */
	struct lkdata *lk = &gfi->tables[0];		/* GSUB */
	int has_aalt_only=false, has_aalt_mixed = false;
	int i, ret;
	FeatureScriptLangList *fl;

	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    for ( fl = lk->all[i].lookup->features; fl!=NULL; fl=fl->next ) {
		if ( fl->featuretag==CHR('a','a','l','t') ) {
		    if ( fl==lk->all[i].lookup->features && fl->next==NULL )
			has_aalt_only = true;
		    else
			has_aalt_mixed = true;
	    break;
		}
	    }
	}
	if ( has_aalt_only || has_aalt_mixed ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
	    buts[0] = GTK_STOCK_OK; buts[1] = GTK_STOCK_CANCEL; buts[2] = NULL;
#endif
	    ret = gwwv_ask(has_aalt_only?_("Lookups will be removed"):_("Feature tags will be removed"),
		    (const char **) buts,0,1,
		    !has_aalt_mixed ?
		    _("Warning: There are already some 'aalt' lookups in\n"
		      "the font. If you proceed with this command those\n"
		      "lookups will be removed and new lookups will be\n"
		      "generated. The old information will be LOST.\n"
		      " Is that what you want?") :
		    !has_aalt_only ?
		    _("Warning: There are already some 'aalt' lookups in\n"
		      "the font but there are other feature tags associated\n"
		      "with these lookups. If you proceed with this command\n"
		      "the 'aalt' tag will be removed from those lookups,\n"
		      "and new lookups will be generate which will NOT be\n"
		      "associated with the other feature tag(s).\n"
		      " Is that what you want?") :
		    _("Warning: There are already some 'aalt' lookups in\n"
		      "the font, some have no other feature tags associated\n"
		      "with them and these will be removed, others have other\n"
		      "tags associated and these will remain while the 'aalt'\n"
		      "tag will be removed from the lookup -- a new lookup\n"
		      "will be generated which is not associated with any\n"
		      "other feature tags.\n"
		      " Is that what you want?") );
	    if ( ret==1 )
return;
	    AALTRemoveOld(gfi->sf,lk);
	}
	AALTCreateNew(gfi->sf,lk);	      
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+0)),NULL,true);
    } else if ( mi->mid==CID_AddDFLT ) {
	struct selection_bits sel;
	int toall, ret, i;
	char *buts[4];
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];

	LookupParseSelection(lk,&sel);
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( sel.lookup_cnt==0 ) {
	    buts[0] = _("_Apply to All"); buts[1] = _("_Cancel"); buts[2]=NULL;
	} else {
	    buts[0] = _("_Apply to All"); buts[1] = _("_Apply to Selection"); buts[2] = _("_Cancel"); buts[3]=NULL;
	}
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( sel.lookup_cnt==0 ) {
	    buts[0] = _("_Apply to All"); buts[1] = GTK_STOCK_CANCEL; buts[2]=NULL;
	} else {
	    buts[0] = _("_Apply to All"); buts[1] = _("_Apply to Selection"); buts[2] = GTK_STOCK_CANCEL; buts[3]=NULL;
	}
#endif
	ret = gwwv_ask(_("Apply to:"),(const char **) buts,0,sel.lookup_cnt==0?1:2,_("Apply change to which lookups?"));
	toall = false;
	if ( ret==0 )
	    toall = true;
	else if ( (ret==1 && sel.lookup_cnt==0) || (ret==2 && sel.lookup_cnt!=0))
return;
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    if ( lk->all[i].selected || toall ) {
		AddDFLT(lk->all[i].lookup);
	    }
	}
    } else {
	memset(&dummy,0,sizeof(dummy));
	dummy.type = et_controlevent;
	dummy.u.control.subtype = et_buttonpress;
	dummy.u.control.g = GWidgetGetControl(gfi->gw,mi->mid);
	dummy.w = GGadgetGetWindow(dummy.u.control.g);
	dummy.u.control.u.button.button = e->u.mouse.button;
	GGadgetDispatchEvent(dummy.u.control.g,&dummy);
    }
}

static GMenuItem lookuppopupmenu[] = {
    { { (unichar_t *) N_("_Top"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupTop },
    { { (unichar_t *) N_("_Up"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupUp },
    { { (unichar_t *) N_("_Down"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupDown },
    { { (unichar_t *) N_("_Bottom"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupBottom },
    { { (unichar_t *) N_("_Sort"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupSort },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Add _Lookup"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddLookup },
    { { (unichar_t *) N_("Add Sub_table"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddSubtable },
    { { (unichar_t *) N_("Edit _Metadata"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_EditMetadata },
    { { (unichar_t *) N_("_Edit Data"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_EditSubtable },
    { { (unichar_t *) N_("De_lete"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_DeleteLookup },
    { { (unichar_t *) N_("_Merge"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_MergeLookup },
    { { (unichar_t *) N_("Sa_ve Lookup"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_SaveLookup },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Add 'aalt' features"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddAllAlternates },
    { { (unichar_t *) N_("Add 'D_FLT' script"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddDFLT },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Revert All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_RevertLookups },
    { { (unichar_t *) N_("_Import"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_RevertLookups },
    { { (unichar_t *) N_("S_ave Feature File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_SaveFeat },
    { NULL }};

static void LookupMenu(struct gfi_data *gfi,struct lkdata *lk,GEvent *event) {
    struct selection_bits sel;
    int i,j;

    LookupParseSelection(lk,&sel);
    for ( i=0; lookuppopupmenu[i].ti.text!=NULL || lookuppopupmenu[i].ti.line; ++i ) {
	switch ( lookuppopupmenu[i].mid ) {
	  case 0:
	    /* Lines */;
	  break;
	  case CID_LookupTop:
	    lookuppopupmenu[i].ti.disabled = sel.any_first || sel.lookup_cnt+sel.sub_cnt!=1;
	  break;
	  case CID_LookupUp:
	    lookuppopupmenu[i].ti.disabled = sel.any_first || sel.lookup_cnt+sel.sub_cnt==0;
	  break;
	  case CID_LookupDown:
	    lookuppopupmenu[i].ti.disabled = sel.any_last || sel.lookup_cnt+sel.sub_cnt==0;
	  break;
	  case CID_LookupBottom:
	    lookuppopupmenu[i].ti.disabled = sel.any_last || sel.lookup_cnt+sel.sub_cnt!=1;
	  break;
	  case CID_LookupSort:
	    lookuppopupmenu[i].ti.disabled = lk->cnt<=1;
	  break;
	  case CID_AddLookup:
	    lookuppopupmenu[i].ti.disabled = sel.lookup_cnt+sel.sub_cnt>1;
	  break;
	  case CID_AddSubtable:
	    lookuppopupmenu[i].ti.disabled = (sel.lookup_cnt!=1 || sel.sub_cnt>1) && (sel.lookup_cnt!=0 || sel.sub_cnt!=1);
	  break;
	  case CID_EditMetadata:
	    lookuppopupmenu[i].ti.disabled = (sel.lookup_cnt!=1 || sel.sub_cnt!=0) &&
			(sel.lookup_cnt!=0 || sel.sub_cnt!=1);
	  break;
	  case CID_EditSubtable:
	    lookuppopupmenu[i].ti.disabled = sel.lookup_cnt!=0 || sel.sub_cnt!=1;
	  break;
	  case CID_DeleteLookup:
	    lookuppopupmenu[i].ti.disabled = sel.lookup_cnt==0 && sel.sub_cnt==0;
	  break;
	  case CID_MergeLookup:
	    lookuppopupmenu[i].ti.disabled = !(
		    (sel.lookup_cnt>=2 && sel.sub_cnt==0 && sel.lookup_mergeable) ||
		    (sel.lookup_cnt==0 && sel.sub_cnt>=2 && sel.sub_table_mergeable)  );
	  break;
	  case CID_RevertLookups:
	    lookuppopupmenu[i].ti.disabled = false;
	  break;
	  case CID_SaveLookup:
	    lookuppopupmenu[i].ti.disabled = sel.lookup_cnt!=1 || sel.sub_cnt!=0;
	    for ( j=0; j<lk->cnt; ++j ) if ( lk->all[j].selected ) {
		int type = lk->all[j].lookup->lookup_type;
		if ( type==kern_statemachine || type==morx_indic ||
			type==morx_context || type==morx_insert )
		    lookuppopupmenu[i].ti.disabled = true;
	    break;
	    }
	  break;
	  case CID_AddDFLT:
	    lookuppopupmenu[i].ti.disabled = lk->cnt==0;
	  break;
	  case CID_AddAllAlternates:
	    lookuppopupmenu[i].ti.disabled = lk->cnt==0 || lk==&gfi->tables[1]/*Only applies to GSUB*/;
	  break;
	  case CID_SaveFeat:
	    lookuppopupmenu[i].ti.disabled = lk->cnt<=1;
	  break;
	}
    }
    GMenuCreatePopupMenu(event->w,event, lookuppopupmenu);
}
    

static void LookupMouse(struct gfi_data *gfi, int isgpos, GEvent *event) {
    struct lkdata *lk = &gfi->tables[isgpos];
    int l = (event->u.mouse.y-LK_MARGIN)/gfi->fh + lk->off_top;
    int inbox = event->u.mouse.x>=LK_MARGIN &&
	    event->u.mouse.x>=LK_MARGIN-lk->off_left &&
	    event->u.mouse.x<=LK_MARGIN-lk->off_left+gfi->as+1;
    GWindow gw = GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos));
    int i,j,lcnt;

    if ( l<0 || event->u.mouse.y>=(gfi->lkheight-2*LK_MARGIN) )
return;

    lcnt = 0;
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( l==lcnt ) {
	    if ( event->type==et_mouseup )
return;
	    else if ( event->type==et_mousemove ) {
		LookupPopup(gw,lk->all[i].lookup,NULL);
return;
	    } else {
		if ( inbox || event->u.mouse.clicks>1 ) {
		    lk->all[i].open = !lk->all[i].open;
		    GFI_LookupScrollbars(gfi, isgpos, true);
return;
		}
		if ( !(event->u.mouse.state&(ksm_shift|ksm_control)) ) {
		    LookupDeselect(lk);
		    lk->all[i].selected = true;
		} else
		    lk->all[i].selected = !lk->all[i].selected;
		GFI_LookupEnableButtons(gfi,isgpos);
		GDrawRequestExpose(gw,NULL,true);
		if ( event->u.mouse.button==3 )
		    LookupMenu(gfi,lk,event);
return;
	    }
	}
	++lcnt;
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( l==lcnt ) {
		    if ( event->type==et_mouseup )
return;
		    else if ( event->type==et_mousemove ) {
			LookupPopup(gw,lk->all[i].lookup,lk->all[i].subtables[j].subtable);
return;
		    } else {
			if ( inbox )
return;		/* Can't open this guy */
			if ( event->u.mouse.clicks>1 )
			    LookupSubtableContents(gfi,isgpos);
			else {
			    if ( !(event->u.mouse.state&(ksm_shift|ksm_control)) ) {
				LookupDeselect(lk);
				lk->all[i].subtables[j].selected = true;
			    } else
				lk->all[i].subtables[j].selected = !lk->all[i].subtables[j].selected;
			    GFI_LookupEnableButtons(gfi,isgpos);
			    GDrawRequestExpose(gw,NULL,true);
			    if ( event->u.mouse.button==3 )
				LookupMenu(gfi,lk,event);
			}
return;
		    }
		}
		++lcnt;
	    }
	}
    }
}

static int lookups_e_h(GWindow gw, GEvent *event, int isgpos) {
    struct gfi_data *gfi = GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(GWidgetGetControl(gw,CID_LookupVSB+isgpos),event));
    }

    switch ( event->type ) {
      case et_char:
return( GFI_Char(gfi,event) );
      case et_expose:
	LookupExpose(gw,gfi,isgpos);
      break;
      case et_mousedown: case et_mousemove: case et_mouseup:
	LookupMouse(gfi,isgpos,event);
      break;
      case et_resize: {
	GRect r;
	GDrawGetSize(gw,&r);
	gfi->lkheight = r.height; gfi->lkwidth = r.width;
	GFI_LookupScrollbars(gfi,false,false);
	GFI_LookupScrollbars(gfi,true,false);
      }
      break;
    }
return( true );
}

static int gposlookups_e_h(GWindow gw, GEvent *event) {
return( lookups_e_h(gw,event,true));
}

static int gsublookups_e_h(GWindow gw, GEvent *event) {
return( lookups_e_h(gw,event,false));
}

void FontInfo(SplineFont *sf,int defaspect,int sync) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo aspects[22], vaspects[5], lkaspects[3];
    GGadgetCreateData mgcd[10], ngcd[17], psgcd[30], tngcd[8],
	pgcd[8], vgcd[19], pangcd[22], comgcd[3], txgcd[23],
	mfgcd[8], mcgcd[8], szgcd[19], mkgcd[5], metgcd[29], vagcd[3], ssgcd[23],
	xugcd[7], dgcd[6], ugcd[4], gaspgcd[5], gaspgcd_def[2], lksubgcd[2][4],
	lkgcd[2], lkbuttonsgcd[15];
    GGadgetCreateData mb[2], mb2, nb[2], nb2, nb3, xub[2], psb[2], psb2[3], ppbox[3],
	    vbox[4], metbox[2], ssbox[2], panbox[2], combox[2], mkbox[3],
	    txbox[5], ubox[2], dbox[2], 
	    mcbox[3], mfbox[3], szbox[6], tnboxes[4], gaspboxes[3],
	    lkbox[7];
    GGadgetCreateData *marray[7], *marray2[9], *narray[26], *narray2[7], *narray3[3],
	*xuarray[13], *psarray[10], *psarray2[21], *psarray3[3], *psarray4[10],
	*ppbuttons[5], *pparray[4], *vradio[5], *varray[38], *metarray[46],
	*ssarray[58], *panarray[38], *comarray[2],
	*mkarray[3], *mkarray2[4], *txarray[5], *txarray2[30],
	*txarray3[6], *txarray4[6], *uarray[3], *darray[10],
	*mcarray[13], *mcarray2[7],
	*mfarray[14], *szarray[7], *szarray2[5], *szarray3[7],
	*szarray4[4], *szarray5[6], *tnvarray[4], *tnharray[6], *tnharray2[5], *gaspharray[6],
	*gaspvarray[3], *lkarray[2][7], *lkbuttonsarray[17], *lkharray[3];
    GTextInfo mlabel[10], nlabel[16], pslabel[30], tnlabel[7],
	plabel[8], vlabel[19], panlabel[22], comlabel[3], txlabel[23],
	mflabel[8], mclabel[8], szlabel[17], mklabel[5], metlabel[28],
	sslabel[23], xulabel[6], dlabel[5], ulabel[1], gasplabel[5],
	lkbuttonslabel[14];
    GTextInfo *namelistnames;
    struct gfi_data *d;
    char iabuf[20], upbuf[20], uwbuf[20], asbuf[20], dsbuf[20],
	    vbuf[20], uibuf[12], vorig[20], embuf[20];
    char dszbuf[20], dsbbuf[20], dstbuf[21], sibuf[20], swbuf[20];
    int i,j,k, psrow;
    int mcs;
    char title[130];
    /* static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };*/
    static unichar_t sans[] = { 'h','e','l','v','e','t','i','c','a',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    FontRequest rq;
    int as, ds, ld;
    char **nlnames;
    char createtime[200], modtime[200];
    unichar_t *tmpcreatetime, *tmpmodtime;
    time_t t;
    const struct tm *tm;
    struct matrixinit mi, gaspmi;

    FontInfoInit();

    if ( sf->fontinfo!=NULL ) {
	GDrawSetVisible(((struct gfi_data *) (sf->fontinfo))->gw,true);
	GDrawRaise( ((struct gfi_data *) (sf->fontinfo))->gw );
return;
    }
    if ( defaspect==-1 )
	defaspect = last_aspect;

    d = gcalloc(1,sizeof(struct gfi_data));
    sf->fontinfo = d;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    if ( sync ) {
	wattrs.mask |= wam_restrict;
	wattrs.restrict_input_to_me = 1;
    }
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    snprintf(title,sizeof(title),_("Font Information for %.90s"),
	    sf->fontname);
    wattrs.utf8_window_title = title;
    pos.x = pos.y = 0;
#ifndef FONTFORGE_CONFIG_INFO_HORIZONTAL
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268+85));
#else
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
#endif
    pos.height = GDrawPointsToPixels(NULL,375);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,d,&wattrs);

    d->sf = sf;
    d->gw = gw;
    d->old_sel = -2;
    d->texdata = sf->texdata;

    memset(&nlabel,0,sizeof(nlabel));
    memset(&ngcd,0,sizeof(ngcd));

    nlabel[0].text = (unichar_t *) _("Fo_ntname:");
    nlabel[0].text_is_1byte = true;
    nlabel[0].text_in_resource = true;
    ngcd[0].gd.label = &nlabel[0];
    ngcd[0].gd.pos.x = 12; ngcd[0].gd.pos.y = 6+6; 
    ngcd[0].gd.flags = gg_visible | gg_enabled;
    ngcd[0].creator = GLabelCreate;

    ngcd[1].gd.pos.x = 115; ngcd[1].gd.pos.y = ngcd[0].gd.pos.y-6; ngcd[1].gd.pos.width = 137;
    ngcd[1].gd.flags = gg_visible | gg_enabled;
    nlabel[1].text = (unichar_t *) sf->fontname;
    nlabel[1].text_is_1byte = true;
    ngcd[1].gd.label = &nlabel[1];
    ngcd[1].gd.cid = CID_Fontname;
    ngcd[1].gd.handle_controlevent = GFI_NameChange;
    ngcd[1].creator = GTextFieldCreate;

    nlabel[2].text = (unichar_t *) _("_Family Name:");
    nlabel[2].text_is_1byte = true;
    nlabel[2].text_in_resource = true;
    ngcd[2].gd.label = &nlabel[2];
    ngcd[2].gd.pos.x = 12; ngcd[2].gd.pos.y = ngcd[0].gd.pos.y+26; 
    ngcd[2].gd.flags = gg_visible | gg_enabled;
    ngcd[2].creator = GLabelCreate;

    ngcd[3].gd.pos.x = ngcd[1].gd.pos.x; ngcd[3].gd.pos.y = ngcd[2].gd.pos.y-6; ngcd[3].gd.pos.width = 137;
    ngcd[3].gd.flags = gg_visible | gg_enabled;
    nlabel[3].text = (unichar_t *) (sf->familyname?sf->familyname:sf->fontname);
    nlabel[3].text_is_1byte = true;
    ngcd[3].gd.label = &nlabel[3];
    ngcd[3].gd.cid = CID_Family;
    ngcd[3].gd.handle_controlevent = GFI_FamilyChange;
    ngcd[3].creator = GTextFieldCreate;
    if ( sf->familyname==NULL || strstr(sf->familyname,"Untitled")==sf->familyname )
	d->family_untitled = true;

    ngcd[4].gd.pos.x = 12; ngcd[4].gd.pos.y = ngcd[3].gd.pos.y+26+6;
    nlabel[4].text = (unichar_t *) _("Name For Human_s:");
    nlabel[4].text_is_1byte = true;
    nlabel[4].text_in_resource = true;
    ngcd[4].gd.label = &nlabel[4];
    ngcd[4].gd.flags = gg_visible | gg_enabled;
    ngcd[4].creator = GLabelCreate;

    ngcd[5].gd.pos.x = 115; ngcd[5].gd.pos.y = ngcd[4].gd.pos.y-6; ngcd[5].gd.pos.width = 137;
    ngcd[5].gd.flags = gg_visible | gg_enabled;
    nlabel[5].text = (unichar_t *) (sf->fullname?sf->fullname:sf->fontname);
    nlabel[5].text_is_1byte = true;
    ngcd[5].gd.label = &nlabel[5];
    ngcd[5].gd.cid = CID_Human;
    ngcd[5].gd.handle_controlevent = GFI_HumanChange;
    ngcd[5].creator = GTextFieldCreate;
    if ( sf->fullname==NULL || strstr(sf->fullname,"Untitled")==sf->fullname )
	d->human_untitled = true;

    nlabel[6].text = (unichar_t *) _("_Weight");
    nlabel[6].text_is_1byte = true;
    nlabel[6].text_in_resource = true;
    ngcd[6].gd.label = &nlabel[6];
    ngcd[6].gd.pos.x = ngcd[4].gd.pos.x; ngcd[6].gd.pos.y = ngcd[4].gd.pos.y+26; 
    ngcd[6].gd.flags = gg_visible | gg_enabled;
    ngcd[6].creator = GLabelCreate;

    ngcd[7].gd.pos.x = ngcd[1].gd.pos.x; ngcd[7].gd.pos.y = ngcd[6].gd.pos.y-6; ngcd[7].gd.pos.width = 137;
    ngcd[7].gd.flags = gg_visible | gg_enabled;
    nlabel[7].text = (unichar_t *) (sf->weight?sf->weight:"Regular");
    nlabel[7].text_is_1byte = true;
    ngcd[7].gd.label = &nlabel[7];
    ngcd[7].gd.cid = CID_Weight;
    ngcd[7].creator = GTextFieldCreate;

    ngcd[8].gd.pos.x = 12; ngcd[8].gd.pos.y = ngcd[6].gd.pos.y+26;
    nlabel[8].text = (unichar_t *) _("_Version:");
    nlabel[8].text_is_1byte = true;
    nlabel[8].text_in_resource = true;
    ngcd[8].gd.label = &nlabel[8];
    ngcd[8].gd.flags = gg_visible | gg_enabled;
    ngcd[8].creator = GLabelCreate;

    ngcd[9].gd.pos.x = 115; ngcd[9].gd.pos.y = ngcd[8].gd.pos.y-6; ngcd[9].gd.pos.width = 137;
    ngcd[9].gd.flags = gg_visible | gg_enabled;
    nlabel[9].text = (unichar_t *) (sf->version?sf->version:"");
    nlabel[9].text_is_1byte = true;
    if ( sf->subfontcnt!=0 ) {
	sprintf( vbuf,"%g", sf->cidversion );
	nlabel[9].text = (unichar_t *) vbuf;
    }
    ngcd[9].gd.label = &nlabel[9];
    ngcd[9].gd.cid = CID_Version;
    ngcd[9].creator = GTextFieldCreate;

    ngcd[10].gd.pos.x = 12; ngcd[10].gd.pos.y = ngcd[8].gd.pos.y+28;
    nlabel[10].text = (unichar_t *) _("_Base Filename:");
    nlabel[10].text_is_1byte = true;
    nlabel[10].text_in_resource = true;
    ngcd[10].gd.label = &nlabel[10];
    ngcd[10].gd.popup_msg = (unichar_t *) _("Use this as the default base for the filename\nwhen generating a font." );
    ngcd[10].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    ngcd[10].creator = GLabelCreate;

    ngcd[11].gd.pos.x = 95; ngcd[11].gd.pos.y = ngcd[10].gd.pos.y-11;
/* GT: The space in front of "Same" makes things line up better */
    nlabel[11].text = (unichar_t *) _(" Same as Fontname");
    nlabel[11].text_is_1byte = true;
    ngcd[11].gd.label = &nlabel[11];
    ngcd[11].gd.flags = sf->defbasefilename==NULL ? (gg_visible | gg_enabled | gg_cb_on ) : (gg_visible | gg_enabled);
    ngcd[11].gd.cid = CID_SameAsFontname;
    ngcd[11].creator = GRadioCreate;

    ngcd[12].gd.pos.x = 95; ngcd[12].gd.pos.y = ngcd[10].gd.pos.y+11;
    nlabel[12].text = (unichar_t *) "";
    nlabel[12].text_is_1byte = true;
    ngcd[12].gd.label = &nlabel[12];
    ngcd[12].gd.flags = sf->defbasefilename!=NULL ?
		(gg_visible | gg_enabled | gg_cb_on | gg_rad_continueold ) :
		(gg_visible | gg_enabled | gg_rad_continueold);
    ngcd[12].gd.cid = CID_HasDefBase;
    ngcd[12].creator = GRadioCreate;

    ngcd[13].gd.pos.x = 115; ngcd[13].gd.pos.y = ngcd[12].gd.pos.y-4; ngcd[13].gd.pos.width = 137;
    ngcd[13].gd.flags = gg_visible | gg_enabled;
    nlabel[13].text = (unichar_t *) (sf->defbasefilename?sf->defbasefilename:"");
    nlabel[13].text_is_1byte = true;
    ngcd[13].gd.label = &nlabel[13];
    ngcd[13].gd.cid = CID_DefBaseName;
    ngcd[13].gd.handle_controlevent = GFI_DefBaseChange;
    ngcd[13].creator = GTextFieldCreate;

    ngcd[14].gd.pos.x = 12; ngcd[14].gd.pos.y = ngcd[10].gd.pos.y+22;
    ngcd[14].gd.flags = gg_visible | gg_enabled;
    nlabel[14].text = (unichar_t *) _("Copy_right:");
    nlabel[14].text_is_1byte = true;
    nlabel[14].text_in_resource = true;
    ngcd[14].gd.label = &nlabel[14];
    ngcd[14].creator = GLabelCreate;

    ngcd[15].gd.pos.x = 12; ngcd[15].gd.pos.y = ngcd[14].gd.pos.y+14;
    ngcd[15].gd.pos.width = ngcd[5].gd.pos.x+ngcd[5].gd.pos.width-26;
    ngcd[15].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    if ( sf->copyright!=NULL ) {
	nlabel[15].text = (unichar_t *) sf->copyright;
	nlabel[15].text_is_1byte = true;
	ngcd[15].gd.label = &nlabel[15];
    }
    ngcd[15].gd.cid = CID_Notice;
    ngcd[15].creator = GTextAreaCreate;

    memset(&nb,0,sizeof(nb)); memset(&nb2,0,sizeof(nb2)); memset(&nb3,0,sizeof(nb3));

    narray3[0] = &ngcd[12]; narray3[1] = &ngcd[13]; narray3[2] = NULL;

    narray2[0] = &ngcd[10]; narray2[1] = &ngcd[11]; narray2[2] = NULL;
    narray2[3] = GCD_RowSpan; narray2[4] = &nb3; narray2[5] = NULL;
    narray2[6] = NULL;

    narray[0] = &ngcd[0]; narray[1] = &ngcd[1]; narray[2] = NULL;
    narray[3] = &ngcd[2]; narray[4] = &ngcd[3]; narray[5] = NULL;
    narray[6] = &ngcd[4]; narray[7] = &ngcd[5]; narray[8] = NULL;
    narray[9] = &ngcd[6]; narray[10] = &ngcd[7]; narray[11] = NULL;
    narray[12] = &ngcd[8]; narray[13] = &ngcd[9]; narray[14] = NULL;
    narray[15] = &nb2; narray[16] = GCD_ColSpan; narray[17] = NULL;
    narray[18] = &ngcd[14]; narray[19] = GCD_ColSpan; narray[20] = NULL;
    narray[21] = &ngcd[15]; narray[22] = GCD_ColSpan; narray[23] = NULL;
    narray[24] = NULL;

    nb3.gd.flags = gg_enabled|gg_visible;
    nb3.gd.u.boxelements = narray3;
    nb3.creator = GHBoxCreate;

    nb2.gd.flags = gg_enabled|gg_visible;
    nb2.gd.u.boxelements = narray2;
    nb2.creator = GHVBoxCreate;

    nb[0].gd.flags = gg_enabled|gg_visible;
    nb[0].gd.u.boxelements = narray;
    nb[0].creator = GHVBoxCreate;

/******************************************************************************/
    memset(&xulabel,0,sizeof(xulabel));
    memset(&xugcd,0,sizeof(xugcd));

    xugcd[0].gd.pos.x = 12; xugcd[0].gd.pos.y = 10+6;
    xugcd[0].gd.flags = gg_visible | gg_enabled;
    xulabel[0].text = (unichar_t *) _("_XUID:");
    xulabel[0].text_is_1byte = true;
    xulabel[0].text_in_resource = true;
    xugcd[0].gd.label = &xulabel[0];
    xugcd[0].creator = GLabelCreate;

    xugcd[1].gd.pos.x = 103; xugcd[1].gd.pos.y = xugcd[0].gd.pos.y-6; xugcd[1].gd.pos.width = 142;
    xugcd[1].gd.flags = gg_visible | gg_enabled;
    if ( sf->xuid!=NULL ) {
	xulabel[1].text = (unichar_t *) sf->xuid;
	xulabel[1].text_is_1byte = true;
	xugcd[1].gd.label = &xulabel[1];
    }
    xugcd[1].gd.cid = CID_XUID;
    xugcd[1].creator = GTextFieldCreate;

    xugcd[2].gd.pos.x = 12; xugcd[2].gd.pos.y = xugcd[1].gd.pos.y+26+6;
    xulabel[2].text = (unichar_t *) _("_UniqueID:");
    xulabel[2].text_is_1byte = true;
    xulabel[2].text_in_resource = true;
    xugcd[2].gd.label = &xulabel[2];
    xugcd[2].gd.flags = gg_visible | gg_enabled;
    xugcd[2].creator = GLabelCreate;

    xugcd[3].gd.pos.x = xugcd[1].gd.pos.x; xugcd[3].gd.pos.y = xugcd[2].gd.pos.y-6; xugcd[3].gd.pos.width = 80;
    xugcd[3].gd.flags = gg_visible | gg_enabled;
    xulabel[3].text = (unichar_t *) "";
    xulabel[3].text_is_1byte = true;
    if ( sf->uniqueid!=0 ) {
	sprintf( uibuf, "%d", sf->uniqueid );
	xulabel[3].text = (unichar_t *) uibuf;
    }
    xugcd[3].gd.label = &xulabel[3];
    xugcd[3].gd.cid = CID_UniqueID;
    xugcd[3].creator = GTextFieldCreate;

    xugcd[4].gd.pos.x = 8; xugcd[4].gd.pos.y = xugcd[3].gd.pos.y+26+6;
    xulabel[4].text = (unichar_t *) _("(Adobe now considers XUID/UniqueID unnecessary)");
    xulabel[4].text_is_1byte = true;
    xulabel[4].text_in_resource = true;
    xugcd[4].gd.label = &xulabel[4];
    xugcd[4].gd.flags = gg_visible | gg_enabled;
    xugcd[4].creator = GLabelCreate;

    xuarray[0] = &xugcd[0]; xuarray[1] = &xugcd[1]; xuarray[2] = NULL;
    xuarray[3] = &xugcd[2]; xuarray[4] = &xugcd[3]; xuarray[5] = NULL;
    xuarray[6] = &xugcd[4]; xuarray[7] = GCD_ColSpan; xuarray[8] = NULL;
    xuarray[9] = GCD_Glue; xuarray[10] = GCD_Glue; xuarray[11] = NULL;
    xuarray[12] = NULL;

    memset(xub,0,sizeof(xub));
    xub[0].gd.flags = gg_enabled|gg_visible;
    xub[0].gd.u.boxelements = xuarray;
    xub[0].creator = GHVBoxCreate;

/******************************************************************************/
    memset(&pslabel,0,sizeof(pslabel));
    memset(&psgcd,0,sizeof(psgcd));

    psgcd[0].gd.pos.x = 12; psgcd[0].gd.pos.y = 12;
    psgcd[0].gd.flags = gg_visible | gg_enabled;
    pslabel[0].text = (unichar_t *) _("_Ascent:");
    pslabel[0].text_is_1byte = true;
    pslabel[0].text_in_resource = true;
    psgcd[0].gd.label = &pslabel[0];
    psgcd[0].creator = GLabelCreate;

    psgcd[1].gd.pos.x = 103; psgcd[1].gd.pos.y = psgcd[0].gd.pos.y-6; psgcd[1].gd.pos.width = 47;
    psgcd[1].gd.flags = gg_visible | gg_enabled;
    sprintf( asbuf, "%d", sf->ascent );
    pslabel[1].text = (unichar_t *) asbuf;
    pslabel[1].text_is_1byte = true;
    psgcd[1].gd.label = &pslabel[1];
    psgcd[1].gd.cid = CID_Ascent;
    psgcd[1].gd.handle_controlevent = GFI_EmChanged;
    psgcd[1].creator = GTextFieldCreate;

    psgcd[2].gd.pos.x = 155; psgcd[2].gd.pos.y = psgcd[0].gd.pos.y;
    psgcd[2].gd.flags = gg_visible | gg_enabled;
    pslabel[2].text = (unichar_t *) _("_Descent:");
    pslabel[2].text_is_1byte = true;
    pslabel[2].text_in_resource = true;
    psgcd[2].gd.label = &pslabel[2];
    psgcd[2].creator = GLabelCreate;

    psgcd[3].gd.pos.x = 200; psgcd[3].gd.pos.y = psgcd[1].gd.pos.y; psgcd[3].gd.pos.width = 47;
    psgcd[3].gd.flags = gg_visible | gg_enabled;
    sprintf( dsbuf, "%d", sf->descent );
    pslabel[3].text = (unichar_t *) dsbuf;
    pslabel[3].text_is_1byte = true;
    psgcd[3].gd.label = &pslabel[3];
    psgcd[3].gd.cid = CID_Descent;
    psgcd[3].gd.handle_controlevent = GFI_EmChanged;
    psgcd[3].creator = GTextFieldCreate;

    psgcd[4].gd.pos.x = psgcd[0].gd.pos.x+5; psgcd[4].gd.pos.y = psgcd[0].gd.pos.y+24;
    psgcd[4].gd.flags = gg_visible | gg_enabled;
    pslabel[4].text = (unichar_t *) _(" _Em Size:");
    pslabel[4].text_is_1byte = true;
    pslabel[4].text_in_resource = true;
    psgcd[4].gd.label = &pslabel[4];
    psgcd[4].creator = GLabelCreate;

    psgcd[5].gd.pos.x = psgcd[1].gd.pos.x-20; psgcd[5].gd.pos.y = psgcd[1].gd.pos.y+24; psgcd[5].gd.pos.width = 67;
    psgcd[5].gd.flags = gg_visible | gg_enabled;
    sprintf( embuf, "%d", sf->descent+sf->ascent );
    pslabel[5].text = (unichar_t *) embuf;
    pslabel[5].text_is_1byte = true;
    psgcd[5].gd.label = &pslabel[5];
    psgcd[5].gd.cid = CID_Em;
    psgcd[5].gd.u.list = emsizes;
    psgcd[5].gd.handle_controlevent = GFI_EmChanged;
    psgcd[5].creator = GListFieldCreate;

    psgcd[6].gd.pos.x = psgcd[2].gd.pos.x; psgcd[6].gd.pos.y = psgcd[4].gd.pos.y-4;
    psgcd[6].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    pslabel[6].text = (unichar_t *) _("_Scale Outlines");
    pslabel[6].text_is_1byte = true;
    pslabel[6].text_in_resource = true;
    psgcd[6].gd.label = &pslabel[6];
    psgcd[6].gd.cid = CID_Scale;
    psgcd[6].creator = GCheckBoxCreate;

/* I've reversed the label, text field order in the gcd here because */
/*  that way the text field will be displayed on top of the label rather */
/*  than the reverse, and in Russian that's important translation of */
/*  "Italic Angle" is too long. Oops, no it's the next one, might as well leave in here though */
    psgcd[8].gd.pos.x = 12; psgcd[8].gd.pos.y = psgcd[5].gd.pos.y+26+6;
    psgcd[8].gd.flags = gg_visible | gg_enabled;
    pslabel[8].text = (unichar_t *) _("_Italic Angle:");
    pslabel[8].text_is_1byte = true;
    pslabel[8].text_in_resource = true;
    psgcd[8].gd.label = &pslabel[8];
    psgcd[8].creator = GLabelCreate;

    psgcd[7].gd.pos.x = 103; psgcd[7].gd.pos.y = psgcd[8].gd.pos.y-6;
    psgcd[7].gd.pos.width = 47;
    psgcd[7].gd.flags = gg_visible | gg_enabled;
    sprintf( iabuf, "%g", (double) sf->italicangle );
    pslabel[7].text = (unichar_t *) iabuf;
    pslabel[7].text_is_1byte = true;
    psgcd[7].gd.label = &pslabel[7];
    psgcd[7].gd.cid = CID_ItalicAngle;
    psgcd[7].creator = GTextFieldCreate;

    psgcd[9].gd.pos.y = psgcd[7].gd.pos.y;
    psgcd[9].gd.pos.width = -1; psgcd[9].gd.pos.height = 0;
    psgcd[9].gd.pos.x = psgcd[3].gd.pos.x+psgcd[3].gd.pos.width-
	    GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    /*if ( strstrmatch(sf->fontname,"Italic")!=NULL ||strstrmatch(sf->fontname,"Oblique")!=NULL )*/
	psgcd[9].gd.flags = gg_visible | gg_enabled;
    pslabel[9].text = (unichar_t *) _("_Guess");
    pslabel[9].text_is_1byte = true;
    pslabel[9].text_in_resource = true;
    psgcd[9].gd.label = &pslabel[9];
    psgcd[9].gd.handle_controlevent = GFI_GuessItalic;
    psgcd[9].creator = GButtonCreate;

/* I've reversed the label, text field order in the gcd here because */
/*  that way the text field will be displayed on top of the label rather */
/*  than the reverse, and in Russian that's important translation of */
/*  "Underline position" is too long. */
    psgcd[11].gd.pos.x = 12; psgcd[11].gd.pos.y = psgcd[7].gd.pos.y+26+6;
    psgcd[11].gd.flags = gg_visible | gg_enabled;
    pslabel[11].text = (unichar_t *) _("Underline _Position:");
    pslabel[11].text_is_1byte = true;
    pslabel[11].text_in_resource = true;
    psgcd[11].gd.label = &pslabel[11];
    psgcd[11].creator = GLabelCreate;

    psgcd[10].gd.pos.x = 103; psgcd[10].gd.pos.y = psgcd[11].gd.pos.y-6; psgcd[10].gd.pos.width = 47;
    psgcd[10].gd.flags = gg_visible | gg_enabled;
    sprintf( upbuf, "%g", (double) sf->upos );
    pslabel[10].text = (unichar_t *) upbuf;
    pslabel[10].text_is_1byte = true;
    psgcd[10].gd.label = &pslabel[10];
    psgcd[10].gd.cid = CID_UPos;
    psgcd[10].creator = GTextFieldCreate;

    psgcd[12].gd.pos.x = 155; psgcd[12].gd.pos.y = psgcd[11].gd.pos.y;
    psgcd[12].gd.flags = gg_visible | gg_enabled;
    pslabel[12].text = (unichar_t *) S_("Underline|_Height:");
    pslabel[12].text_is_1byte = true;
    pslabel[12].text_in_resource = true;
    psgcd[12].gd.label = &pslabel[12];
    psgcd[12].creator = GLabelCreate;

    psgcd[13].gd.pos.x = 200; psgcd[13].gd.pos.y = psgcd[10].gd.pos.y; psgcd[13].gd.pos.width = 47;
    psgcd[13].gd.flags = gg_visible | gg_enabled;
    sprintf( uwbuf, "%g", (double) sf->uwidth );
    pslabel[13].text = (unichar_t *) uwbuf;
    pslabel[13].text_is_1byte = true;
    psgcd[13].gd.label = &pslabel[13];
    psgcd[13].gd.cid = CID_UWidth;
    psgcd[13].creator = GTextFieldCreate;

    psgcd[14].gd.pos.x = 12; psgcd[14].gd.pos.y = psgcd[13].gd.pos.y+26;
    pslabel[14].text = (unichar_t *) _("Has _Vertical Metrics");
    pslabel[14].text_is_1byte = true;
    pslabel[14].text_in_resource = true;
    psgcd[14].gd.label = &pslabel[14];
    psgcd[14].gd.cid = CID_HasVerticalMetrics;
    psgcd[14].gd.flags = gg_visible | gg_enabled;
    if ( sf->hasvmetrics )
	psgcd[14].gd.flags |= gg_cb_on;
    psgcd[14].gd.handle_controlevent = GFI_VMetricsCheck;
    psgcd[14].creator = GCheckBoxCreate;

    psgcd[15].gd.pos.x = 12; psgcd[15].gd.pos.y = psgcd[14].gd.pos.y+22;
    pslabel[15].text = (unichar_t *) _("Vertical _Origin:");
    pslabel[15].text_is_1byte = true;
    pslabel[15].text_in_resource = true;
    psgcd[15].gd.label = &pslabel[15];
    psgcd[15].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled) : gg_visible;
    psgcd[15].gd.cid = CID_VOriginLab;
    psgcd[15].creator = GLabelCreate;

    psgcd[16].gd.pos.x = psgcd[7].gd.pos.x; psgcd[16].gd.pos.y = psgcd[15].gd.pos.y-4; psgcd[16].gd.pos.width = psgcd[15].gd.pos.width;
    psgcd[16].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled) : gg_visible;
    pslabel[16].text = (unichar_t *) "";
    pslabel[16].text_is_1byte = true;
    if ( sf->vertical_origin!=0 || sf->hasvmetrics ) {
	sprintf( vorig, "%d", sf->vertical_origin );
	pslabel[16].text = (unichar_t *) vorig;
    }
    psgcd[16].gd.label = &pslabel[16];
    psgcd[16].gd.cid = CID_VOrigin;
    psgcd[16].creator = GTextFieldCreate;

    psgcd[17].gd.pos.x = 12; psgcd[17].gd.pos.y = psgcd[16].gd.pos.y+22;
    pslabel[17].text = (unichar_t *) _("_Quadratic Splines");
    pslabel[17].text_is_1byte = true;
    pslabel[17].text_in_resource = true;
    psgcd[17].gd.label = &pslabel[17];
    psgcd[17].gd.flags = sf->order2 ? (gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup) : (gg_visible | gg_enabled | gg_utf8_popup);
    psgcd[17].gd.cid = CID_IsOrder2;
    psgcd[17].creator = GCheckBoxCreate;
    psgcd[17].gd.popup_msg = (unichar_t *) _("Use quadratic (that is truetype) splines to hold the outlines of this\nfont rather than cubic (postscript) splines. Set this option if you\nare editing truetype font. Unset it if you are editing an opentype\nor postscript font (FontForge will convert to the appropriate\nspline type when it generates fonts so this is not required).");

#ifdef FONTFORGE_CONFIG_TYPE3
    psgcd[18].gd.pos.x = 12; psgcd[18].gd.pos.y = psgcd[17].gd.pos.y+18;
    pslabel[18].text = (unichar_t *) _("_Outline Font");
    pslabel[18].text_is_1byte = true;
    pslabel[18].text_in_resource = true;
    psgcd[18].gd.label = &pslabel[18];
    psgcd[18].gd.flags = (!sf->strokedfont && !sf->multilayer)?
	    (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    psgcd[18].creator = GRadioCreate;

    psgcd[19].gd.pos.x = 12; psgcd[19].gd.pos.y = psgcd[18].gd.pos.y+14;
    pslabel[19].text = (unichar_t *) _("_Multi Layered Font");
    pslabel[19].text_is_1byte = true;
    pslabel[19].text_in_resource = true;
    psgcd[19].gd.label = &pslabel[19];
    psgcd[19].gd.flags = sf->multilayer ? (gg_visible | gg_enabled | gg_utf8_popup | gg_cb_on | gg_rad_continueold) : (gg_visible | gg_enabled | gg_utf8_popup | gg_rad_continueold);
    psgcd[19].gd.cid = CID_IsMultiLayer;
    psgcd[19].creator = GRadioCreate;
    psgcd[19].gd.popup_msg = (unichar_t *) _("Allow editing of multiple colors and shades, fills and strokes.\nMulti layered fonts can only be output as type3 or svg fonts.");

    psgcd[20].gd.pos.x = 12; psgcd[20].gd.pos.y = psgcd[19].gd.pos.y+14;
    pslabel[20].text = (unichar_t *) _("_Stroked Font");
    pslabel[20].text_is_1byte = true;
    pslabel[20].text_in_resource = true;
    psgcd[20].gd.label = &pslabel[20];
    psgcd[20].gd.flags = sf->strokedfont ? (gg_visible | gg_enabled | gg_utf8_popup | gg_cb_on) : (gg_visible | gg_enabled | gg_utf8_popup);
    psgcd[20].gd.cid = CID_IsStrokedFont;
    psgcd[20].creator = GRadioCreate;
    psgcd[20].gd.popup_msg = (unichar_t *) _("Glyphs will be composed of stroked lines rather than filled outlines.\nAll glyphs are stroked at the following width");

    k=21;
#else
    psgcd[18].gd.pos.x = 12; psgcd[18].gd.pos.y = psgcd[17].gd.pos.y+16;
    pslabel[18].text = (unichar_t *) _("_Stroked Font");
    pslabel[18].text_is_1byte = true;
    pslabel[18].text_in_resource = true;
    psgcd[18].gd.label = &pslabel[18];
    psgcd[18].gd.flags = sf->strokedfont ? (gg_visible | gg_enabled | gg_utf8_popup | gg_cb_on) : (gg_visible | gg_enabled | gg_utf8_popup);
    psgcd[18].gd.cid = CID_IsStrokedFont;
    psgcd[18].creator = GCheckBoxCreate;
    psgcd[18].gd.popup_msg = (unichar_t *) _("Glyphs will be composed of stroked lines rather than filled outlines.\nAll glyphs are stroked at the following width");

    k=19;
#endif

    psgcd[k].gd.pos.x = 12; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y+20;
    pslabel[k].text = (unichar_t *) _("Stroke _Width:");
    pslabel[k].text_is_1byte = true;
    pslabel[k].text_in_resource = true;
    psgcd[k].gd.label = &pslabel[k];
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    psgcd[k++].creator = GLabelCreate;

    sprintf( swbuf,"%g", (double) sf->strokewidth );
    psgcd[k].gd.pos.x = 115; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y-6; psgcd[k].gd.pos.width = 137;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    pslabel[k].text = (unichar_t *) swbuf;
    pslabel[k].text_is_1byte = true;
    psgcd[k].gd.label = &pslabel[k];
    psgcd[k].gd.cid = CID_StrokeWidth;
    psgcd[k++].creator = GTextFieldCreate;

    psgcd[k].gd.pos.x = psgcd[k-2].gd.pos.x; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y+32;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    pslabel[k].text = (unichar_t *) _("Interpretation:");
    pslabel[k].text_is_1byte = true;
    pslabel[k].text_in_resource = true;
    psgcd[k].gd.label = &pslabel[k];
    psgcd[k++].creator = GLabelCreate;

    psgcd[k].gd.pos.x = psgcd[k-2].gd.pos.x; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y-6;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    psgcd[k].gd.u.list = interpretations;
    psgcd[k].gd.cid = CID_Interpretation;
    psgcd[k].creator = GListButtonCreate;
    for ( i=0; interpretations[i].text!=NULL || interpretations[i].line; ++i ) {
	if ( (void *) (sf->uni_interp)==interpretations[i].userdata &&
		interpretations[i].text!=NULL ) {
	    interpretations[i].selected = true;
	    psgcd[k].gd.label = &interpretations[i];
	} else
	    interpretations[i].selected = false;
    }
    ++k;

    psgcd[k].gd.pos.x = psgcd[k-2].gd.pos.x; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y+32;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    pslabel[k].text = (unichar_t *) _("Name List:");
    pslabel[k].text_is_1byte = true;
    pslabel[k].text_in_resource = true;
    psgcd[k].gd.label = &pslabel[k];
    psgcd[k++].creator = GLabelCreate;

    psgcd[k].gd.pos.x = psgcd[k-2].gd.pos.x; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y-6;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    psgcd[k].gd.cid = CID_Namelist;
    psgcd[k].creator = GListButtonCreate;
    nlnames = AllNamelistNames();
    for ( i=0; nlnames[i]!=NULL; ++i);
    namelistnames = gcalloc(i+1,sizeof(GTextInfo));
    for ( i=0; nlnames[i]!=NULL; ++i) {
	namelistnames[i].text = (unichar_t *) nlnames[i];
	namelistnames[i].text_is_1byte = true;
	if ( strcmp(_(sf->for_new_glyphs->title),nlnames[i])==0 ) {
	    namelistnames[i].selected = true;
	    psgcd[k].gd.label = &namelistnames[i];
	}
    }
    psgcd[k++].gd.u.list = namelistnames;
    free(nlnames);


    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<=13; ++i )
	    psgcd[i].gd.flags &= ~gg_enabled;
    } else if ( sf->cidmaster!=NULL ) {
	for ( i=14; i<=16; ++i )
	    psgcd[i].gd.flags &= ~gg_enabled;
    }

    psarray[0] = &psb2[0];
    psarray[1] = &psgcd[14];
    psarray[2] = &psb2[1];
    psarray[3] = &psgcd[17];
    psarray[4] = &psgcd[18];
#ifdef FONTFORGE_CONFIG_TYPE3
    psarray[5] = &psgcd[19];
    psarray[6] = &psgcd[20];
    j=7;
#else
    j=5;
#endif
    psarray[j++] = &psb2[2];
    psrow = j;
    psarray[j++] = GCD_Glue;
    psarray[j++] = NULL;

    psarray2[0] = &psgcd[0]; psarray2[1] = &psgcd[1]; psarray2[2] = &psgcd[2]; psarray2[3] = &psgcd[3]; psarray2[4] = NULL;
    psarray2[5] = &psgcd[4]; psarray2[6] = &psgcd[5]; psarray2[7] = &psgcd[6]; psarray2[8] = GCD_ColSpan; psarray2[9] = NULL;
    psarray2[10] = &psgcd[8]; psarray2[11] = &psgcd[7]; psarray2[12] = &psgcd[9]; psarray2[13] = GCD_ColSpan; psarray2[14] = NULL;
    psarray2[15] = &psgcd[11]; psarray2[16] = &psgcd[10]; psarray2[17] = &psgcd[12]; psarray2[18] = &psgcd[13]; psarray2[19] = NULL;
    psarray2[20] = NULL;

    psarray3[0] = &psgcd[15]; psarray3[1] = &psgcd[16]; psarray3[2] = NULL;

    psarray4[0] = &psgcd[k-6]; psarray4[1] = &psgcd[k-5]; psarray4[2] = NULL;
    psarray4[3] = &psgcd[k-4]; psarray4[4] = &psgcd[k-3]; psarray4[5] = NULL;
    psarray4[6] = &psgcd[k-2]; psarray4[7] = &psgcd[k-1]; psarray4[8] = NULL;
    psarray4[9] = NULL;

    memset(psb,0,sizeof(psb));
    psb[0].gd.flags = gg_enabled|gg_visible;
    psb[0].gd.u.boxelements = psarray;
    psb[0].creator = GVBoxCreate;

    memset(psb2,0,sizeof(psb2));
    psb2[0].gd.flags = gg_enabled|gg_visible;
    psb2[0].gd.u.boxelements = psarray2;
    psb2[0].creator = GHVBoxCreate;

    psb2[1].gd.flags = gg_enabled|gg_visible;
    psb2[1].gd.u.boxelements = psarray3;
    psb2[1].creator = GHBoxCreate;

    psb2[2].gd.flags = gg_enabled|gg_visible;
    psb2[2].gd.u.boxelements = psarray4;
    psb2[2].creator = GHVBoxCreate;
/******************************************************************************/

    memset(&plabel,0,sizeof(plabel));
    memset(&pgcd,0,sizeof(pgcd));

    pgcd[0].gd.pos.x = 10; pgcd[0].gd.pos.y = 6;
    pgcd[0].gd.pos.width = 240; pgcd[0].gd.pos.height = 8*12+10;
    pgcd[0].gd.flags = gg_visible | gg_enabled;
    pgcd[0].gd.cid = CID_PrivateEntries;
    pgcd[0].gd.u.list = PI_ListSet(sf);
    pgcd[0].gd.handle_controlevent = PI_ListSel;
    pgcd[0].creator = GListCreate;

    pgcd[1].gd.pos.x = 10; pgcd[1].gd.pos.y = pgcd[0].gd.pos.y+pgcd[0].gd.pos.height+10;
    pgcd[1].gd.pos.width = pgcd[0].gd.pos.width; pgcd[1].gd.pos.height = 8*12+10;
    pgcd[1].gd.flags = gg_visible | gg_enabled;
    pgcd[1].gd.cid = CID_PrivateValues;
    pgcd[1].creator = GTextAreaCreate;

    pgcd[2].gd.pos.x = 10; pgcd[2].gd.pos.y = 300-35-30;
    pgcd[2].gd.pos.width = -1; pgcd[2].gd.pos.height = 0;
    pgcd[2].gd.flags = gg_visible | gg_enabled ;
    plabel[2].text = (unichar_t *) _("_Add");
    plabel[2].text_is_1byte = true;
    plabel[2].text_in_resource = true;
    pgcd[2].gd.label = &plabel[2];
    pgcd[2].gd.handle_controlevent = PI_Add;
    pgcd[2].gd.cid = CID_Add;
    pgcd[2].creator = GButtonCreate;

    pgcd[3].gd.pos.x = (260)/2-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-5;
    pgcd[3].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[3].gd.pos.width = -1; pgcd[3].gd.pos.height = 0;
    pgcd[3].gd.flags = gg_visible ;
    plabel[3].text = (unichar_t *) _("_Guess");
    plabel[3].text_is_1byte = true;
    plabel[3].text_in_resource = true;
    pgcd[3].gd.label = &plabel[3];
    pgcd[3].gd.handle_controlevent = PI_Guess;
    pgcd[3].gd.cid = CID_Guess;
    pgcd[3].creator = GButtonCreate;

    pgcd[4].gd.pos.x = -(260/2-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-5);
    pgcd[4].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[4].gd.pos.width = -1; pgcd[4].gd.pos.height = 0;
    pgcd[4].gd.flags = gg_visible | gg_utf8_popup ;
/* GT: This is an abbreviation for Histogram */
    plabel[4].text = (unichar_t *) _("_Hist.");
    plabel[4].text_is_1byte = true;
    plabel[4].text_in_resource = true;
    pgcd[4].gd.label = &plabel[4];
    pgcd[4].gd.handle_controlevent = PI_Hist;
    pgcd[4].gd.cid = CID_Hist;
    pgcd[4].gd.popup_msg = (unichar_t *) _("Histogram Dialog");
    pgcd[4].creator = GButtonCreate;

    pgcd[5].gd.pos.x = -10; pgcd[5].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[5].gd.pos.width = -1; pgcd[5].gd.pos.height = 0;
    pgcd[5].gd.flags = gg_visible | gg_enabled ;
    plabel[5].text = (unichar_t *) _("_Remove");
    plabel[5].text_is_1byte = true;
    plabel[5].text_in_resource = true;
    pgcd[5].gd.label = &plabel[5];
    pgcd[5].gd.handle_controlevent = PI_Delete;
    pgcd[5].gd.cid = CID_Remove;
    pgcd[5].creator = GButtonCreate;

    ppbuttons[0] = &pgcd[2]; ppbuttons[1] = &pgcd[3];
    ppbuttons[2] = &pgcd[4]; ppbuttons[3] = &pgcd[5]; ppbuttons[4] = NULL;

    pparray[0] = &pgcd[0]; pparray[1] = &pgcd[1]; pparray[2] = &ppbox[2];
    pparray[3] = NULL;

    memset(ppbox,0,sizeof(ppbox));
    ppbox[0].gd.flags = gg_enabled|gg_visible;
    ppbox[0].gd.u.boxelements = pparray;
    ppbox[0].creator = GVBoxCreate;

    ppbox[2].gd.flags = gg_enabled|gg_visible;
    ppbox[2].gd.u.boxelements = ppbuttons;
    ppbox[2].creator = GHBoxCreate;
/******************************************************************************/
    memset(&vlabel,0,sizeof(vlabel));
    memset(&vgcd,0,sizeof(vgcd));

    vgcd[0].gd.pos.x = 10; vgcd[0].gd.pos.y = 12;
    vlabel[0].text = (unichar_t *) _("_Weight Class");
    vlabel[0].text_is_1byte = true;
    vlabel[0].text_in_resource = true;
    vgcd[0].gd.label = &vlabel[0];
    vgcd[0].gd.flags = gg_visible | gg_enabled;
    vgcd[0].creator = GLabelCreate;

    vgcd[1].gd.pos.x = 90; vgcd[1].gd.pos.y = vgcd[0].gd.pos.y-6; vgcd[1].gd.pos.width = 140;
    vgcd[1].gd.flags = gg_visible | gg_enabled;
    vgcd[1].gd.cid = CID_WeightClass;
    vgcd[1].gd.u.list = weightclass;
    vgcd[1].creator = GListFieldCreate;

    vgcd[2].gd.pos.x = 10; vgcd[2].gd.pos.y = vgcd[1].gd.pos.y+26+6;
    vlabel[2].text = (unichar_t *) _("Width _Class");
    vlabel[2].text_is_1byte = true;
    vlabel[2].text_in_resource = true;
    vgcd[2].gd.label = &vlabel[2];
    vgcd[2].gd.flags = gg_visible | gg_enabled;
    vgcd[2].creator = GLabelCreate;

    vgcd[3].gd.pos.x = 90; vgcd[3].gd.pos.y = vgcd[2].gd.pos.y-6;
    vgcd[3].gd.flags = gg_visible | gg_enabled;
    vgcd[3].gd.cid = CID_WidthClass;
    vgcd[3].gd.u.list = widthclass;
    vgcd[3].creator = GListButtonCreate;

    vgcd[4].gd.pos.x = 10; vgcd[4].gd.pos.y = vgcd[3].gd.pos.y+26+6;
    vlabel[4].text = (unichar_t *) _("P_FM Family");
    vlabel[4].text_is_1byte = true;
    vlabel[4].text_in_resource = true;
    vgcd[4].gd.label = &vlabel[4];
    vgcd[4].gd.flags = gg_visible | gg_enabled;
    vgcd[4].creator = GLabelCreate;

    vgcd[5].gd.pos.x = 90; vgcd[5].gd.pos.y = vgcd[4].gd.pos.y-6; vgcd[5].gd.pos.width = 140;
    vgcd[5].gd.flags = gg_visible | gg_enabled;
    vgcd[5].gd.cid = CID_PFMFamily;
    vgcd[5].gd.u.list = pfmfamily;
    vgcd[5].creator = GListButtonCreate;

    vgcd[6].gd.pos.x = 10; vgcd[6].gd.pos.y = vgcd[5].gd.pos.y+26+6;
    vlabel[6].text = (unichar_t *) _("_Embeddable");
    vlabel[6].text_is_1byte = true;
    vlabel[6].text_in_resource = true;
    vgcd[6].gd.label = &vlabel[6];
    vgcd[6].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    vgcd[6].gd.popup_msg = (unichar_t *) _("Can this font be embedded in a downloadable (pdf)\ndocument, and if so, what behaviors are permitted on\nboth the document and the font.");
    vgcd[6].creator = GLabelCreate;

    vgcd[7].gd.pos.x = 90; vgcd[7].gd.pos.y = vgcd[6].gd.pos.y-6;
    vgcd[7].gd.pos.width = 140;
    vgcd[7].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    vgcd[7].gd.cid = CID_FSType;
    vgcd[7].gd.u.list = fstype;
    vgcd[7].gd.popup_msg = vgcd[6].gd.popup_msg;
    vgcd[7].creator = GListButtonCreate;
    fstype[0].selected = fstype[1].selected =
	    fstype[2].selected = fstype[3].selected = false;
    if ( sf->pfminfo.fstype&0x8 /* also catches the "not set" case == -1 */ )
	i = 2;
    else if ( sf->pfminfo.fstype&0x4 )
	i = 1;
    else if ( sf->pfminfo.fstype&0x2 )
	i = 0;
    else
	i = 3;
    fstype[i].selected = true;
    vgcd[7].gd.label = &fstype[i];

    vgcd[8].gd.pos.x = 20; vgcd[8].gd.pos.y = vgcd[7].gd.pos.y+26;
    vlabel[8].text = (unichar_t *) _("No Subsetting");
    vlabel[8].text_is_1byte = true;
    vgcd[8].gd.label = &vlabel[8];
    vgcd[8].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    if ( sf->pfminfo.fstype!=-1 && (sf->pfminfo.fstype&0x100) )
	vgcd[8].gd.flags |= gg_cb_on;
    vgcd[8].gd.popup_msg = (unichar_t *) _("If set then the entire font must be\nembedded in a document when any character is.\nOtherwise the document creator need\nonly include the characters it uses.");
    vgcd[8].gd.cid = CID_NoSubsetting;
    vgcd[8].creator = GCheckBoxCreate;

    vgcd[9].gd.pos.x = 110; vgcd[9].gd.pos.y = vgcd[8].gd.pos.y;
    vlabel[9].text = (unichar_t *) _("Only Embed Bitmaps");
    vlabel[9].text_is_1byte = true;
    vgcd[9].gd.label = &vlabel[9];
    vgcd[9].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    if ( sf->pfminfo.fstype!=-1 && ( sf->pfminfo.fstype&0x200 ))
	vgcd[9].gd.flags |= gg_cb_on;
    vgcd[9].gd.popup_msg = (unichar_t *) _("Only Bitmaps may be embedded.\nOutline descriptions may not be\n(if font file contains no bitmaps\nthen nothing may be embedded).");
    vgcd[9].gd.cid = CID_OnlyBitmaps;
    vgcd[9].creator = GCheckBoxCreate;

    vgcd[10].gd.pos.x = 10; vgcd[10].gd.pos.y = vgcd[9].gd.pos.y+24;
    vlabel[10].text = (unichar_t *) _("Vendor ID:");
    vlabel[10].text_is_1byte = true;
    vgcd[10].gd.label = &vlabel[10];
    vgcd[10].gd.flags = gg_visible | gg_enabled;
    vgcd[10].creator = GLabelCreate;

    vgcd[11].gd.pos.x = 90; vgcd[11].gd.pos.y = vgcd[11-1].gd.pos.y-4;
    vgcd[11].gd.pos.width = 50;
    vgcd[11].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    vgcd[11].gd.cid = CID_Vendor;
    vgcd[11].creator = GTextFieldCreate;

    vgcd[12].gd.pos.x = 10; vgcd[12].gd.pos.y = vgcd[11].gd.pos.y+24+6;
    vlabel[12].text = (unichar_t *) _("_IBM Family:");
    vlabel[12].text_is_1byte = true;
    vlabel[12].text_in_resource = true;
    vgcd[12].gd.label = &vlabel[12];
    vgcd[12].gd.flags = gg_visible | gg_enabled;
    vgcd[12].creator = GLabelCreate;

    vgcd[13].gd.pos.x = 90; vgcd[13].gd.pos.y = vgcd[12].gd.pos.y-4; vgcd[13].gd.pos.width = vgcd[7].gd.pos.width;
    vgcd[13].gd.flags = gg_visible | gg_enabled;
    vgcd[13].gd.cid = CID_IBMFamily;
    vgcd[13].gd.u.list = ibmfamily;
    vgcd[13].creator = GListButtonCreate;

    vgcd[14].gd.pos.x = 10; vgcd[14].gd.pos.y = vgcd[13].gd.pos.y+24+6;
    vlabel[14].text = (unichar_t *) _("Weight, Width, Slope Only");
    vlabel[14].text_is_1byte = true;
    vlabel[14].text_in_resource = true;
    vgcd[14].gd.label = &vlabel[14];
    vgcd[14].gd.cid = CID_WeightWidthSlopeOnly;
    vgcd[14].gd.popup_msg = (unichar_t *) _("MS needs to know whether a font family's members differ\nonly in weight, width and slope (and not in other variables\nlike optical size)." );
    vgcd[14].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    vgcd[14].creator = GCheckBoxCreate;

    vgcd[15].gd.pos.x = 10; vgcd[15].gd.pos.y = vgcd[11].gd.pos.y+24+6;
    vlabel[15].text = (unichar_t *) _("_OS/2 Version");
    vlabel[15].text_is_1byte = true;
    vlabel[15].text_in_resource = true;
    vgcd[15].gd.label = &vlabel[15];
    vgcd[15].gd.popup_msg = (unichar_t *) _("The 'OS/2' table has changed slightly over the years,\nGenerally fields have been added, but occasionally their\nmeanings have been redefined." );
    vgcd[15].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    vgcd[15].creator = GLabelCreate;

    vgcd[16].gd.pos.x = 90; vgcd[16].gd.pos.y = vgcd[15].gd.pos.y-4; vgcd[16].gd.pos.width = vgcd[7].gd.pos.width;
    vgcd[16].gd.flags = gg_visible | gg_enabled;
    vgcd[16].gd.cid = CID_OS2Version;
    vgcd[16].gd.u.list = os2versions;
    vgcd[16].creator = GListFieldCreate;

    vradio[0] = GCD_Glue; vradio[1] = &vgcd[8]; vradio[2] = &vgcd[9]; vradio[3] = GCD_Glue; vradio[4] = NULL;

    varray[0] = &vgcd[15]; varray[1] = &vgcd[16]; varray[2] = NULL;
    varray[3] = &vgcd[0]; varray[4] = &vgcd[1]; varray[5] = NULL;
    varray[6] = &vgcd[2]; varray[7] = &vgcd[3]; varray[8] = NULL;
    varray[9] = &vgcd[4]; varray[10] = &vgcd[5]; varray[11] = NULL;
    varray[12] = &vgcd[6]; varray[13] = &vgcd[7]; varray[14] = NULL;
    varray[15] = &vbox[2]; varray[16] = GCD_ColSpan; varray[17] = NULL;
    varray[18] = &vgcd[10]; varray[19] = &vgcd[11]; varray[20] = NULL;
    varray[21] = &vgcd[12]; varray[22] = &vgcd[13]; varray[23] = NULL;
    varray[24] = &vgcd[14]; varray[25] = GCD_ColSpan; varray[26] = NULL;
    varray[27] = GCD_Glue; varray[28] = GCD_Glue; varray[29] = NULL;
    varray[30] = GCD_Glue; varray[31] = GCD_Glue; varray[32] = NULL;
    varray[33] = varray[37] = NULL;

    memset(vbox,0,sizeof(vbox));
    vbox[0].gd.flags = gg_enabled|gg_visible;
    vbox[0].gd.u.boxelements = varray;
    vbox[0].creator = GHVBoxCreate;

    vbox[2].gd.flags = gg_enabled|gg_visible;
    vbox[2].gd.u.boxelements = vradio;
    vbox[2].creator = GHBoxCreate;

/******************************************************************************/

    memset(&metgcd,0,sizeof(metgcd));
    memset(&metlabel,'\0',sizeof(metlabel));

    i = j = 0;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = 9;
    metlabel[i].text = (unichar_t *) _("Win _Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Anything outside the OS/2 WinAscent &\nWinDescent fields will be clipped by windows.\nThis includes marks, etc. that have been repositioned by GPOS.\n(The descent field is usually positive.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in OS/2.\nIf set then any number you enter will be added to the\nfont's bounds. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.");
    metgcd[i].gd.cid = CID_WinAscentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinAscentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("Win _Descent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_WinDescentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinDescentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("_Typo Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("The type ascent&descent fields are>supposed<\nto specify the line spacing on windows.\nIn fact usually the win ascent/descent fields do.\n(The descent field is usually negative.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in OS/2.\nIf set then any number you enter will be added to the\nEm-size. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.");
    metgcd[i].gd.cid = CID_TypoAscentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoAscentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 5; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Really use Typo metrics");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_UseTypoMetrics;
    metgcd[i].gd.popup_msg = (unichar_t *) _("The spec already says that the typeo metrics should be\nused to determine line spacing. But so many\nprograms fail to follow the spec that MS desided a bit\nwas needed to remind them to do so.");
    metarray[j++] = &metgcd[i]; metarray[j++] = GCD_ColSpan; metarray[j++] = GCD_Glue;
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("T_ypo Descent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_TypoDescentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoDescentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("Typo Line _Gap:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the TypoLinegap field in the OS/2 table, used on MS Windows");
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* Line gap value set later */
    metgcd[i].gd.cid = CID_TypoLineGap;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;
    metarray[j++] = GCD_Glue;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("_HHead Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("This specifies the line spacing on the mac.\n(The descent field is usually negative.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in hhea.\nIf set then any number you enter will be added to the\nfont's bounds. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.");
    metgcd[i].gd.cid = CID_HHeadAscentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadAscentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("HHead De_scent Offset:");
    metlabel[i].text_in_resource = true;
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_HHeadDescentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadDescentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("HHead _Line Gap:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the linegap field in the hhea table, used on the mac");
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* Line gap value set later */
    metgcd[i].gd.cid = CID_LineGap;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;
    metarray[j++] = GCD_Glue;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y+26+6;
    metlabel[i].text = (unichar_t *) _("VHead _Column Spacing:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled | gg_utf8_popup) : (gg_visible | gg_utf8_popup);
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the linegap field in the vhea table.\nThis is the horizontal spacing between rows\nof vertically set text.");
    metgcd[i].gd.cid = CID_VLineGapLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-6;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled | gg_utf8_popup) : (gg_visible | gg_utf8_popup);
	/* V Line gap value set later */
    metgcd[i].gd.cid = CID_VLineGap;
    metgcd[i].gd.popup_msg = metgcd[17].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;
    metarray[j++] = GCD_Glue;
    metarray[j++] = NULL;
    metarray[j++] = GCD_Glue; metarray[j++] = GCD_Glue; metarray[j++] = GCD_Glue;

    metarray[j++] = NULL; metarray[j++] = NULL;

    memset(metbox,0,sizeof(metbox));
    metbox[0].gd.flags = gg_enabled|gg_visible;
    metbox[0].gd.u.boxelements = metarray;
    metbox[0].creator = GHVBoxCreate;
/******************************************************************************/

    memset(&ssgcd,0,sizeof(ssgcd));
    memset(&sslabel,'\0',sizeof(sslabel));

    i = j = 0;

    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = 5;
    sslabel[i].text = (unichar_t *) S_("SubscriptSuperUse|Default");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    ssgcd[i].gd.cid = CID_SubSuperDefault;
    ssgcd[i].gd.handle_controlevent = GFI_SubSuperDefault;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GCheckBoxCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue;
    ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+16;
    sslabel[i].text = (unichar_t *) _("Subscript");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 120; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-4;
/* GT: X is a coordinate, the leading spaces help to align it */
    sslabel[i].text = (unichar_t *) _("  X");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 180; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
/* GT: Y is a coordinate, the leading spaces help to align it */
    sslabel[i].text = (unichar_t *) _("Y");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubXSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubYSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Offset");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubXOffset;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubYOffset;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;


    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+30;
    sslabel[i].text = (unichar_t *) _("Superscript");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue;
    ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperXSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperYSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Offset");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperXOffset;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperYOffset;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;


    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+30;
    sslabel[i].text = (unichar_t *) _("Strikeout");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue;
    ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_StrikeoutSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-2].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Pos");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_StrikeoutPos;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue;

    ssarray[j++] = NULL; ssarray[j++] = NULL;

    memset(ssbox,0,sizeof(ssbox));
    ssbox[0].gd.flags = gg_enabled|gg_visible;
    ssbox[0].gd.u.boxelements = ssarray;
    ssbox[0].creator = GHVBoxCreate;
/******************************************************************************/
    memset(&panlabel,0,sizeof(panlabel));
    memset(&pangcd,0,sizeof(pangcd));

    i = j = 0;

    pangcd[i].gd.pos.x = 5; pangcd[i].gd.pos.y = 5;
    panlabel[i].text = (unichar_t *) S_("PanoseUse|Default");
    panlabel[i].text_is_1byte = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    pangcd[i].gd.cid = CID_PanDefault;
    /*pangcd[i].gd.popup_msg = pangcd[i-1].gd.popup_msg;*/
    pangcd[i].gd.handle_controlevent = GFI_PanoseDefault;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GCheckBoxCreate;
    panarray[j++] = GCD_Glue; panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+14+4;
    panlabel[i].text = (unichar_t *) S_("Panose|_Family");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanFamilyLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanFamily;
    pangcd[i].gd.u.list = panfamily;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Serifs");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanSerifsLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanSerifs;
    pangcd[i].gd.u.list = panserifs;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) S_("Panose|_Weight");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanWeightLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanWeight;
    pangcd[i].gd.u.list = panweight;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Proportion");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanPropLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanProp;
    pangcd[i].gd.u.list = panprop;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Contrast");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanContrastLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanContrast;
    pangcd[i].gd.u.list = pancontrast;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("Stroke _Variation");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanStrokeVarLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanStrokeVar;
    pangcd[i].gd.u.list = panstrokevar;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Arm Style");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanArmStyleLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanArmStyle;
    pangcd[i].gd.u.list = panarmstyle;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Letterform");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanLetterformLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanLetterform;
    pangcd[i].gd.u.list = panletterform;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Midline");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanMidLineLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanMidLine;
    pangcd[i].gd.u.list = panmidline;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_X-Height");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanXHeightLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanXHeight;
    pangcd[i].gd.u.list = panxheight;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    panarray[j++] = GCD_Glue; panarray[j++] = GCD_Glue;

    panarray[j++] = NULL; panarray[j++] = NULL;

    memset(panbox,0,sizeof(panbox));
    panbox[0].gd.flags = gg_enabled|gg_visible;
    panbox[0].gd.u.boxelements = panarray;
    panbox[0].creator = GHVBoxCreate;
/******************************************************************************/

    memset(&vagcd,0,sizeof(vagcd));
    memset(&vaspects,'\0',sizeof(vaspects));

    i = 0;
    vaspects[i].text = (unichar_t *) _("Misc.");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = vbox;

    vaspects[i].text = (unichar_t *) _("Metrics");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = metbox;

    vaspects[i].text = (unichar_t *) _("Sub/Super");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = ssbox;

    vaspects[i].text = (unichar_t *) _("Panose");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = panbox;

    vagcd[0].gd.pos.x = 4; vagcd[0].gd.pos.y = 6;
    vagcd[0].gd.pos.width = 252;
    vagcd[0].gd.pos.height = 300;
    vagcd[0].gd.u.tabs = vaspects;
    vagcd[0].gd.flags = gg_visible | gg_enabled;
    /*vagcd[0].gd.handle_controlevent = GFI_TTFAspectChange;*/
    vagcd[0].gd.cid = CID_TTFTabs;
    vagcd[0].creator = GTabSetCreate;

/******************************************************************************/
    memset(&gaspboxes,0,sizeof(gaspboxes));
    memset(&gaspgcd,0,sizeof(gaspgcd));
    memset(&gasplabel,0,sizeof(gasplabel));
    memset(&gaspgcd_def,0,sizeof(gaspgcd_def));

    GaspMatrixInit(&gaspmi,d);

    i = j = 0;

    gasplabel[i].text = (unichar_t *) S_("Gasp|_Version");
    gasplabel[i].text_is_1byte = true;
    gasplabel[i].text_in_resource = true;
    gaspgcd[i].gd.label = &gasplabel[i];
    gaspgcd[i].gd.flags = gg_visible | gg_enabled;
    gaspharray[j++] = &gaspgcd[i];
    gaspgcd[i++].creator = GLabelCreate;

    gaspgcd[i].gd.flags = gg_visible | gg_enabled;
    gaspgcd[i].gd.cid = CID_GaspVersion;
    gaspgcd[i].gd.label = &gaspversions[sf->gasp_version];
    gaspgcd[i].gd.u.list = gaspversions;
    gaspharray[j++] = &gaspgcd[i];
    gaspgcd[i].gd.handle_controlevent = GFI_GaspVersion;
    gaspgcd[i++].creator = GListButtonCreate;
    gaspharray[j++] = GCD_HPad10;

    gasplabel[i].text = (unichar_t *) _("Optimized For Cleartype");
    gasplabel[i].text_is_1byte = true;
    gasplabel[i].text_in_resource = true;
    gaspgcd[i].gd.label = &gasplabel[i];
    gaspgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gaspgcd[i].gd.cid = CID_HeadClearType;
    if ( sf->head_optimized_for_cleartype )
	gaspgcd[i].gd.flags |= gg_cb_on;
    gaspgcd[i].gd.popup_msg = (unichar_t *) _("Actually a bit in the 'head' table.\nIf unset then certain East Asian fonts will not be hinted");
    gaspharray[j++] = &gaspgcd[i];
    gaspgcd[i++].creator = GCheckBoxCreate;
    gaspharray[j++] = NULL;

    gaspboxes[2].gd.flags = gg_enabled|gg_visible;
    gaspboxes[2].gd.u.boxelements = gaspharray;
    gaspboxes[2].creator = GHBoxCreate;

    gaspvarray[0] = &gaspboxes[2];

    gaspgcd[i].gd.pos.width = 300; gaspgcd[i].gd.pos.height = 200;
    gaspgcd[i].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gaspgcd[i].gd.cid = CID_Gasp;
    gaspgcd[i].gd.u.matrix = &gaspmi;
    gaspgcd[i].gd.popup_msg = (unichar_t *) _(
	"The 'gasp' table gives you control over when grid-fitting and\n"
	"anti-aliased rasterizing are done.\n"
	"The table consists of an (ordered) list of pixel sizes each with\n"
	"a set of flags. Those flags apply to all pixel sizes bigger than\n"
	"the previous table entry but less than or equal to the current.\n"
	"The list must be terminated with a pixel size of 65535.\n"
	"Version 1 of the table contains two additional flags that\n"
	"apply to MS's ClearType rasterizer.\n\n"
	"The 'gasp' table only applies to truetype fonts.");
    gaspgcd[i].data = d;
    gaspgcd[i].creator = GMatrixEditCreate;
    gaspvarray[1] = &gaspgcd[i];
    gaspvarray[2] = NULL;

    gaspboxes[0].gd.flags = gg_enabled|gg_visible;
    gaspboxes[0].gd.u.boxelements = gaspvarray;
    gaspboxes[0].creator = GVBoxCreate;

    gaspgcd_def[0].gd.flags = gg_visible | gg_enabled;
    gasplabel[4].text = (unichar_t *) S_("Gasp|_Default");
    gasplabel[4].text_is_1byte = true;
    gasplabel[4].text_in_resource = true;
    gaspgcd_def[0].gd.label = &gasplabel[4];
    gaspgcd_def[0].gd.handle_controlevent = Gasp_Default;
    gaspgcd_def[0].creator = GButtonCreate;
/******************************************************************************/
    memset(&tnlabel,0,sizeof(tnlabel));
    memset(&tngcd,0,sizeof(tngcd));
    memset(&tnboxes,0,sizeof(tnboxes));

    TNMatrixInit(&mi,d);

    tngcd[0].gd.pos.x = 5; tngcd[0].gd.pos.y = 6;
    tngcd[0].gd.flags = gg_visible | gg_enabled;
    tnlabel[0].text = (unichar_t *) _("Sort By:");
    tnlabel[0].text_is_1byte = true;
    tngcd[0].gd.label = &tnlabel[0];
    tngcd[0].creator = GLabelCreate;
    tnharray[0] = &tngcd[0];

    tngcd[1].gd.pos.x = 50; tngcd[1].gd.pos.y = tngcd[0].gd.pos.y-4;
    tngcd[1].gd.flags = gg_enabled | gg_visible;
    tngcd[1].gd.cid = CID_TNLangSort;
    tnlabel[1].text = (unichar_t *) _("_Language");
    tnlabel[1].text_is_1byte = true;
    tnlabel[1].text_in_resource = true;
    tngcd[1].gd.label = &tnlabel[1];
    tngcd[1].creator = GRadioCreate;
    tngcd[1].gd.handle_controlevent = GFI_SortBy;
    tnharray[1] = &tngcd[1];

    tngcd[2].gd.pos.x = 120; tngcd[2].gd.pos.y = tngcd[1].gd.pos.y;
    tngcd[2].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    tngcd[2].gd.cid = CID_TNStringSort;
    tnlabel[2].text = (unichar_t *) _("_String Type");
    tnlabel[2].text_is_1byte = true;
    tnlabel[2].text_in_resource = true;
    tngcd[2].gd.label = &tnlabel[2];
    tngcd[2].creator = GRadioCreate;
    tngcd[2].gd.handle_controlevent = GFI_SortBy;
    tnharray[2] = &tngcd[2];

    tngcd[3].gd.pos.x = 195; tngcd[3].gd.pos.y = tngcd[1].gd.pos.y;
    tngcd[3].gd.flags = gg_visible | gg_enabled | gg_cb_on | gg_rad_continueold;
    tnlabel[3].text = (unichar_t *) S_("SortingScheme|Default");
    tnlabel[3].text_is_1byte = true;
    tngcd[3].gd.label = &tnlabel[3];
    tngcd[3].creator = GRadioCreate;
    tngcd[3].gd.handle_controlevent = GFI_SortBy;
    tnharray[3] = &tngcd[3]; tnharray[4] = GCD_Glue; tnharray[5] = NULL;

    tngcd[4].gd.pos.x = 10; tngcd[4].gd.pos.y = tngcd[1].gd.pos.y+14;
    tngcd[4].gd.pos.width = 300; tngcd[4].gd.pos.height = 200;
    tngcd[4].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    tngcd[4].gd.cid = CID_TNames;
    tngcd[4].gd.u.matrix = &mi;
    tngcd[4].gd.popup_msg = (unichar_t *) _(
	"To create a new name, left click on the <New> button, and select a locale.\n"
	"To change the locale, left click on it.\n"
	"To change the string type, left click on it.\n"
	"To change the text, left click in it and then type.\n"
	"To delete a name, right click on the name & select Delete from the menu.\n"
	"To associate or disassocate a truetype name to its postscript equivalent\n"
	"right click and select the appropriate menu item." );
    tngcd[4].data = d;
    tngcd[4].creator = GMatrixEditCreate;

    tngcd[5].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
/* GT: when translating this please leave the "SIL Open Font License" in */
/* GT: English (possibly translating it in parentheses). I believe there */
/* GT: are legal reasons for this. */
/* GT: So "A\303\261adir SIL Open Font License (licencia de fuentes libres)" */
    tnlabel[5].text = (unichar_t *) S_("Add SIL ");
    tnlabel[5].image_precedes = false;
    tnlabel[5].image = &OFL_logo;
    tnlabel[5].text_is_1byte = true;
    tnlabel[5].text_in_resource = true;
    tngcd[5].gd.label = &tnlabel[5];
    tngcd[5].gd.handle_controlevent = GFI_AddOFL;
    tngcd[5].gd.popup_msg = (unichar_t *) _(
	"The SIL Open Font License (OFL) is designed for free/libre/open font projects.\n"
	"Most other FLOSS licenses are designed for conventional software and are problematic for fonts.\n"
	"The OFL is a community-approved license and is well-suited for releasing fonts to be freely \n"
	"used, studied, copied, modified, embedded, merged and distributed while maintaining artistic integrity.\n"
        "You are encouraged you to use it if you can.\n"
	"\n"
	"For more details about the OFL - and the corresponding FAQ - see http://scripts.sil.org/OFL \n"
	"\n"
	"Simply press this button to add the OFL metadata to your font.\n"
	"\n");
    tngcd[5].creator = GButtonCreate;

    tngcd[6].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    tnlabel[6].text = (unichar_t *) S_("OFL website");
    tnlabel[6].text_is_1byte = true;
    tnlabel[6].text_in_resource = true;
    tngcd[6].gd.label = &tnlabel[6];
    tngcd[6].gd.handle_controlevent = GFI_HelpOFL;
    tngcd[6].gd.popup_msg = (unichar_t *) _(
	"\n"
	"Click here to go to http://scripts.sil.org/OFL \n"
	"to get all the details about the Open Font License \n"
	"and to read the corresponding FAQ. \n"
	"\n");
    tngcd[6].creator = GButtonCreate;
    tnharray2[0] = &tngcd[5]; tnharray2[1] = &tngcd[6]; tnharray2[2] = GCD_Glue; tnharray2[3] = NULL;
    tnvarray[0] = &tnboxes[2]; tnvarray[1] = &tngcd[4]; tnvarray[2] = &tnboxes[3]; tnvarray[3] = NULL;

    tnboxes[0].gd.flags = gg_enabled|gg_visible;
    tnboxes[0].gd.u.boxelements = tnvarray;
    tnboxes[0].creator = GVBoxCreate;

    tnboxes[2].gd.flags = gg_enabled|gg_visible;
    tnboxes[2].gd.u.boxelements = tnharray;
    tnboxes[2].creator = GHBoxCreate;

    tnboxes[3].gd.flags = gg_enabled|gg_visible;
    tnboxes[3].gd.u.boxelements = tnharray2;
    tnboxes[3].creator = GHBoxCreate;
/******************************************************************************/
    memset(&comlabel,0,sizeof(comlabel));
    memset(&comgcd,0,sizeof(comgcd));

    comgcd[0].gd.pos.x = 10; comgcd[0].gd.pos.y = 10;
    comgcd[0].gd.pos.width = ngcd[11].gd.pos.width; comgcd[0].gd.pos.height = 230;
    comgcd[0].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    comgcd[0].gd.cid = CID_Comment;
    comlabel[0].text = (unichar_t *) sf->comments;
    comlabel[0].text_is_1byte = true;
    if ( comlabel[0].text==NULL ) comlabel[0].text = (unichar_t *) "";
    comgcd[0].gd.label = &comlabel[0];
    comgcd[0].creator = GTextAreaCreate;

    comarray[0] = &comgcd[0]; comarray[1] = NULL;
    memset(combox,0,sizeof(combox));
    combox[0].gd.flags = gg_enabled|gg_visible;
    combox[0].gd.u.boxelements = comarray;
    combox[0].creator = GHBoxCreate;

/******************************************************************************/
    memset(&mklabel,0,sizeof(mklabel));
    memset(&mkgcd,0,sizeof(mkgcd));

    mkgcd[0].gd.pos.x = 10; mkgcd[0].gd.pos.y = 10;
    mkgcd[0].gd.pos.width = ngcd[15].gd.pos.width; mkgcd[0].gd.pos.height = 200;
    mkgcd[0].gd.flags = gg_visible | gg_enabled;
    mkgcd[0].gd.cid = CID_MarkClasses;
    mkgcd[0].gd.u.list = MarkClassesList(sf);
    mkgcd[0].gd.handle_controlevent = GFI_MarkSelChanged;
    mkgcd[0].creator = GListCreate;

    mkgcd[1].gd.pos.x = 10; mkgcd[1].gd.pos.y = mkgcd[0].gd.pos.y+mkgcd[0].gd.pos.height+4;
    mkgcd[1].gd.pos.width = -1;
    mkgcd[1].gd.flags = gg_visible | gg_enabled;
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "_New...", ignore "Mark|" */
    mklabel[1].text = (unichar_t *) S_("Mark|_New...");
    mklabel[1].text_is_1byte = true;
    mklabel[1].text_in_resource = true;
    mkgcd[1].gd.label = &mklabel[1];
    mkgcd[1].gd.cid = CID_MarkNew;
    mkgcd[1].gd.handle_controlevent = GFI_MarkNew;
    mkgcd[1].creator = GButtonCreate;

    mkgcd[2].gd.pos.x = -10; mkgcd[2].gd.pos.y = mkgcd[1].gd.pos.y;
    mkgcd[2].gd.pos.width = -1;
    mkgcd[2].gd.flags = gg_visible;
    mklabel[2].text = (unichar_t *) _("_Edit...");
    mklabel[2].text_is_1byte = true;
    mklabel[2].text_in_resource = true;
    mkgcd[2].gd.label = &mklabel[2];
    mkgcd[2].gd.cid = CID_MarkEdit;
    mkgcd[2].gd.handle_controlevent = GFI_MarkEdit;
    mkgcd[2].creator = GButtonCreate;

    mkarray[0] = &mkgcd[0]; mkarray[1] = &mkbox[2]; mkarray[2] = NULL;
    mkarray2[0] = &mkgcd[1]; mkarray2[1] = GCD_Glue; mkarray2[2] = &mkgcd[2];
     mkarray2[3] = NULL;
    memset(mkbox,0,sizeof(mkbox));
    mkbox[0].gd.flags = gg_enabled|gg_visible;
    mkbox[0].gd.u.boxelements = mkarray;
    mkbox[0].creator = GVBoxCreate;

    mkbox[2].gd.flags = gg_enabled|gg_visible;
    mkbox[2].gd.u.boxelements = mkarray2;
    mkbox[2].creator = GHBoxCreate;
/******************************************************************************/
    memset(&txlabel,0,sizeof(txlabel));
    memset(&txgcd,0,sizeof(txgcd));

    k=0;
    txlabel[k].text = (unichar_t *) U_("\316\244\316\265\316\247 General");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = 10;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_TeXText;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) U_("\316\244\316\265\316\247 Math Symbol");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 80; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y;
    txgcd[k].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    txgcd[k].gd.cid = CID_TeXMathSym;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) U_("\316\244\316\265\316\247 Math Extension");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 155; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y;
    txgcd[k].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    txgcd[k].gd.cid = CID_TeXMathExt;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    for ( i=j=0; texparams[i]!=0; ++i ) {
	txlabel[k].text = (unichar_t *) texparams[i];
	txlabel[k].text_is_1byte = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = txgcd[k-2].gd.pos.y+26;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	txgcd[k].gd.popup_msg = (unichar_t *) texpopups[i];
	txarray2[j++] = &txgcd[k];
	txgcd[k++].creator = GLabelCreate;

	txgcd[k].gd.pos.x = 70; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y-4;
	txgcd[k].gd.flags = gg_visible | gg_enabled;
	txgcd[k].gd.cid = CID_TeX + i;
	txarray2[j++] = &txgcd[k];
	txgcd[k++].creator = GTextFieldCreate;
	txarray2[j++] = GCD_Glue;
	txarray2[j++] = NULL;
    }
    txgcd[k-2].gd.cid = CID_TeXExtraSpLabel;
    txarray2[j++] = NULL;

/* GT: More Parameters */
    txlabel[k].text = (unichar_t *) _("More Params");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 20; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y+26;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.handle_controlevent = GFI_MoreParams;
    txgcd[k].gd.cid = CID_MoreParams;
    txgcd[k++].creator = GButtonCreate;

    txarray[0] = &txbox[2]; txarray[1] = &txbox[3]; txarray[2] = &txbox[4]; txarray[3] = GCD_Glue; txarray[4] = NULL;
    txarray3[0] = &txgcd[0]; txarray3[1] = GCD_Glue; txarray3[2] = &txgcd[1];
     txarray3[3] = GCD_Glue; txarray3[4] = &txgcd[2]; txarray3[5] = NULL;
    txarray4[0] = GCD_Glue; txarray4[1] = &txgcd[k-1];
     txarray4[2] = txarray4[3] = txarray4[4] = GCD_Glue; txarray4[5] = NULL;
    memset(txbox,0,sizeof(txbox));
    txbox[0].gd.flags = gg_enabled|gg_visible;
    txbox[0].gd.u.boxelements = txarray;
    txbox[0].creator = GVBoxCreate;

    txbox[2].gd.flags = gg_enabled|gg_visible;
    txbox[2].gd.u.boxelements = txarray3;
    txbox[2].creator = GHBoxCreate;

    txbox[3].gd.flags = gg_enabled|gg_visible;
    txbox[3].gd.u.boxelements = txarray2;
    txbox[3].creator = GHVBoxCreate;

    txbox[4].gd.flags = gg_enabled|gg_visible;
    txbox[4].gd.u.boxelements = txarray4;
    txbox[4].creator = GHBoxCreate;
/******************************************************************************/
    memset(&szlabel,0,sizeof(szlabel));
    memset(&szgcd,0,sizeof(szgcd));

    k=0;

    szlabel[k].text = (unichar_t *) _("De_sign Size:");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 10; szgcd[k].gd.pos.y = 9;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The size (in points) for which this face was designed");
    szgcd[k++].creator = GLabelCreate;

    sprintf(dszbuf, "%.1f", sf->design_size/10.0);
    szlabel[k].text = (unichar_t *) dszbuf;
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 70; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y-4;
    szgcd[k].gd.pos.width = 60;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szgcd[k].gd.cid = CID_DesignSize;
    szgcd[k++].creator = GTextFieldCreate;

    szlabel[k].text = (unichar_t *) S_("Size|Points");
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 134; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The size (in points) for which this face was designed");
    szgcd[k++].creator = GLabelCreate;

    szlabel[k].text = (unichar_t *) _("Design Range");
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 14; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y+24;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The range of sizes (in points) to which this face applies.\nLower bound is exclusive, upper bound is inclusive.");
    szgcd[k++].creator = GLabelCreate;

    szgcd[k].gd.pos.x = 8; szgcd[k].gd.pos.y = GDrawPointsToPixels(NULL,szgcd[k-1].gd.pos.y+6);
    szgcd[k].gd.pos.width = pos.width-32; szgcd[k].gd.pos.height = GDrawPointsToPixels(NULL,36);
    szgcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    szgcd[k++].creator = GGroupCreate;

    szlabel[k].text = (unichar_t *) _("_Bottom:");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 14; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y+18;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The range of sizes (in points) to which this face applies.\nLower bound is exclusive, upper bound is inclusive.");
    szgcd[k++].creator = GLabelCreate;

    sprintf(dsbbuf, "%.1f", sf->design_range_bottom/10.0);
    szlabel[k].text = (unichar_t *) dsbbuf;
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 70; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y-4;
    szgcd[k].gd.pos.width = 60;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szgcd[k].gd.cid = CID_DesignBottom;
    szgcd[k++].creator = GTextFieldCreate;

    szlabel[k].text = (unichar_t *) _("_Top:");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 140; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The range of sizes (in points) to which this face applies.\nLower bound is exclusive, upper bound is inclusive.");
    szgcd[k++].creator = GLabelCreate;

    sprintf(dstbuf, "%.1f", sf->design_range_top/10.0);
    szlabel[k].text = (unichar_t *) dstbuf;
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 180; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y-4;
    szgcd[k].gd.pos.width = 60;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szgcd[k].gd.cid = CID_DesignTop;
    szgcd[k++].creator = GTextFieldCreate;

    szlabel[k].text = (unichar_t *) _("Style _ID:");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 14; szgcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,szgcd[k-5].gd.pos.y+szgcd[k-5].gd.pos.height)+10;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("This is an identifying number shared by all members of\nthis font family with the same style (I.e. 10pt Bold and\n24pt Bold would have the same id, but 10pt Italic would not");
    szgcd[k++].creator = GLabelCreate;

    sprintf(sibuf, "%d", sf->fontstyle_id);
    szlabel[k].text = (unichar_t *) sibuf;
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 70; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y-4;
    szgcd[k].gd.pos.width = 60;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szgcd[k].gd.cid = CID_StyleID;
    szgcd[k++].creator = GTextFieldCreate;

    szlabel[k].text = (unichar_t *) _("Style Name:");
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 14; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y+22;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("This provides a set of names used to identify the\nstyle of this font. Names may be translated into multiple\nlanguages (English is required, others are optional)\nAll fonts with the same Style ID should share this name.");
    szgcd[k++].creator = GLabelCreate;

    szgcd[k].gd.pos.x = 10; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y+14;
    szgcd[k].gd.pos.width = ngcd[15].gd.pos.width; szgcd[k].gd.pos.height = 100;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
    szgcd[k].gd.cid = CID_StyleName;
    szgcd[k].gd.handle_controlevent = GFI_StyleNameSelChanged;
    szgcd[k++].creator = GListCreate;

    szgcd[k].gd.pos.x = 10; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y+szgcd[k-1].gd.pos.height+4;
    szgcd[k].gd.pos.width = -1;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szlabel[k].text = (unichar_t *) S_("StyleName|_New...");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.cid = CID_StyleNameNew;
    szgcd[k].gd.handle_controlevent = GFI_StyleNameNew;
    szgcd[k++].creator = GButtonCreate;

    szgcd[k].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y;
    szgcd[k].gd.pos.width = -1;
    szgcd[k].gd.flags = gg_visible;
    szlabel[k].text = (unichar_t *) _("_Delete");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.cid = CID_StyleNameDel;
    szgcd[k].gd.handle_controlevent = GFI_StyleNameDel;
    szgcd[k++].creator = GButtonCreate;

    szgcd[k].gd.pos.x = 10 + 2*(10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor));
    szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y;
    szgcd[k].gd.pos.width = -1;
    szgcd[k].gd.flags = gg_visible;
    szlabel[k].text = (unichar_t *) _("_Edit...");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.cid = CID_StyleNameRename;
    szgcd[k].gd.handle_controlevent = GFI_StyleNameRename;
    szgcd[k++].creator = GButtonCreate;

    szarray[0] = &szbox[2];
    szarray[1] = &szbox[3];
    szarray[2] = &szbox[4];
    szarray[3] = &szgcd[11];
    szarray[4] = &szgcd[12];
    szarray[5] = &szbox[5];
    szarray[6] = NULL;

    szarray2[0] = &szgcd[0]; szarray2[1] = &szgcd[1]; szarray2[2] = &szgcd[2]; szarray2[3] = GCD_Glue; szarray2[4] = NULL;
    szarray3[0] = &szgcd[5]; szarray3[1] = &szgcd[6]; szarray3[2] = &szgcd[7]; szarray3[3] = &szgcd[8]; szarray3[4] = GCD_Glue; szarray3[5] = NULL;
     szarray3[6] = NULL;
    szarray4[0] = &szgcd[9]; szarray4[1] = &szgcd[10]; szarray4[2] = GCD_Glue; szarray4[3] = NULL;
    szarray5[0] = &szgcd[13]; szarray5[1] = GCD_Glue;
     szarray5[2] = &szgcd[14]; szarray5[3] = GCD_Glue;
     szarray5[4] = &szgcd[15]; szarray5[5] = NULL;

    memset(szbox,0,sizeof(szbox));
    szbox[0].gd.flags = gg_enabled|gg_visible;
    szbox[0].gd.u.boxelements = szarray;
    szbox[0].creator = GVBoxCreate;

    szbox[2].gd.flags = gg_enabled|gg_visible;
    szbox[2].gd.u.boxelements = szarray2;
    szbox[2].creator = GHBoxCreate;

    szbox[3].gd.flags = gg_enabled|gg_visible;
    szbox[3].gd.label = (GTextInfo *) &szgcd[3];
    szbox[3].gd.u.boxelements = szarray3;
    szbox[3].creator = GHVGroupCreate;

    szbox[4].gd.flags = gg_enabled|gg_visible;
    szbox[4].gd.u.boxelements = szarray4;
    szbox[4].creator = GHBoxCreate;

    szbox[5].gd.flags = gg_enabled|gg_visible;
    szbox[5].gd.u.boxelements = szarray5;
    szbox[5].creator = GHBoxCreate;

/******************************************************************************/
    memset(&mcgcd,0,sizeof(mcgcd));
    memset(&mclabel,'\0',sizeof(mclabel));

    k=0;
    mclabel[k].text = (unichar_t *) _("Mac Style Set:");
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = 7;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k++].creator = GLabelCreate;

    mclabel[k].text = (unichar_t *) _("Automatic");
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.flags = (sf->macstyle==-1) ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    mcgcd[k].gd.cid = CID_MacAutomatic;
    mcgcd[k].gd.handle_controlevent = GFI_MacAutomatic;
    mcgcd[k++].creator = GRadioCreate;

    mcgcd[k].gd.pos.x = 90; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.flags = (sf->macstyle!=-1) ? (gg_visible | gg_enabled | gg_cb_on | gg_rad_continueold) : (gg_visible | gg_enabled | gg_rad_continueold);
    mcgcd[k].gd.handle_controlevent = GFI_MacAutomatic;
    mcgcd[k++].creator = GRadioCreate;

    mcgcd[k].gd.pos.x = 110; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.pos.width = 120; mcgcd[k].gd.pos.height = 7*12+10;
    mcgcd[k].gd.flags = (sf->macstyle==-1) ? (gg_visible | gg_list_multiplesel) : (gg_visible | gg_enabled | gg_list_multiplesel);
    mcgcd[k].gd.cid = CID_MacStyles;
    mcgcd[k].gd.u.list = macstyles;
    mcgcd[k++].creator = GListCreate;

    mclabel[k].text = (unichar_t *) _("FOND Name:");
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = mcgcd[k-1].gd.pos.y + mcgcd[k-1].gd.pos.height+8;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k++].creator = GLabelCreate;

    mclabel[k].text = (unichar_t *) sf->fondname;
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = sf->fondname==NULL ? NULL : &mclabel[k];
    mcgcd[k].gd.pos.x = 90; mcgcd[k].gd.pos.y = mcgcd[k-1].gd.pos.y - 4;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k].gd.cid = CID_MacFOND;
    mcgcd[k++].creator = GTextFieldCreate;


    mcs = MacStyleCode(sf,NULL);
    for ( i=0; macstyles[i].text!=NULL; ++i )
	macstyles[i].selected = (mcs&(int) (intpt) macstyles[i].userdata)? 1 : 0;

    mcarray[0] = &mcgcd[0]; mcarray[1] = GCD_Glue; mcarray[2] = NULL;
    mcarray[3] = &mcbox[2]; mcarray[4] = &mcgcd[3]; mcarray[5] = NULL;
    mcarray[6] = &mcgcd[4]; mcarray[7] = &mcgcd[5]; mcarray[8] = NULL;
    mcarray[9] = GCD_Glue; mcarray[10] = GCD_Glue; mcarray[11] = NULL;
    mcarray[12] = NULL;
    mcarray2[0] = &mcgcd[1]; mcarray2[1] = &mcgcd[2]; mcarray2[2] = NULL;
    mcarray2[3] = GCD_Glue; mcarray2[4] = GCD_Glue; mcarray2[5] = NULL;
    mcarray2[6] = NULL;

    memset(mcbox,0,sizeof(mcbox));
    mcbox[0].gd.flags = gg_enabled|gg_visible;
    mcbox[0].gd.u.boxelements = mcarray;
    mcbox[0].creator = GHVBoxCreate;

    mcbox[2].gd.flags = gg_enabled|gg_visible;
    mcbox[2].gd.u.boxelements = mcarray2;
    mcbox[2].creator = GHVBoxCreate;

/******************************************************************************/
    memset(&lksubgcd,0,sizeof(lksubgcd));
    memset(&lkgcd,0,sizeof(lkgcd));
    memset(&lkaspects,'\0',sizeof(lkaspects));
    memset(&lkbox,'\0',sizeof(lkbox));
    memset(&lkbuttonsgcd,'\0',sizeof(lkbuttonsgcd));
    memset(&lkbuttonslabel,'\0',sizeof(lkbuttonslabel));

    LookupSetup(&d->tables[0],sf->gsub_lookups);
    LookupSetup(&d->tables[1],sf->gpos_lookups);

    i=0;
    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Moves the currently selected lookup to be first in the lookup ordering\nor moves the currently selected subtable to be first in its lookup.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Top");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupTop;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupOrder;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Moves the currently selected lookup before the previous lookup\nor moves the currently selected subtable before the previous subtable.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Up");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupUp;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupOrder;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Moves the currently selected lookup after the next lookup\nor moves the currently selected subtable after the next subtable.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Down");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupDown;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupOrder;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Moves the currently selected lookup to the end of the lookup chain\nor moves the currently selected subtable to be the last subtable in the lookup");
    lkbuttonslabel[i].text = (unichar_t *) _("_Bottom");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupBottom;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupOrder;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Sorts the lookups in a default ordering based on feature tags");
    lkbuttonslabel[i].text = (unichar_t *) _("_Sort");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupSort;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupSort;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_enabled ;
    lkbuttonsgcd[i++].creator = GLineCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Adds a new lookup after the selected lookup\nor at the start of the lookup list if nothing is selected.");
    lkbuttonslabel[i].text = (unichar_t *) _("Add _Lookup");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_AddLookup;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupAddLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Adds a new lookup subtable after the selected subtable\nor at the start of the lookup if nothing is selected.");
    lkbuttonslabel[i].text = (unichar_t *) _("Add Sub_table");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_AddSubtable;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupAddSubtable;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Edits a lookup or lookup subtable.");
    lkbuttonslabel[i].text = (unichar_t *) _("Edit _Metadata");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_EditMetadata;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupEditMetadata;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Edits the transformations in a lookup subtable.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Edit Data");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_EditSubtable;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupEditSubtableContents;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Deletes any selected lookups and their subtables, or deletes any selected subtables.\nThis will also delete any transformations associated with those subtables.");
    lkbuttonslabel[i].text = (unichar_t *) _("De_lete");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_DeleteLookup;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupDeleteLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Merges two selected (and compatible) lookups into one,\nor merges two selected subtables of a lookup into one");
    lkbuttonslabel[i].text = (unichar_t *) _("_Merge");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_MergeLookup;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupMergeLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Reverts the lookup list to its original condition.\nBut any changes to subtable data will remain.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Revert");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_RevertLookups;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupRevertLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Imports a lookup (and all its subtables) from another font.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Import");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_ImportLookups;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupImportLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;
    lkbuttonsarray[i] = GCD_Glue;
    lkbuttonsarray[i+1] = NULL;

    for ( i=0; i<2; ++i ) {
	lkaspects[i].text = (unichar_t *) (i?"GPOS":"GSUB");
	lkaspects[i].text_is_1byte = true;
	lkaspects[i].gcd = &lkbox[2*i];

	lksubgcd[i][0].gd.pos.x = 10; lksubgcd[i][0].gd.pos.y = 10;
	lksubgcd[i][0].gd.pos.width = ngcd[15].gd.pos.width;
	lksubgcd[i][0].gd.pos.height = 150;
	lksubgcd[i][0].gd.flags = gg_visible | gg_enabled;
	lksubgcd[i][0].gd.u.drawable_e_h = i ? gposlookups_e_h : gsublookups_e_h;
	lksubgcd[i][0].gd.cid = CID_LookupWin+i;
	lksubgcd[i][0].creator = GDrawableCreate;

	lksubgcd[i][1].gd.pos.x = 10; lksubgcd[i][1].gd.pos.y = 10;
	lksubgcd[i][1].gd.pos.height = 150;
	lksubgcd[i][1].gd.flags = gg_visible | gg_enabled | gg_sb_vert;
	lksubgcd[i][1].gd.cid = CID_LookupVSB+i;
	lksubgcd[i][1].gd.handle_controlevent = LookupsVScroll;
	lksubgcd[i][1].creator = GScrollBarCreate;

	lksubgcd[i][2].gd.pos.x = 10; lksubgcd[i][2].gd.pos.y = 10;
	lksubgcd[i][2].gd.pos.width = 150;
	lksubgcd[i][2].gd.flags = gg_visible | gg_enabled;
	lksubgcd[i][2].gd.cid = CID_LookupHSB+i;
	lksubgcd[i][2].gd.handle_controlevent = LookupsHScroll;
	lksubgcd[i][2].creator = GScrollBarCreate;

	lksubgcd[i][3].gd.pos.x = 10; lksubgcd[i][3].gd.pos.y = 10;
	lksubgcd[i][3].gd.pos.width = lksubgcd[i][3].gd.pos.height = _GScrollBar_Width;
	lksubgcd[i][3].gd.flags = gg_visible | gg_enabled | gg_tabset_nowindow;
	lksubgcd[i][3].creator = GDrawableCreate;

	lkarray[i][0] = &lksubgcd[i][0]; lkarray[i][1] = &lksubgcd[i][1]; lkarray[i][2] = NULL;
	lkarray[i][3] = &lksubgcd[i][2]; lkarray[i][4] = &lksubgcd[i][3]; lkarray[i][5] = NULL;
	lkarray[i][6] = NULL;

	lkbox[2*i].gd.flags = gg_enabled|gg_visible;
	lkbox[2*i].gd.u.boxelements = lkarray[i];
	lkbox[2*i].creator = GHVBoxCreate;
    }

    lkaspects[0].selected = true;

    lkgcd[0].gd.pos.x = 4; lkgcd[0].gd.pos.y = 10;
    lkgcd[0].gd.pos.width = 250;
    lkgcd[0].gd.pos.height = 260;
    lkgcd[0].gd.u.tabs = lkaspects;
    lkgcd[0].gd.flags = gg_visible | gg_enabled;
    lkgcd[0].gd.cid = CID_Lookups;
    lkgcd[0].gd.handle_controlevent = GFI_LookupAspectChange;
    lkgcd[0].creator = GTabSetCreate;

    lkharray[0] = &lkgcd[0]; lkharray[1] = &lkbox[4]; lkharray[2] = NULL;

    lkbox[4].gd.flags = gg_enabled|gg_visible;
    lkbox[4].gd.u.boxelements = lkbuttonsarray;
    lkbox[4].creator = GVBoxCreate;

    lkbox[5].gd.flags = gg_enabled|gg_visible;
    lkbox[5].gd.u.boxelements = lkharray;
    lkbox[5].creator = GHBoxCreate;
    

/******************************************************************************/
    memset(&mfgcd,0,sizeof(mfgcd));
    memset(&mflabel,'\0',sizeof(mflabel));
    memset(mfbox,0,sizeof(mfbox));

    GCDFillMacFeat(mfgcd,mflabel,250,sf->features, false, mfbox, mfarray);
/******************************************************************************/

    memset(&dlabel,0,sizeof(dlabel));
    memset(&dgcd,0,sizeof(dgcd));

    dlabel[0].text = (unichar_t *) _("Creation Date:");
    dlabel[0].text_is_1byte = true;
    dlabel[0].text_in_resource = true;
    dgcd[0].gd.label = &dlabel[0];
    dgcd[0].gd.pos.x = 12; dgcd[0].gd.pos.y = 6+6; 
    dgcd[0].gd.flags = gg_visible | gg_enabled;
    dgcd[0].creator = GLabelCreate;

    t = sf->creationtime;
    tm = localtime(&t);
    strftime(createtime,sizeof(createtime),"%c",tm);
    tmpcreatetime = def2u_copy(createtime);
    dgcd[1].gd.pos.x = 115; dgcd[1].gd.pos.y = dgcd[0].gd.pos.y;
    dgcd[1].gd.flags = gg_visible | gg_enabled;
    dlabel[1].text = tmpcreatetime;
    dgcd[1].gd.label = &dlabel[1];
    dgcd[1].creator = GLabelCreate;

    dlabel[2].text = (unichar_t *) _("Modification Date:");
    dlabel[2].text_is_1byte = true;
    dlabel[2].text_in_resource = true;
    dgcd[2].gd.label = &dlabel[2];
    dgcd[2].gd.pos.x = 12; dgcd[2].gd.pos.y = dgcd[0].gd.pos.y+14; 
    dgcd[2].gd.flags = gg_visible | gg_enabled;
    dgcd[2].creator = GLabelCreate;

    t = sf->modificationtime;
    tm = localtime(&t);
    strftime(modtime,sizeof(modtime),"%c",tm);
    tmpmodtime = def2u_copy(modtime);
    dgcd[3].gd.pos.x = 115; dgcd[3].gd.pos.y = dgcd[2].gd.pos.y;
    dgcd[3].gd.flags = gg_visible | gg_enabled;
    dlabel[3].text = tmpmodtime;
    dgcd[3].gd.label = &dlabel[3];
    dgcd[3].creator = GLabelCreate;

    darray[0] = &dgcd[0]; darray[1] = &dgcd[1]; darray[2] = NULL;
    darray[3] = &dgcd[2]; darray[4] = &dgcd[3]; darray[5] = NULL;
    darray[6] = GCD_Glue; darray[7] = GCD_Glue; darray[8] = NULL;
    darray[9] = NULL;

    memset(dbox,0,sizeof(dbox));
    dbox[0].gd.flags = gg_enabled|gg_visible;
    dbox[0].gd.u.boxelements = darray;
    dbox[0].creator = GHVBoxCreate;
/******************************************************************************/

    memset(&ulabel,0,sizeof(ulabel));
    memset(&ugcd,0,sizeof(ugcd));

    ulabel[0].text = (unichar_t *) _("Include Empty Blocks");
    ulabel[0].text_is_1byte = true;
    ulabel[0].text_in_resource = true;
    ugcd[0].gd.label = &ulabel[0];
    ugcd[0].gd.pos.x = 12; ugcd[0].gd.pos.y = 10; 
    ugcd[0].gd.flags = gg_visible | gg_enabled;
    ugcd[0].gd.handle_controlevent = GFI_UnicodeEmptiesChange;
    ugcd[0].gd.cid = CID_UnicodeEmpties;
    ugcd[0].creator = GCheckBoxCreate;

    ugcd[1].gd.pos.x = 12; ugcd[1].gd.pos.y = 30;
    ugcd[1].gd.pos.width = ngcd[15].gd.pos.width;
    ugcd[1].gd.pos.height = 200;
    ugcd[1].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    ugcd[1].gd.cid = CID_Unicode;
    ugcd[1].gd.handle_controlevent = GFI_UnicodeRangeChange;
    ugcd[1].gd.popup_msg = (unichar_t *) _("Click on a range to select characters in that range.\nDouble click on a range to see characters that should be\nin the range but aren't.");
    ugcd[1].creator = GListCreate;

    uarray[0] = &ugcd[0]; uarray[1] = &ugcd[1]; uarray[2] = NULL;

    memset(ubox,0,sizeof(ubox));
    ubox[0].gd.flags = gg_enabled|gg_visible;
    ubox[0].gd.u.boxelements = uarray;
    ubox[0].creator = GVBoxCreate;

/******************************************************************************/

    memset(&mlabel,0,sizeof(mlabel));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&aspects,'\0',sizeof(aspects));

    i = 0;

    aspects[i].text = (unichar_t *) _("Names");
    d->old_aspect = 0;
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = nb;

    aspects[i].text = (unichar_t *) _("General");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = psb;

    aspects[i].text = (unichar_t *) _("PS UID");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = xub;

    d->private_aspect = i;
    aspects[i].text = (unichar_t *) _("PS Private");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = ppbox;

    d->ttfv_aspect = i;
    aspects[i].text = (unichar_t *) _("OS/2");
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = vagcd;

    d->tn_aspect = i;
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("TTF Names");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = tnboxes;

    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("Grid Fitting");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = gaspboxes;

    d->tx_aspect = i;
/* xgettext won't use non-ASCII messages */
    aspects[i].text = (unichar_t *) U_("\316\244\316\265\316\247");	/* Tau epsilon Chi, in greek */
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = txbox;

    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("Size");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = szbox;

    aspects[i].text = (unichar_t *) _("Comment");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = combox;

    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("Mark Classes");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mkbox;

/* GT: OpenType GPOS/GSUB lookups */
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) S_("OpenType|Lookups");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = &lkbox[5];

    aspects[i].text = (unichar_t *) _("Mac");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mcbox;

    aspects[i].text = (unichar_t *) _("Mac Features");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mfbox;

#ifndef FONTFORGE_CONFIG_INFO_HORIZONTAL
    aspects[i].text = (unichar_t *) _("Dates");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = dbox;

    d->unicode_aspect = i;
    aspects[i].text = (unichar_t *) _("Unicode Ranges");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = ubox;
#endif

    aspects[defaspect].selected = true;

    mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
    mgcd[0].gd.u.tabs = aspects;
#ifndef FONTFORGE_CONFIG_INFO_HORIZONTAL
    mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
    mgcd[0].gd.pos.width = 260+85;
    mgcd[0].gd.pos.height = 325;
#else
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.pos.width = 260;
    mgcd[0].gd.pos.height = 325;
#endif
    mgcd[0].gd.handle_controlevent = GFI_AspectChange;
    mgcd[0].gd.cid = CID_Tabs;
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.pos.x = 30-3; mgcd[1].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _("_OK");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = GFI_OK;
    mgcd[1].gd.cid = CID_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.pos.x = -30; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y+3;
    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _("_Cancel");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = GFI_Cancel;
    mgcd[2].gd.cid = CID_Cancel;
    mgcd[2].creator = GButtonCreate;

    mgcd[3].gd.pos.x = 2; mgcd[3].gd.pos.y = 2;
    mgcd[3].gd.pos.width = pos.width-4; mgcd[3].gd.pos.height = pos.height-4;
    mgcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    mgcd[3].gd.cid = CID_MainGroup;
    mgcd[3].creator = GGroupCreate;

    marray2[0] = marray2[2] = marray2[3] = marray2[4] = marray2[6] = GCD_Glue; marray2[7] = NULL;
    marray2[1] = &mgcd[1]; marray2[5] = &mgcd[2];

    marray[0] = &mgcd[0]; marray[1] = NULL;
    marray[2] = &mb2; marray[3] = NULL;
    marray[4] = GCD_Glue; marray[5] = NULL;
    marray[6] = NULL;

    memset(mb,0,sizeof(mb));
    mb[0].gd.pos.x = mb[0].gd.pos.y = 2;
    mb[0].gd.flags = gg_enabled|gg_visible;
    mb[0].gd.u.boxelements = marray;
    mb[0].creator = GHVBoxCreate;

    memset(&mb2,0,sizeof(mb2));
    mb2.gd.flags = gg_enabled|gg_visible;
    mb2.gd.u.boxelements = marray2;
    mb2.creator = GHBoxCreate;

    GGadgetsCreate(gw,mb);
    GMatrixEditSetNewText(tngcd[4].ret,S_("TrueTypeName|New"));
    GGadgetSelectOneListItem(gaspgcd[0].ret,sf->gasp_version);
    GMatrixEditSetNewText(gaspgcd[3].ret,S_("gaspTableEntry|New"));
    GMatrixEditAddButtons(gaspgcd[3].ret,gaspgcd_def);
    if ( sf->gasp_version==0 ) {
	GMatrixEditEnableColumn(gaspgcd[3].ret,3,false);
	GMatrixEditEnableColumn(gaspgcd[3].ret,4,false);
    }
    GHVBoxSetExpandableCol(gaspboxes[2].ret,2);
    GHVBoxSetExpandableRow(gaspboxes[0].ret,1);

    GHVBoxSetExpandableRow(mb[0].ret,0);
    GHVBoxSetExpandableCol(mb2.ret,gb_expandgluesame);

    GHVBoxSetExpandableCol(nb[0].ret,1);
    GHVBoxSetExpandableRow(nb[0].ret,7);
    GHVBoxSetExpandableCol(nb2.ret,1);
    GHVBoxSetExpandableCol(nb3.ret,1);

    GHVBoxSetExpandableCol(xub[0].ret,1);
    GHVBoxSetExpandableRow(xub[0].ret,3);

    GHVBoxSetExpandableRow(psb[0].ret,psrow);
    GHVBoxSetExpandableCol(psb2[0].ret,3);
    GHVBoxSetExpandableCol(psb2[1].ret,1);
    GHVBoxSetExpandableCol(psb2[2].ret,1);

    GHVBoxSetExpandableRow(ppbox[0].ret,0);
    GHVBoxSetExpandableCol(ppbox[2].ret,gb_samesize);

    GHVBoxSetExpandableRow(vbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(vbox[0].ret,1);
    GHVBoxSetExpandableCol(vbox[2].ret,gb_expandglue);

    GHVBoxSetExpandableRow(metbox[0].ret,10);
    GHVBoxSetExpandableCol(metbox[0].ret,1);

    GHVBoxSetExpandableRow(ssbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(ssbox[0].ret,gb_expandglue);

    GHVBoxSetExpandableRow(panbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(panbox[0].ret,1);

    GHVBoxSetExpandableRow(mkbox[0].ret,0);
    GHVBoxSetExpandableCol(mkbox[2].ret,gb_expandglue);

    GHVBoxSetExpandableRow(txbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(txbox[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(txbox[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(txbox[4].ret,gb_expandglue);

    GHVBoxSetExpandableRow(szbox[0].ret,4);
    GHVBoxSetPadding(szbox[2].ret,6,2);
    GHVBoxSetExpandableCol(szbox[2].ret,gb_expandglue);
    GHVBoxSetPadding(szbox[3].ret,6,2);
    GHVBoxSetExpandableCol(szbox[3].ret,gb_expandglue);
    GHVBoxSetPadding(szbox[4].ret,6,2);
    GHVBoxSetExpandableCol(szbox[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(szbox[5].ret,gb_expandglue);

    GHVBoxSetExpandableRow(tnboxes[0].ret,1);
    GHVBoxSetExpandableCol(tnboxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(tnboxes[3].ret,gb_expandglue);

    GHVBoxSetExpandableRow(mcbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(mcbox[0].ret,1);
    GHVBoxSetExpandableRow(mcbox[2].ret,gb_expandglue);

    GHVBoxSetExpandableRow(mfbox[0].ret,0);
    GHVBoxSetExpandableRow(mfbox[2].ret,gb_expandglue);

    GHVBoxSetExpandableRow(dbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(dbox[0].ret,1);

    GHVBoxSetExpandableRow(ubox[0].ret,1);

    GHVBoxSetExpandableCol(lkbox[0].ret,0);
    GHVBoxSetExpandableRow(lkbox[0].ret,0);
    GHVBoxSetPadding(lkbox[0].ret,0,0);
    GHVBoxSetExpandableCol(lkbox[2].ret,0);
    GHVBoxSetExpandableRow(lkbox[2].ret,0);
    GHVBoxSetPadding(lkbox[2].ret,0,0);
    GHVBoxSetExpandableRow(lkbox[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(lkbox[5].ret,0);

    GFI_LookupEnableButtons(d,true);
    GFI_LookupEnableButtons(d,false);

    memset(&rq,0,sizeof(rq));
    rq.family_name = sans;
    rq.point_size = 12;
    rq.weight = 400;
    d->font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(d->font,&as,&ds,&ld);
    d->as = as; d->fh = as+ds;

    GTextInfoListFree(namelistnames);
    GTextInfoListFree(pgcd[0].gd.u.list);

    for ( i=0; i<mi.initial_row_cnt; ++i )
	free( mi.matrix_data[3*i+2].u.md_str );
    free( mi.matrix_data );

    free(tmpcreatetime);
    free(tmpmodtime);

    GHVBoxFitWindow(mb[0].ret);
#if 0
    if ( GTabSetGetTabLines(mgcd[0].ret)>3 ) {
	int offset = (GTabSetGetTabLines(mgcd[0].ret)-2)*GDrawPointsToPixels(NULL,20);
	GDrawResize(gw,pos.width,pos.height+offset);
    }
#endif

    GWidgetIndicateFocusGadget(ngcd[1].ret);
    ProcessListSel(d);
    GFI_AspectChange(mgcd[0].ret,NULL);
    GFI_InitMarkClasses(d);
    GGadgetSetList(GWidgetGetControl(gw,CID_StyleName),StyleNames(sf->fontstyle_name),false);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);

    if ( sync ) {
	while ( !d->done )
	    GDrawProcessOneEvent(NULL);
    }
}

void FontMenuFontInfo(void *_fv) {
    FontInfo( ((FontView *) _fv)->sf,-1,false);
}

void FontInfoDestroy(SplineFont *sf) {
    if ( sf->fontinfo )
	GFI_CancelClose( (struct gfi_data *) (sf->fontinfo) );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */


void FontInfoInit(void) {
    static int done = false;
    int i, j;
    static GTextInfo *needswork[] = {
	macstyles, widthclass, weightclass, fstype, pfmfamily, ibmfamily,
	panfamily, panserifs, panweight, panprop, pancontrast, panstrokevar,
	panarmstyle, panletterform, panmidline, panxheight, mslanguages,
	ttfnameids, interpretations, gridfit, antialias, os2versions,
	NULL
    };
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    static char **needswork2[] = { texparams, texpopups,
	mathparams, mathpopups, extparams, extpopups,
    NULL };
#endif
    if ( done )
return;
    done = true;
#ifndef _NO_PYTHON
    scriptingSaveEnglishNames(ttfnameids,mslanguages);
#endif
    for ( j=0; needswork[j]!=NULL; ++j ) {
	for ( i=0; needswork[j][i].text!=NULL; ++i )
	    needswork[j][i].text = (unichar_t *) S_((char *) needswork[j][i].text);
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    for ( j=0; needswork2[j]!=NULL; ++j ) {
	for ( i=0; needswork2[j][i]!=NULL; ++i )
	    needswork2[j][i] = _(needswork2[j][i]);
    }

    ci[0].title = S_(ci[0].title);
    ci[1].title = S_(ci[1].title);
    ci[2].title = S_(ci[2].title);

    gaspci[0].title = S_(gaspci[0].title);
    gaspci[1].title = S_(gaspci[1].title);
    gaspci[2].title = S_(gaspci[2].title);
    gaspci[3].title = S_(gaspci[3].title);
    gaspci[4].title = S_(gaspci[4].title);
#endif
}
