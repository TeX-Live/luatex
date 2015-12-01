/* lfontlib.c

   Copyright 2006-2014 Taco Hoekwater <taco@luatex.org>

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
#include "lua/luatex-api.h"

#define TIMERS 0

#if TIMERS
#  include <sys/time.h>
#endif

static int get_fontid(void)
{
    if (font_tables == NULL || font_tables[0] == NULL) {
        create_null_font();
    }
    return new_font();
}

static int font_read_tfm(lua_State * L)
{
    internal_font_number f;
    scaled s;
    int k;
    const char *cnom;
    if (lua_type(L, 1) == LUA_TSTRING) {
        cnom = lua_tostring(L, 1);
        if (lua_type(L, 2) == LUA_TNUMBER) {
            s = (int) lua_tointeger(L, 2);
            if (strlen(cnom)) {
                f = get_fontid();
                if (read_tfm_info(f, cnom, s)) {
                    k = font_to_lua(L, f);
                    delete_font(f);
                    return k;
                } else {
                    delete_font(f);
                    luaL_error(L, "font loading failed");
                }
            } else {
                luaL_error(L, "expected tfm name as first argument");
            }
        } else {
            luaL_error(L, "expected an integer size as second argument");
        }
    } else {
        luaL_error(L, "expected tfm name as first argument");
    }
    return 2;                   /* not reached */
}


static int font_read_vf(lua_State * L)
{
    int i;
    const char *cnom;
    if (lua_type(L, 1) == LUA_TSTRING) {
        cnom = lua_tostring(L, 1);
        if (strlen(cnom)) {
            if (lua_type(L, 2) == LUA_TNUMBER) {
                i = (int) lua_tointeger(L, 2);
                return make_vf_table(L, cnom, (scaled) i);
            } else {
                luaL_error(L, "expected an integer size as second argument");
                return 2;
            }
        }
    }
    luaL_error(L, "expected vf name as first argument");
    return 2;                   /* not reached */
}

static int tex_current_font(lua_State * L)
{
    int i;
    i = (int) luaL_optinteger(L, 1, 0);
    if (i > 0) {
        if (is_valid_font(i)) {
            zset_cur_font(i);
            return 0;
        } else {
            luaL_error(L, "expected a valid font id");
            return 2;           /* not reached */
        }
    } else {
        lua_pushinteger(L, get_cur_font());
        return 1;
    }
}

static int tex_max_font(lua_State * L)
{
    lua_pushinteger(L, max_font_id());
    return 1;
}


static int tex_each_font_next(lua_State * L)
{
    int m = lua_tointeger(L, 1);
    int i = lua_tointeger(L, 2);
    i++;
    while (i <= m && !is_valid_font(i))
        i++;
    if (i > m) {
        lua_pushnil(L);
        return 1;
    } else {
        lua_pushinteger(L, i);
        if (!font_to_lua(L, i))
            lua_pushnil(L);
        return 2;
    }
}

static int tex_each_font(lua_State * L)
{
    lua_pushcclosure(L, tex_each_font_next, 0);
    lua_pushinteger(L, max_font_id());
    lua_pushinteger(L, 0);
    return 3;
}

static int frozenfont(lua_State * L)
{
    int i;
    i = (int) luaL_checkinteger(L, 1);
    if (i) {
        if (is_valid_font(i)) {
            if (font_touched(i) || font_used(i)) {
                lua_pushboolean(L, 1);
            } else {
                lua_pushboolean(L, 0);
            }
        } else {
            lua_pushnil(L);
        }
        return 1;
    } else {
        luaL_error(L, "expected an integer argument");
    }
    return 0;                   /* not reached */
}


static int setfont(lua_State * L)
{
    int i;
    i = (int) luaL_checkinteger(L, -2);
    if (i) {
        luaL_checktype(L, -1, LUA_TTABLE);
        if (is_valid_font(i)) {
            if (!(font_touched(i) || font_used(i))) {
                font_from_lua(L, i);
            } else {
                luaL_error(L,
                           "that font has been accessed already, changing it is forbidden");
            }
        } else {
            luaL_error(L, "that integer id is not a valid font");
        }
    }
    return 0;
}


