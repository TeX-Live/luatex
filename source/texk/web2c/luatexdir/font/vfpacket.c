/* vfpacket.c
   
   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
   Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>

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

static const char _svn_version[] =
    "$Id$ $URL$";

/*
  The |do_vf_packet| procedure is called in order to interpret the
  character packet for a virtual character. Such a packet may contain
  the instruction to typeset a character from the same or an other
  virtual font; in such cases |do_vf_packet| calls itself
  recursively. The recursion level, i.e., the number of times this has
  happened, is kept in the global variable |packet_cur_s| and should
  not exceed |packet_max_recursion|.
*/

#define packet_max_recursion 100

typedef unsigned char packet_stack_index;       /* an index into the stack */

typedef struct packet_stack_record {
    scaled stack_h;
    scaled stack_v;
} packet_stack_record;


static packet_stack_index packet_cur_s = 0;     /* current recursion level */
static packet_stack_record packet_stack[packet_max_recursion];
static packet_stack_index packet_stack_ptr = 0; /* pointer into |packet_stack| */


/* Some macros for processing character packets. */

#define do_packet_byte() vf_packets[cur_packet_byte++]

#define packet_number(fw)  {              \
  fw = do_packet_byte();                  \
  fw = fw*256 + do_packet_byte();         \
  fw = fw*256 + do_packet_byte();         \
  fw = fw*256 + do_packet_byte(); }

#define packet_scaled(a,fs) { integer fw; \
  fw = do_packet_byte();                  \
  if (fw>127) fw = fw - 256;              \
  fw = fw*256 + do_packet_byte();         \
  fw = fw*256 + do_packet_byte();         \
  fw = fw*256 + do_packet_byte();         \
    a = store_scaled_f(fw, fs); }


/* count the number of bytes in a command packet */
int vf_packet_bytes(charinfo * co)
{
    eight_bits *vf_packets;
    integer cur_packet_byte;
    unsigned k;
    int cmd;

    vf_packets = get_charinfo_packets(co);
    if (vf_packets == NULL) {
        return 0;
    }
    cur_packet_byte = 0;
    while ((cmd = vf_packets[cur_packet_byte]) != packet_end_code) {
        cur_packet_byte++;
        switch (cmd) {
        case packet_char_code:
        case packet_font_code:
        case packet_right_code:
        case packet_down_code:
        case packet_node_code:
            cur_packet_byte += 4;
            break;
        case packet_push_code:
        case packet_pop_code:
            break;
        case packet_rule_code:
            cur_packet_byte += 8;
            break;
        case packet_special_code:
            packet_number(k);   /* +4 */
            cur_packet_byte += k;
            break;
        case packet_image_code:
            cur_packet_byte += 4;
            break;
        case packet_nop_code:
            break;
        default:
            pdf_error("vf", "invalid DVI command (1)");
        }
    };
    return (cur_packet_byte + 1);
}


/* typeset the \.{DVI} commands in the
   character packet for character |c| in current font |f| */

char *packet_command_names[] = {
    "char", "font", "pop", "push", "special", "image",
    "right", "down", "rule", "node", "nop", "end", NULL
};