static int deffont(lua_State * L)
{
    int i;
#if TIMERS
    struct timeval tva;
    struct timeval tvb;
    double tvdiff;
#endif
    luaL_checktype(L, -1, LUA_TTABLE);
    i = get_fontid();
#if TIMERS
    gettimeofday(&tva, NULL);
#endif
    if (font_from_lua(L, i)) {
#if TIMERS
        gettimeofday(&tvb, NULL);
        tvdiff = tvb.tv_sec * 1000000.0;
        tvdiff += (double) tvb.tv_usec;
        tvdiff -= (tva.tv_sec * 1000000.0);
        tvdiff -= (double) tva.tv_usec;
        tvdiff /= 1000000;
        fprintf(stdout, "font.define(%s,%i): %f seconds\n",
                font_fullname(i), i, tvdiff);
#endif
        lua_pushinteger(L, i);
        return 1;
    } else {
        lua_pop(L, 1);          /* pop the broken table */
        delete_font(i);
        luaL_error(L, "font creation failed");
    }
    return 0;                   /* not reached */
}

/* this returns the expected (!) next fontid. */
static int nextfontid(lua_State * L)
{
    int i = get_fontid();
    lua_pushinteger(L, i);
    delete_font(i);
    return 1;
}


static int getfont(lua_State * L)
{
    int i;
    i = (int) luaL_checkinteger(L, -1);
    if (i && is_valid_font(i) && font_to_lua(L, i))
        return 1;
    lua_pushnil(L);
    return 1;
}


static int getfontid(lua_State * L)
{
    const char *s;
    size_t ff;
    int cs;
    int f;
    if (lua_type(L, 1) == LUA_TSTRING) {
        s = lua_tolstring(L, 1, &ff);
        cs = string_lookup(s, ff);
        if (cs == undefined_control_sequence || cs == undefined_cs_cmd
            || eq_type(cs) != set_font_cmd) {
            lua_pushstring(L, "not a valid font csname");
            f = -1;
        } else {
            f = equiv(cs);
        }
        lua_pushinteger(L, f);
    } else {
        luaL_error(L, "expected font csname string as argument");
    }
    return 1;
}


static const struct luaL_Reg fontlib[] = {
    {"read_tfm", font_read_tfm},
    {"read_vf", font_read_vf},
    {"current", tex_current_font},
    {"max", tex_max_font},
    {"each", tex_each_font},
    {"getfont", getfont},
    {"setfont", setfont},
    {"define", deffont},
    {"nextid", nextfontid},
    {"id", getfontid},
    {"frozen", frozenfont},
    {NULL, NULL}                /* sentinel */
};

int luaopen_font(lua_State * L)
{
    luaL_register(L, "font", fontlib);
    make_table(L, "fonts", "tex.fonts", "getfont", "setfont");
    return 1;
}

/**********************************************************************/
/* "vf" library: Lua functions within virtual fonts */

static int l_vf_char(lua_State * L)
{
    int k, w;
    /*int ex = 0;*/                 /* Wrong! TODO */
    vf_struct *vsp = static_pdf->vfstruct;
    packet_stack_record *mat_p;
    internal_font_number lf = vsp->lf;
    int ex_glyph = vsp->ex_glyph/1000;
    if (!vsp->vflua)
        normal_error("vf", "vf.char() outside virtual font");
    k = (int) luaL_checkinteger(L, 1);
    if (!char_exists(lf, (int) k)) {
        char_warning(lf, (int) k);
    } else {
        if (has_packet(lf, (int) k))
            do_vf_packet(static_pdf, lf, (int) k, ex_glyph);
        else
            backend_out[glyph_node] (static_pdf, lf, (int) k, ex_glyph);
    }
    mat_p = &(vsp->packet_stack[vsp->packet_stack_level]);
    w = char_width(lf, (int) k);
    mat_p->pos.h += round_xn_over_d(w, 1000 + ex_glyph, 1000);
    synch_pos_with_cur(static_pdf->posstruct, vsp->refpos, mat_p->pos);
    return 0;
}

static int l_vf_down(lua_State * L)
{
    scaled i;
    vf_struct *vsp = static_pdf->vfstruct;
    packet_stack_record *mat_p;
    if (!vsp->vflua)
        normal_error("vf", "vf.down() outside virtual font");
    i = (scaled) luaL_checkinteger(L, 1);
    i = store_scaled_f(i, vsp->fs_f);
    mat_p = &(vsp->packet_stack[vsp->packet_stack_level]);
    mat_p->pos.v += i;
    synch_pos_with_cur(static_pdf->posstruct, vsp->refpos, mat_p->pos);
    return 0;
}

static int l_vf_fontid(lua_State * L)
{
    vf_struct *vsp = static_pdf->vfstruct;
    if (!vsp->vflua)
        normal_error("vf", "vf.fontid() outside virtual font");
    vsp->lf = (int) luaL_checkinteger(L, 1);
    return 0;
}

static int l_vf_image(lua_State * L)
{
    int k;
    vf_struct *vsp = static_pdf->vfstruct;
    if (!vsp->vflua)
        normal_error("vf", "vf.image() outside virtual font");
    k = (int) luaL_checkinteger(L, 1);
    vf_out_image(static_pdf, k);
    return 0;
}

static int l_vf_node(lua_State * L)
{
    int k;
    vf_struct *vsp = static_pdf->vfstruct;
    if (!vsp->vflua)
        normal_error("vf", "vf.node() outside virtual font");
    k = (int) luaL_checkinteger(L, 1);
    hlist_out(static_pdf, (halfword) k, 0);
    return 0;
}

static int l_vf_nop(lua_State * L)
{
    vf_struct *vsp = static_pdf->vfstruct;
    if (!vsp->vflua)
        normal_error("vf", "vf.nop() outside virtual font");
    return 0;
}

static int l_vf_pop(lua_State * L)
{
    vf_struct *vsp = static_pdf->vfstruct;
    packet_stack_record *mat_p;
    if (!vsp->vflua)
        normal_error("vf", "vf.pop() outside virtual font");
    if (vsp->packet_stack_level == vsp->packet_stack_minlevel)
        normal_error("vf", "packet_stack_level underflow");
    vsp->packet_stack_level--;
    mat_p = &(vsp->packet_stack[vsp->packet_stack_level]);
    synch_pos_with_cur(static_pdf->posstruct, vsp->refpos, mat_p->pos);
    return 0;
}

static int l_vf_push(lua_State * L)
{
    vf_struct *vsp = static_pdf->vfstruct;
    packet_stack_record *mat_p;
    if (!vsp->vflua)
        normal_error("vf", "vf.push() outside virtual font");
    mat_p = &(vsp->packet_stack[vsp->packet_stack_level]);
    vsp->packet_stack_level++;
    if (vsp->packet_stack_level == packet_stack_size)
        normal_error("vf", "packet_stack_level overflow");
    vsp->packet_stack[vsp->packet_stack_level] = *mat_p;
    mat_p = &(vsp->packet_stack[vsp->packet_stack_level]);
    return 0;
}

static int l_vf_right(lua_State * L)
{
    scaled i;
    vf_struct *vsp = static_pdf->vfstruct;
    packet_stack_record *mat_p;
    if (!vsp->vflua)
        normal_error("vf", "vf.right() outside virtual font");
    mat_p = &(vsp->packet_stack[vsp->packet_stack_level]);
    i = (scaled) luaL_checkinteger(L, 1);
    i = store_scaled_f(i, vsp->fs_f);
    mat_p->pos.h += i;
    synch_pos_with_cur(static_pdf->posstruct, vsp->refpos, mat_p->pos);
    return 0;
}

static int l_vf_rule(lua_State * L)
{
    scaledpos size;
    vf_struct *vsp = static_pdf->vfstruct;
    packet_stack_record *mat_p;
    if (!vsp->vflua)
        normal_error("vf", "vf.rule() outside virtual font");
    size.h = (scaled) luaL_checkinteger(L, 1);
    size.v = (scaled) luaL_checkinteger(L, 2);
    size.h = store_scaled_f(size.h, vsp->fs_f);
    size.v = store_scaled_f(size.v, vsp->fs_f);
    if (size.h > 0 && size.v > 0)
        pdf_place_rule(static_pdf, 0, size, 0);    /* the 0 is unused */
    mat_p = &(vsp->packet_stack[vsp->packet_stack_level]);
    mat_p->pos.h += size.h;
    synch_pos_with_cur(static_pdf->posstruct, vsp->refpos, mat_p->pos);
    return 0;
}

static int l_vf_special(lua_State * L)
{
    const_lstring st;
    int texstr;
    vf_struct *vsp = static_pdf->vfstruct;
    if (!vsp->vflua)
        normal_error("vf", "vf.special() outside virtual font");
    st.s = lua_tolstring(L, 1, &(st.l));
    texstr = maketexlstring(st.s, st.l);
    pdf_literal(static_pdf, texstr, scan_special, false);
    flush_str(texstr);
    return 0;
}

static const struct luaL_Reg vflib[] = {
    {"char", l_vf_char},
    {"down", l_vf_down},
    /* {"font", l_vf_font}, */
    {"fontid", l_vf_fontid},
    {"image", l_vf_image},
    /* {"lua", l_vf_lua}, */
    {"node", l_vf_node},
    {"nop", l_vf_nop},
    {"pop", l_vf_pop},
    {"push", l_vf_push},
    {"right", l_vf_right},
    {"rule", l_vf_rule},
    /* {"scale", l_vf_scale}, */
    /* {"slot", l_vf_slot}, */
    {"special", l_vf_special},
    {NULL, NULL}                /* sentinel */
};

int luaopen_vf(lua_State * L)
{
    luaL_register(L, "vf", vflib);
    return 1;
}