void do_vf_packet(PDF pdf, internal_font_number vf_f, integer c)
{
    internal_font_number lf;
    charinfo *co;
    scaledpos save_cur;
    eight_bits *vf_packets;
    integer cur_packet_byte;
    integer cmd, fs_f;
    scaled i;
    unsigned k;
    str_number s;

    posstructure localpos;      /* the position structure local within this function */
    posstructure *refpos;       /* the list origin pos. on the page, provided by the caller */

    lf = 0;                     /* for -Wall */
    packet_cur_s++;
    if (packet_cur_s >= packet_max_recursion)
        overflow("max level recursion of virtual fonts", packet_max_recursion);
    co = get_charinfo(vf_f, c);
    vf_packets = get_charinfo_packets(co);
    if (vf_packets == NULL) {
        packet_cur_s--;
        return;
    }

    refpos = pdf->posstruct;
    pdf->posstruct = &localpos; /* use local structure for recursion */
    localpos.dir = dir_TL_;     /* invariably for vf */

    save_cur = cur;             /* TODO: make cur local */
    set_to_zero(cur);

    cur_packet_byte = 0;
    fs_f = font_size(vf_f);
    while ((cmd = vf_packets[cur_packet_byte]) != packet_end_code) {
        cur_packet_byte++;
        /*
           if (cmd>packet_end_code) {
           fprintf(stdout, "do_vf_packet(%i,%i) command code = illegal \n", vf_f,c);
           } else {
           fprintf(stdout, "do_vf_packet(%i,%i) command code = %s\n",vf_f, c, packet_command_names[cmd]);
           }
         */
        switch (cmd) {
        case packet_font_code:
            packet_number(lf);
            break;
        case packet_push_code:
            packet_stack[packet_stack_ptr].stack_h = cur.h;
            packet_stack[packet_stack_ptr].stack_v = cur.v;
            packet_stack_ptr++;
            break;
        case packet_pop_code:
            packet_stack_ptr--;
            cur.h = packet_stack[packet_stack_ptr].stack_h;
            cur.v = packet_stack[packet_stack_ptr].stack_v;
            break;
        case packet_char_code:
            packet_number(k);
            if (!char_exists(lf, k)) {
                char_warning(lf, k);
            } else {
                (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                if (has_packet(lf, k))
                    do_vf_packet(pdf, lf, k);
                else
                    pdf_place_glyph(pdf, lf, k);

            }
            cur.h = cur.h + char_width(lf, k);
            break;
        case packet_rule_code:
            packet_scaled(rule.ht, fs_f);
            packet_scaled(rule.wd, fs_f);
            if ((rule.wd > 0) && (rule.ht > 0)) {
                (void) new_synch_pos_with_cur(&localpos, refpos, cur);
                pdf_place_rule(pdf, rule.wd, rule.ht);
            }
            cur.h = cur.h + rule.wd;
            break;
        case packet_right_code:
            packet_scaled(i, fs_f);
            cur.h = cur.h + i;
            break;
        case packet_down_code:
            packet_scaled(i, fs_f);
            cur.v = cur.v + i;
            break;
        case packet_special_code:
            packet_number(k);
            str_room(k);
            while (k > 0) {
                k--;
                append_char(do_packet_byte());
            }
            s = make_string();
            (void) new_synch_pos_with_cur(&localpos, refpos, cur);
            pdf_literal(pdf, s, scan_special, false);
            flush_str(s);
            break;
        case packet_image_code:
            packet_number(k);
            pos = new_synch_pos_with_cur(&localpos, refpos, cur);
            vf_out_image(k, pos);
            break;
        case packet_node_code:
            packet_number(k);
            temp_ptr = k;
            (void) new_synch_pos_with_cur(&localpos, refpos, cur);
            pdf_hlist_out(pdf);
            break;
        case packet_nop_code:
            break;
        default:
            pdf_error("vf", "invalid DVI command (2)");
        }
    };
    cur = save_cur;
    packet_cur_s--;
    pdf->posstruct = refpos;
}

integer *packet_local_fonts(internal_font_number f, integer * num)
{
    int c, cmd, cur_packet_byte, lf, k, l, i;
    integer localfonts[256] = { 0 };
    integer *lfs;
    charinfo *co;

    eight_bits *vf_packets;
    k = 0;
    for (c = font_bc(f); c <= font_ec(f); c++) {
        if (char_exists(f, c)) {
            co = get_charinfo(f, c);
            vf_packets = get_charinfo_packets(co);
            if (vf_packets == NULL)
                continue;
            cur_packet_byte = 0;
            while ((cmd = vf_packets[cur_packet_byte]) != packet_end_code) {
                cur_packet_byte++;
                switch (cmd) {
                case packet_font_code:
                    packet_number(lf);
                    for (l = 0; l < k; l++) {
                        if (localfonts[l] == lf) {
                            break;
                        }
                    }
                    if (l == k) {
                        localfonts[k++] = lf;
                    }
                    break;
                case packet_push_code:
                case packet_pop_code:
                case packet_nop_code:
                    break;
                case packet_char_code:
                case packet_right_code:
                case packet_down_code:
                case packet_node_code:
                    cur_packet_byte += 4;
                    break;
                case packet_rule_code:
                    cur_packet_byte += 8;
                    break;
                case packet_special_code:
                    packet_number(i);
                    while (i-- > 0)
                        (void) do_packet_byte();
                    break;
                case packet_image_code:
                    cur_packet_byte += 4;
                    break;
                default:
                    pdf_error("vf", "invalid DVI command (3)");
                }
            }
        }
    }
    *num = k;
    if (k > 0) {
        lfs = xmalloc(k * sizeof(integer));
        memcpy(lfs, localfonts, k * sizeof(integer));
        return lfs;
    }
    return NULL;
}


void
replace_packet_fonts(internal_font_number f, integer * old_fontid,
                     integer * new_fontid, int count)
{
    int c, cmd, cur_packet_byte, lf, k, l;
    charinfo *co;
    eight_bits *vf_packets;

    k = 0;
    for (c = font_bc(f); c <= font_ec(f); c++) {
        if (char_exists(f, c)) {
            co = get_charinfo(f, c);
            vf_packets = get_charinfo_packets(co);
            if (vf_packets == NULL)
                continue;
            cur_packet_byte = 0;
            while ((cmd = vf_packets[cur_packet_byte]) != packet_end_code) {
                cur_packet_byte++;
                switch (cmd) {
                case packet_font_code:
                    packet_number(lf);
                    for (l = 0; l < count; l++) {
                        if (old_fontid[l] == lf) {
                            break;
                        }
                    }
                    if (l < count) {
                        k = new_fontid[l];
                        vf_packets[(cur_packet_byte - 4)] =
                            (k & 0xFF000000) >> 24;
                        vf_packets[(cur_packet_byte - 3)] =
                            (k & 0x00FF0000) >> 16;
                        vf_packets[(cur_packet_byte - 2)] =
                            (k & 0x0000FF00) >> 8;
                        vf_packets[(cur_packet_byte - 1)] = (k & 0x000000FF);
                    }
                    break;
                case packet_push_code:
                case packet_pop_code:
                case packet_nop_code:
                    break;
                case packet_char_code:
                case packet_right_code:
                case packet_down_code:
                case packet_node_code:
                    cur_packet_byte += 4;
                    break;
                case packet_rule_code:
                    cur_packet_byte += 8;
                    break;
                case packet_special_code:
                    packet_number(k);
                    while (k-- > 0)
                        (void) do_packet_byte();
                    break;
                case packet_image_code:
                    cur_packet_byte += 4;
                    break;
                default:
                    pdf_error("vf", "invalid DVI command (4)");
                }
            }
        }
    }
}
