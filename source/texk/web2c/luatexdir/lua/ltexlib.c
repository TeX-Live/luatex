/* ltexlib.c
   
   Copyright 2006-2010 Taco Hoekwater <taco@luatex.org>

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

#include "lua/luatex-api.h"
#include "ptexlib.h"



static const char _svn_version[] =
    "$Id$ $URL$";

#define attribute(A) eqtb[attribute_base+(A)].hh.rh
#define dimen(A) eqtb[scaled_base+(A)].hh.rh
#undef skip
#define skip(A) eqtb[skip_base+(A)].hh.rh
#define mu_skip(A) eqtb[mu_skip_base+(A)].hh.rh
#define count(A) eqtb[count_base+(A)].hh.rh
#define box(A) equiv(box_base+(A))


typedef struct {
    char *text;
    unsigned int tsize;
    void *next;
    boolean partial;
    int cattable;
} rope;

typedef struct {
    rope *head;
    rope *tail;
    char complete;              /* currently still writing ? */
} spindle;

#define  PARTIAL_LINE       1
#define  FULL_LINE          0

#define  write_spindle spindles[spindle_index]
#define  read_spindle  spindles[(spindle_index-1)]

static int spindle_size = 0;
static spindle *spindles = NULL;
static int spindle_index = 0;

static void luac_store(lua_State * L, int i, int partial, int cattable)
{
    char *st;
    const char *sttemp;
    size_t tsize;
    rope *rn = NULL;
    sttemp = lua_tolstring(L, i, &tsize);
    st = xmalloc((unsigned)(tsize + 1));
    memcpy(st, sttemp, (tsize + 1));
    if (st) {
        luacstrings++;
        rn = (rope *) xmalloc(sizeof(rope));
        rn->text = st;
        rn->tsize = (unsigned)tsize;
        rn->partial = partial;
        rn->cattable = cattable;
        rn->next = NULL;
        if (write_spindle.head == NULL) {
            assert(write_spindle.tail == NULL);
            write_spindle.head = rn;
        } else {
            write_spindle.tail->next = rn;
        }
        write_spindle.tail = rn;
        write_spindle.complete = 0;
    }
}


static int do_luacprint(lua_State * L, int partial, int deftable)
{
    int i, n;
    int cattable = deftable;
    int startstrings = 1;
    n = lua_gettop(L);
    if (cattable != NO_CAT_TABLE) {
        if (lua_type(L, 1) == LUA_TNUMBER && n > 1) {
            lua_number2int(cattable, lua_tonumber(L, 1));
            startstrings = 2;
        }
    }
    if (lua_type(L, startstrings) == LUA_TTABLE) {
        for (i = 1;; i++) {
            lua_rawgeti(L, startstrings, i);
            if (lua_isstring(L, -1)) {
                luac_store(L, -1, partial, cattable);
                lua_pop(L, 1);
            } else {
                break;
            }
        }
    } else {
        for (i = startstrings; i <= n; i++) {
            if (!lua_isstring(L, i)) {
                lua_pushstring(L, "no string to print");
                lua_error(L);
            }
            luac_store(L, i, partial, cattable);
        }
    }
    return 0;
}

int luacwrite(lua_State * L)
{
    return do_luacprint(L, FULL_LINE, NO_CAT_TABLE);
}

int luacprint(lua_State * L)
{
    return do_luacprint(L, FULL_LINE, DEFAULT_CAT_TABLE);
}

int luacsprint(lua_State * L)
{
    return do_luacprint(L, PARTIAL_LINE, DEFAULT_CAT_TABLE);
}

int luacstring_cattable(void)
{
    return (int) read_spindle.tail->cattable;
}

int luacstring_partial(void)
{
    return read_spindle.tail->partial;
}

int luacstring_final_line(void)
{
    return (read_spindle.tail->next == NULL);
}

int luacstring_input(void)
{
    char *st;
    int ret;
    rope *t = read_spindle.head;
    if (!read_spindle.complete) {
        read_spindle.complete = 1;
        read_spindle.tail = NULL;
    }
    if (t == NULL) {
        if (read_spindle.tail != NULL)
            free(read_spindle.tail);
        read_spindle.tail = NULL;
        return 0;
    }
    if (t->text != NULL) {
        st = t->text;
        /* put that thing in the buffer */
        last = first;
        ret = last;
        check_buffer_overflow(last + (int) t->tsize);

        while (t->tsize-- > 0)
            buffer[last++] = (packed_ASCII_code) * st++;
        if (!t->partial) {
            while (last - 1 > ret && buffer[last - 1] == ' ')
                last--;
        }
        free(t->text);
        t->text = NULL;
    }
    if (read_spindle.tail != NULL) {    /* not a one-liner */
        free(read_spindle.tail);
    }
    read_spindle.tail = t;
    read_spindle.head = t->next;
    return 1;
}

/* open for reading, and make a new one for writing */
void luacstring_start(int n)
{
    (void) n;                   /* for -W */
    spindle_index++;
    if (spindle_size == spindle_index) {        /* add a new one */
        spindles =
            xrealloc(spindles, (unsigned)(sizeof(spindle) * (unsigned) (spindle_size + 1)));
        spindles[spindle_index].head = NULL;
        spindles[spindle_index].tail = NULL;
        spindles[spindle_index].complete = 0;
        spindle_size++;
    }
}

/* close for reading */

void luacstring_close(int n)
{
    rope *next, *t;
    (void) n;                   /* for -W */
    next = read_spindle.head;
    while (next != NULL) {
        if (next->text != NULL)
            free(next->text);
        t = next;
        next = next->next;
        free(t);
    }
    read_spindle.head = NULL;
    if (read_spindle.tail != NULL)
        free(read_spindle.tail);
    read_spindle.tail = NULL;
    read_spindle.complete = 0;
    spindle_index--;
}

/* local (static) versions */

#define check_index_range(j,s)						\
  if (j<0 || j > 65535) {							\
    lua_pushfstring(L, "incorrect index value %d for tex.%s()", (int)j, s); \
     lua_error(L);  }


int dimen_to_number(lua_State * L, const char *s)
{
    double v;
    char *d;
    int j;
    v = strtod(s, &d);
    if (strcmp(d, "in") == 0) {
        j = (int) (((v * 7227) / 100) * 65536);
    } else if (strcmp(d, "pc") == 0) {
        j = (int) ((v * 12) * 65536);
    } else if (strcmp(d, "cm") == 0) {
        j = (int) (((v * 7227) / 254) * 65536);
    } else if (strcmp(d, "mm") == 0) {
        j = (int) (((v * 7227) / 2540) * 65536);
    } else if (strcmp(d, "bp") == 0) {
        j = (int) (((v * 7227) / 7200) * 65536);
    } else if (strcmp(d, "dd") == 0) {
        j = (int) (((v * 1238) / 1157) * 65536);
    } else if (strcmp(d, "cc") == 0) {
        j = (int) (((v * 14856) / 1157) * 65536);
    } else if (strcmp(d, "nd") == 0) {
        j = (int) (((v * 21681) / 20320) * 65536);
    } else if (strcmp(d, "nc") == 0) {
        j = (int) (((v * 65043) / 5080) * 65536);
    } else if (strcmp(d, "pt") == 0) {
        j = (int) (v * 65536);
    } else if (strcmp(d, "sp") == 0) {
        j = (int) (v);
    } else {
        lua_pushstring(L, "unknown dimension specifier");
        lua_error(L);
        j = 0;
    }
    return j;
}


int get_item_index(lua_State * L, int i, int base)
{
    size_t kk;
    int k;
    int cur_cs;
    const char *s;
    if (lua_type(L, i) == LUA_TSTRING) {
        s = lua_tolstring(L, i, &kk);
        cur_cs = string_lookup(s, kk);
        if (cur_cs == undefined_control_sequence || cur_cs == undefined_cs_cmd) {
            k = -1;             /* guarandeed invalid */
        } else {
            k = (equiv(cur_cs) - base);
        }
    } else {
        k = (int) luaL_checkinteger(L, i);
    }
    return k;
}


static int vsetdimen(lua_State * L, int is_global)
{
    int i, j, err;
    int k;
    int save_global_defs = int_par(global_defs_code);
    if (is_global)
        int_par(global_defs_code) = 1;
    i = lua_gettop(L);
    j = 0;
    /* find the value */
    if (!lua_isnumber(L, i)) {
        if (lua_isstring(L, i)) {
            j = dimen_to_number(L, lua_tostring(L, i));
        } else {
            lua_pushstring(L, "unsupported value type");
            lua_error(L);
        }
    } else {
        lua_number2int(j, lua_tonumber(L, i));
    }
    k = get_item_index(L, (i - 1), scaled_base);
    check_index_range(k, "setdimen");
    err = set_tex_dimen_register(k, j);
    int_par(global_defs_code) = save_global_defs;
    if (err) {
        lua_pushstring(L, "incorrect value");
        lua_error(L);
    }
    return 0;
}

static int setdimen(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsetdimen(L, isglobal);
}

static int getdimen(lua_State * L)
{
    int j;
    int k;
    k = get_item_index(L, lua_gettop(L), scaled_base);
    check_index_range(k, "getdimen");
    j = get_tex_dimen_register(k);
    lua_pushnumber(L, j);
    return 1;
}

static int vsetskip(lua_State * L, int is_global)
{
    int i, err;
    halfword *j;
    int k;
    int save_global_defs = int_par(global_defs_code);
    if (is_global)
        int_par(global_defs_code) = 1;
    i = lua_gettop(L);
    j = check_isnode(L, i);     /* the value */
    k = get_item_index(L, (i - 1), skip_base);
    check_index_range(k, "setskip");    /* the index */
    err = set_tex_skip_register(k, *j);
    int_par(global_defs_code) = save_global_defs;
    if (err) {
        lua_pushstring(L, "incorrect value");
        lua_error(L);
    }
    return 0;
}

static int setskip(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsetskip(L, isglobal);
}

int getskip(lua_State * L)
{
    halfword j;
    int k;
    k = get_item_index(L, lua_gettop(L), skip_base);
    check_index_range(k, "getskip");
    j = get_tex_skip_register(k);
    lua_nodelib_push_fast(L, j);
    return 1;
}



static int vsetcount(lua_State * L, int is_global)
{
    int i, j, err;
    int k;
    int save_global_defs = int_par(global_defs_code);
    if (is_global)
        int_par(global_defs_code) = 1;
    i = lua_gettop(L);
    j = (int) luaL_checkinteger(L, i);
    k = get_item_index(L, (i - 1), count_base);
    check_index_range(k, "setcount");
    err = set_tex_count_register(k, j);
    int_par(global_defs_code) = save_global_defs;
    if (err) {
        lua_pushstring(L, "incorrect value");
        lua_error(L);
    }
    return 0;
}

static int setcount(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsetcount(L, isglobal);
}

static int getcount(lua_State * L)
{
    int j;
    int k;
    k = get_item_index(L, lua_gettop(L), count_base);
    check_index_range(k, "getcount");
    j = get_tex_count_register(k);
    lua_pushnumber(L, j);
    return 1;
}


static int vsetattribute(lua_State * L, int is_global)
{
    int i, j, err;
    int k;
    int save_global_defs = int_par(global_defs_code);
    if (is_global)
        int_par(global_defs_code) = 1;
    i = lua_gettop(L);
    j = (int) luaL_checkinteger(L, i);
    k = get_item_index(L, (i - 1), attribute_base);
    check_index_range(k, "setattribute");
    err = set_tex_attribute_register(k, j);
    int_par(global_defs_code) = save_global_defs;
    if (err) {
        lua_pushstring(L, "incorrect value");
        lua_error(L);
    }
    return 0;
}

static int setattribute(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsetattribute(L, isglobal);
}

static int getattribute(lua_State * L)
{
    int j;
    int k;
    k = get_item_index(L, lua_gettop(L), attribute_base);
    check_index_range(k, "getattribute");
    j = get_tex_attribute_register(k);
    lua_pushnumber(L, j);
    return 1;
}

int vsettoks(lua_State * L, int is_global)
{
    int i, err;
    int k;
    lstring str;
    int save_global_defs = int_par(global_defs_code);
    if (is_global)
        int_par(global_defs_code) = 1;
    i = lua_gettop(L);
    if (!lua_isstring(L, i)) {
        lua_pushstring(L, "unsupported value type");
        lua_error(L);
    }
    str.s = (unsigned char *) xstrdup(lua_tolstring(L, i, &str.l));
    k = get_item_index(L, (i - 1), toks_base);
    check_index_range(k, "settoks");
    err = set_tex_toks_register(k, str);
    xfree(str.s);
    int_par(global_defs_code) = save_global_defs;
    if (err) {
        lua_pushstring(L, "incorrect value");
        lua_error(L);
    }
    return 0;
}

static int settoks(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsettoks(L, isglobal);
}

static int gettoks(lua_State * L)
{
    int k;
    str_number t;
    char *ss;
    k = get_item_index(L, lua_gettop(L), toks_base);
    check_index_range(k, "gettoks");
    t = get_tex_toks_register(k);
    ss = makecstring(t);
    lua_pushstring(L, ss);
    free(ss);
    flush_str(t);
    return 1;
}

static int get_box_id(lua_State * L, int i)
{
    const char *s;
    int cur_cs, cur_cmd;
    size_t k = 0;
    int j = -1;
    if (lua_type(L, i) == LUA_TSTRING) {
        s = lua_tolstring(L, i, &k);
        cur_cs = string_lookup(s, k);
        cur_cmd = eq_type(cur_cs);
        if (cur_cmd == char_given_cmd ||
            cur_cmd == math_given_cmd || cur_cmd == omath_given_cmd) {
            j = equiv(cur_cs);
        }
    } else {
        lua_number2int(j, lua_tonumber(L, (i)));
    }
    return j;
}

static int getbox(lua_State * L)
{
    int k, t;
    k = get_box_id(L, -1);
    check_index_range(k, "getbox");
    t = get_tex_box_register(k);
    nodelist_to_lua(L, t);
    return 1;
}

static int vsetbox(lua_State * L, int is_global)
{
    int i, j, k, err;
    int save_global_defs = int_par(global_defs_code);
    if (is_global)
        int_par(global_defs_code) = 1;
    k = get_box_id(L, -2);
    check_index_range(k, "setbox");
    i = get_tex_box_register(k);
    if (lua_isboolean(L, -1)) {
        j = lua_toboolean(L, -1);
        if (j == 0)
            j = null;
        else
            return 0;
    } else {
        j = nodelist_from_lua(L);
        if (j != null && type(j) != hlist_node && type(j) != vlist_node) {
            lua_pushfstring(L, "setbox: incompatible node type (%s)\n",
                            get_node_name(type(j), subtype(j)));
            lua_error(L);
            return 0;
        }

    }
    err = set_tex_box_register(k, j);
    int_par(global_defs_code) = save_global_defs;
    if (err) {
        lua_pushstring(L, "incorrect value");
        lua_error(L);
    }
    return 0;
}

static int setbox(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsetbox(L, isglobal);
}

static int getboxdim(lua_State * L, int whichdim)
{
    int i, j;
    i = lua_gettop(L);
    j = get_box_id(L, i);
    lua_settop(L, (i - 2));     /* table at -1 */
    if (j < 0 || j > 65535) {
        lua_pushstring(L, "incorrect index");
        lua_error(L);
    }
    switch (whichdim) {
    case width_offset:
        lua_pushnumber(L, get_tex_box_width(j));
        break;
    case height_offset:
        lua_pushnumber(L, get_tex_box_height(j));
        break;
    case depth_offset:
        lua_pushnumber(L, get_tex_box_depth(j));
    }
    return 1;
}

int getboxwd(lua_State * L)
{
    return getboxdim(L, width_offset);
}

int getboxht(lua_State * L)
{
    return getboxdim(L, height_offset);
}

int getboxdp(lua_State * L)
{
    return getboxdim(L, depth_offset);
}

static int vsetboxdim(lua_State * L, int whichdim, int is_global)
{
    int i, j, k, err;
    int save_global_defs = int_par(global_defs_code);
    if (is_global)
        int_par(global_defs_code) = 1;
    i = lua_gettop(L);
    if (!lua_isnumber(L, i)) {
        j = dimen_to_number(L, lua_tostring(L, i));
    } else {
        lua_number2int(j, lua_tonumber(L, i));
    }
    k = get_box_id(L, (i - 1));
    lua_settop(L, (i - 3));     /* table at -2 */
    if (k < 0 || k > 65535) {
        lua_pushstring(L, "incorrect index");
        lua_error(L);
    }
    err = 0;
    switch (whichdim) {
    case width_offset:
        err = set_tex_box_width(k, j);
        break;
    case height_offset:
        err = set_tex_box_height(k, j);
        break;
    case depth_offset:
        err = set_tex_box_depth(k, j);
    }
    int_par(global_defs_code) = save_global_defs;
    if (err) {
        lua_pushstring(L, "not a box");
        lua_error(L);
    }
    return 0;
}

static int setboxwd(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsetboxdim(L, width_offset, isglobal);
}

static int setboxht(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsetboxdim(L, height_offset, isglobal);
}

static int setboxdp(lua_State * L)
{
    int isglobal = 0;
    int n = lua_gettop(L);
    if (n == 3 && lua_isstring(L, 1)) {
        const char *s = lua_tostring(L, 1);
        if (strcmp(s, "global") == 0)
            isglobal = 1;
    }
    return vsetboxdim(L, depth_offset, isglobal);
}

int settex(lua_State * L)
{
    const char *st;
    int i, j, texstr;
    size_t k;
    int cur_cs, cur_cmd;
    int isglobal = 0;
    j = 0;
    i = lua_gettop(L);
    if (lua_isstring(L, (i - 1))) {
        st = lua_tolstring(L, (i - 1), &k);
        texstr = maketexlstring(st, k);
        if (is_primitive(texstr)) {
            if (i == 3 && lua_isstring(L, 1)) {
                const char *s = lua_tostring(L, 1);
                if (strcmp(s, "global") == 0)
                    isglobal = 1;
            }
            cur_cs = string_lookup(st, k);
            flush_str(texstr);
            cur_cmd = eq_type(cur_cs);
            if (is_int_assign(cur_cmd)) {
                if (lua_isnumber(L, i)) {
                    int luai;
                    lua_number2int(luai, lua_tonumber(L, i));
                    assign_internal_value((isglobal ? 4 : 0),
                                          equiv(cur_cs), luai);
                } else {
                    lua_pushstring(L, "unsupported value type");
                    lua_error(L);
                }
            } else if (is_dim_assign(cur_cmd)) {
                if (!lua_isnumber(L, i)) {
                    if (lua_isstring(L, i)) {
                        j = dimen_to_number(L, lua_tostring(L, i));
                    } else {
                        lua_pushstring(L, "unsupported value type");
                        lua_error(L);
                    }
                } else {
                    lua_number2int(j, lua_tonumber(L, i));
                }
                assign_internal_value((isglobal ? 4 : 0), equiv(cur_cs), j);
            } else if (is_toks_assign(cur_cmd)) {
                if (lua_isstring(L, i)) {
                    j = tokenlist_from_lua(L);  /* uses stack -1 */
                    assign_internal_value((isglobal ? 4 : 0), equiv(cur_cs), j);

                } else {
                    lua_pushstring(L, "unsupported value type");
                    lua_error(L);
                }

            } else {
                lua_pushstring(L, "unsupported tex internal assignment");
                lua_error(L);
            }
        } else {
            if (lua_istable(L, (i - 2)))
                lua_rawset(L, (i - 2));
        }
    } else {
        if (lua_istable(L, (i - 2)))
            lua_rawset(L, (i - 2));
    }
    return 0;
}

int do_convert(lua_State * L, int cur_code)
{
    int texstr;
    int i = -1;
    char *str = NULL;
    switch (cur_code) {
    case pdf_creation_date_code:       /* ? */
    case pdf_insert_ht_code:   /* arg <register int> */
    case pdf_ximage_bbox_code: /* arg 2 ints */
    case lua_code:             /* arg complex */
    case lua_escape_string_code:       /* arg token list */
    case pdf_colorstack_init_code:     /* arg complex */
    case left_margin_kern_code:        /* arg box */
    case right_margin_kern_code:       /* arg box */
        break;
    case string_code:          /* arg token */
    case meaning_code:         /* arg token */
        break;

        /* the next fall through, and come from 'official' indices! */
    case font_name_code:       /* arg fontid */
    case font_identifier_code: /* arg fontid */
    case pdf_font_name_code:   /* arg fontid */
    case pdf_font_objnum_code: /* arg fontid */
    case pdf_font_size_code:   /* arg fontid */
    case uniform_deviate_code: /* arg int */
    case number_code:          /* arg int */
    case roman_numeral_code:   /* arg int */
    case pdf_page_ref_code:    /* arg int */
    case pdf_xform_name_code:  /* arg int */
        if (lua_gettop(L) < 1) {
            /* error */
        }
        lua_number2int(i, lua_tonumber(L, 1));  /* these fall through! */
    default:
        texstr = the_convert_string(cur_code, i);
        if (texstr) {
            str = makecstring(texstr);
            flush_str(texstr);
        }
    }
    if (str) {
        lua_pushstring(L, str);
        free(str);
    } else {
        lua_pushnil(L);
    }
    return 1;
}


int do_scan_internal(lua_State * L, int cur_cmd, int cur_code)
{
    int texstr;
    char *str = NULL;
    int save_cur_val, save_cur_val_level;
    save_cur_val = cur_val;
    save_cur_val_level = cur_val_level;
    scan_something_simple(cur_cmd, cur_code);

    if (cur_val_level == int_val_level ||
        cur_val_level == dimen_val_level || cur_val_level == attr_val_level) {
        lua_pushnumber(L, cur_val);
    } else if (cur_val_level == glue_val_level) {
        lua_nodelib_push_fast(L, cur_val);
    } else {                    /* dir_val_level, mu_val_level, tok_val_level */
        texstr = the_scanned_result();
        str = makecstring(texstr);
        if (str) {
            lua_pushstring(L, str);
            free(str);
        } else {
            lua_pushnil(L);
        }
        flush_str(texstr);
    }
    cur_val = save_cur_val;
    cur_val_level = save_cur_val_level;
    return 1;
}

int do_lastitem(lua_State * L, int cur_code)
{
    int retval = 1;
    switch (cur_code) {
        /* the next two do not actually exist */
    case lastattr_code:
    case attrexpr_code:
        lua_pushnil(L);
        break;
        /* the expressions do something complicated with arguments, yuck */
    case numexpr_code:
    case dimexpr_code:
    case glueexpr_code:
    case muexpr_code:
        lua_pushnil(L);
        break;
        /* these read a glue or muglue, todo */
    case mu_to_glue_code:
    case glue_to_mu_code:
    case glue_stretch_order_code:
    case glue_shrink_order_code:
    case glue_stretch_code:
    case glue_shrink_code:
        lua_pushnil(L);
        break;
        /* these read a fontid and a char, todo */
    case font_char_wd_code:
    case font_char_ht_code:
    case font_char_dp_code:
    case font_char_ic_code:
        lua_pushnil(L);
        break;
        /* these read an integer, todo */
    case par_shape_length_code:
    case par_shape_indent_code:
    case par_shape_dimen_code:
        lua_pushnil(L);
        break;
    case lastpenalty_code:
    case lastkern_code:
    case lastskip_code:
    case last_node_type_code:
    case input_line_no_code:
    case badness_code:
    case pdftex_version_code:
    case pdf_last_obj_code:
    case pdf_last_xform_code:
    case pdf_last_ximage_code:
    case pdf_last_ximage_pages_code:
    case pdf_last_annot_code:
    case pdf_last_x_pos_code:
    case pdf_last_y_pos_code:
    case pdf_retval_code:
    case pdf_last_ximage_colordepth_code:
    case random_seed_code:
    case pdf_last_link_code:
    case luatex_version_code:
    case Aleph_version_code:
    case Omega_version_code:
    case Aleph_minor_version_code:
    case Omega_minor_version_code:
    case eTeX_minor_version_code:
    case eTeX_version_code:
    case current_group_level_code:
    case current_group_type_code:
    case current_if_level_code:
    case current_if_type_code:
    case current_if_branch_code:
        retval = do_scan_internal(L, last_item_cmd, cur_code);
        break;
    default:
        lua_pushnil(L);
        break;
    }
    return retval;
}

static int tex_setmathparm(lua_State * L)
{
    int i, j;
    int k;
    int n;
    int l = cur_level;
    n = lua_gettop(L);

    if ((n == 3) || (n == 4)) {
        if (n == 4 && lua_isstring(L, 1)) {
            const char *s = lua_tostring(L, 1);
            if (strcmp(s, "global") == 0)
                l = 1;
        }
        i = luaL_checkoption(L, (n - 2), NULL, math_param_names);
        j = luaL_checkoption(L, (n - 1), NULL, math_style_names);
        lua_number2int(k, lua_tonumber(L, n));
        def_math_param(i, j, (scaled) k, l);
    }
    return 0;
}

static int tex_getmathparm(lua_State * L)
{
    int i, j;
    scaled k;
    if ((lua_gettop(L) == 2)) {
        i = luaL_checkoption(L, 1, NULL, math_param_names);
        j = luaL_checkoption(L, 2, NULL, math_style_names);
        k = get_math_param(i, j);
        lua_pushnumber(L, k);
    }
    return 1;
}

static int getfontname(lua_State * L)
{
    return do_convert(L, font_name_code);
}

static int getfontidentifier(lua_State * L)
{
    return do_convert(L, font_identifier_code);
}

static int getpdffontname(lua_State * L)
{
    return do_convert(L, pdf_font_name_code);
}

static int getpdffontobjnum(lua_State * L)
{
    return do_convert(L, pdf_font_objnum_code);
}

static int getpdffontsize(lua_State * L)
{
    return do_convert(L, pdf_font_size_code);
}

static int getuniformdeviate(lua_State * L)
{
    return do_convert(L, uniform_deviate_code);
}

static int getnumber(lua_State * L)
{
    return do_convert(L, number_code);
}

static int getromannumeral(lua_State * L)
{
    return do_convert(L, roman_numeral_code);
}

static int getpdfpageref(lua_State * L)
{
    return do_convert(L, pdf_page_ref_code);
}

static int getpdfxformname(lua_State * L)
{
    return do_convert(L, pdf_xform_name_code);
}


int get_parshape(lua_State * L)
{
    int n;
    halfword par_shape_ptr = equiv(par_shape_loc);
    if (par_shape_ptr != 0) {
        int m = 1;
        n = vinfo(par_shape_ptr + 1);
        lua_createtable(L, n, 0);
        while (m <= n) {
            lua_createtable(L, 2, 0);
            lua_pushnumber(L, vlink((par_shape_ptr) + (2 * (m - 1)) + 2));
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, vlink((par_shape_ptr) + (2 * (m - 1)) + 3));
            lua_rawseti(L, -2, 2);
            lua_rawseti(L, -2, m);
            m++;
        }
    } else {
        lua_pushnil(L);
    }
    return 1;
}


int gettex(lua_State * L)
{
    int cur_cs = -1;
    int retval = 1;             /* default is to return nil  */

    if (lua_isstring(L, 2)) {   /* 1 == 'tex' */
        int texstr;
        size_t k;
        const char *st = lua_tolstring(L, 2, &k);
        texstr = maketexlstring(st, k);
        cur_cs = prim_lookup(texstr);   /* not found == relax == 0 */
        flush_str(texstr);
    }
    if (cur_cs > 0) {
        int cur_cmd, cur_code;
        cur_cmd = get_prim_eq_type(cur_cs);
        cur_code = get_prim_equiv(cur_cs);
        switch (cur_cmd) {
        case last_item_cmd:
            retval = do_lastitem(L, cur_code);
            break;
        case convert_cmd:
            retval = do_convert(L, cur_code);
            break;
        case assign_toks_cmd:
        case assign_int_cmd:
        case assign_attr_cmd:
        case assign_dir_cmd:
        case assign_dimen_cmd:
        case assign_glue_cmd:
        case assign_mu_glue_cmd:
        case set_aux_cmd:
        case set_prev_graf_cmd:
        case set_page_int_cmd:
        case set_page_dimen_cmd:
        case char_given_cmd:
        case math_given_cmd:
        case omath_given_cmd:
            retval = do_scan_internal(L, cur_cmd, cur_code);
            break;
        case set_tex_shape_cmd:
            retval = get_parshape(L);
            break;
        default:
            lua_pushnil(L);
            break;
        }
    } else {
        lua_rawget(L, 1);       /* fetch other index from table */
    }
    return retval;
}


int getlist(lua_State * L)
{
    const char *str;
    if (lua_isstring(L, 2)) {
        str = lua_tostring(L, 2);
        if (strcmp(str, "page_ins_head") == 0) {
            if (vlink(page_ins_head) == page_ins_head)
                lua_pushnumber(L, null);
            else
                lua_pushnumber(L, vlink(page_ins_head));
            lua_nodelib_push(L);
        } else if (strcmp(str, "contrib_head") == 0) {
            lua_pushnumber(L, vlink(contrib_head));
            lua_nodelib_push(L);
        } else if (strcmp(str, "page_head") == 0) {
            lua_pushnumber(L, vlink(page_head));
            lua_nodelib_push(L);
        } else if (strcmp(str, "temp_head") == 0) {
            lua_pushnumber(L, vlink(temp_head));
            lua_nodelib_push(L);
        } else if (strcmp(str, "hold_head") == 0) {
            lua_pushnumber(L, vlink(hold_head));
            lua_nodelib_push(L);
        } else if (strcmp(str, "adjust_head") == 0) {
            lua_pushnumber(L, vlink(adjust_head));
            lua_nodelib_push(L);
        } else if (strcmp(str, "best_page_break") == 0) {
            lua_pushnumber(L, best_page_break);
            lua_nodelib_push(L);
        } else if (strcmp(str, "least_page_cost") == 0) {
            lua_pushnumber(L, least_page_cost);
        } else if (strcmp(str, "best_size") == 0) {
            lua_pushnumber(L, best_size);
        } else if (strcmp(str, "pre_adjust_head") == 0) {
            lua_pushnumber(L, vlink(pre_adjust_head));
            lua_nodelib_push(L);
        } else if (strcmp(str, "align_head") == 0) {
            lua_pushnumber(L, vlink(align_head));
            lua_nodelib_push(L);
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int setlist(lua_State * L)
{
    halfword *n_ptr;
    const char *str;
    halfword n = 0;
    if (lua_isstring(L, 2)) {
        str = lua_tostring(L, 2);
        if (strcmp(str, "best_size") == 0) {
            best_size = (int)lua_tointeger(L, 3);
        } else if (strcmp(str, "least_page_cost") == 0) {
            least_page_cost = (int)lua_tointeger(L, 3);
        } else {
            if (!lua_isnil(L, 3)) {
                n_ptr = check_isnode(L, 3);
                n = *n_ptr;
            }
            if (strcmp(str, "page_ins_head") == 0) {
                if (n == 0) {
                    vlink(page_ins_head) = page_ins_head;
                } else {
                    halfword m;
                    vlink(page_ins_head) = n;
                    m = tail_of_list(n);
                    vlink(m) = page_ins_head;
                }
            } else if (strcmp(str, "contrib_head") == 0) {
                vlink(contrib_head) = n;
                if (n == 0) {
                    if (nest_ptr == 0)
                        cur_list.tail_field = contrib_head;     /* vertical mode */
                    else
                        nest[0].tail_field = contrib_head;      /* other modes */
                }
            } else if (strcmp(str, "best_page_break") == 0) {
                best_page_break = n;
            } else if (strcmp(str, "page_head") == 0) {
                vlink(page_head) = n;
                page_tail = (n == 0 ? page_head : tail_of_list(n));
            } else if (strcmp(str, "temp_head") == 0) {
                vlink(temp_head) = n;
            } else if (strcmp(str, "hold_head") == 0) {
                vlink(hold_head) = n;
            } else if (strcmp(str, "adjust_head") == 0) {
                vlink(adjust_head) = n;
                adjust_tail = (n == 0 ? adjust_head : tail_of_list(n));
            } else if (strcmp(str, "pre_adjust_head") == 0) {
                vlink(pre_adjust_head) = n;
                pre_adjust_tail = (n == 0 ? pre_adjust_head : tail_of_list(n));
            } else if (strcmp(str, "align_head") == 0) {
                vlink(align_head) = n;
            }
        }
    }
    return 0;
}

static int do_integer_error(double m)
{
    const char *help[] =
        { "I can only go up to 2147483647='17777777777=" "7FFFFFFF,",
        "so I'm using that number instead of yours.",
        NULL
    };
    tex_error("Number too big", help);
    return (m > 0.0 ? infinity : -infinity);
}


static int tex_roundnumber(lua_State * L)
{
    double m = (double) lua_tonumber(L, 1) + 0.5;
    if (abs(m) > (double) infinity)
        lua_pushnumber(L, do_integer_error(m));
    else
        lua_pushnumber(L, floor(m));
    return 1;
}

static int tex_scaletable(lua_State * L)
{
    double delta = luaL_checknumber(L, 2);
    if (lua_istable(L, 1)) {
        lua_newtable(L);        /* the new table is at index 3 */
        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {   /* numeric value */
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            if (lua_isnumber(L, -1)) {
                double m = (double) lua_tonumber(L, -1) * delta + 0.5;
                lua_pop(L, 1);
                if (abs(m) > (double) infinity)
                    lua_pushnumber(L, do_integer_error(m));
                else
                    lua_pushnumber(L, floor(m));
            }
            lua_rawset(L, 3);
        }
    } else if (lua_isnumber(L, 1)) {
        double m = (double) lua_tonumber(L, 1) * delta + 0.5;
        if (abs(m) > (double) infinity)
            lua_pushnumber(L, do_integer_error(m));
        else
            lua_pushnumber(L, floor(m));
    } else {
        lua_pushnil(L);
    }
    return 1;
}

#define hash_text(A) hash[(A)].rh

static int tex_definefont(lua_State * L)
{
    const char *csname;
    int f, u;
    str_number t;
    size_t l;
    int i = 1;
    int a = 0;
    if (!no_new_control_sequence) {
        const char *help[] =
            { "You can't create a new font inside a \\csname\\endcsname pair",
            NULL
        };
        tex_error("Definition active", help);
    }
    if ((lua_gettop(L) == 3) && lua_isboolean(L, 1)) {
        a = lua_toboolean(L, 1);
        i = 2;
    }
    csname = luaL_checklstring(L, i, &l);
    f = (int)luaL_checkinteger(L, (i + 1));
    t = maketexlstring(csname, l);
    no_new_control_sequence = 0;
    u = string_lookup(csname, l);
    no_new_control_sequence = 1;
    if (a)
        geq_define(u, set_font_cmd, f);
    else
        eq_define(u, set_font_cmd, f);
    eqtb[font_id_base + f] = eqtb[u];
    hash_text(font_id_base + f) = t;
    return 0;
}

static int tex_hashpairs(lua_State * L)
{
    int cmd, chr;
    str_number s = 0;
    int cs = 1;
    lua_newtable(L);
    while (cs < eqtb_size) {
        s = hash_text(cs);
        if (s > 0) {
            char *ss = makecstring(s);
            lua_pushstring(L, ss);
            free(ss);
            cmd = eq_type(cs);
            chr = equiv(cs);
            make_token_table(L, cmd, chr, cs);
            lua_rawset(L, -3);
        }
        cs++;
    }
    return 1;
}

static int tex_primitives(lua_State * L)
{
    int cmd, chr;
    str_number s = 0;
    int cs = 0;
    lua_newtable(L);
    while (cs < prim_size) {
        s = get_prim_text(cs);
        if (s > 0) {
            char *ss = makecstring(s);
            lua_pushstring(L, ss);
            free(ss);
            cmd = get_prim_eq_type(cs);
            chr = get_prim_equiv(cs);
            make_token_table(L, cmd, chr, 0);
            lua_rawset(L, -3);
        }
        cs++;
    }
    return 1;
}

static int tex_extraprimitives(lua_State * L)
{
    int n, i;
    int mask = 0;
    str_number s = 0;
    int cs = 0;
    n = lua_gettop(L);
    if (n == 0) {
        mask = etex_command + aleph_command + omega_command +
            pdftex_command + luatex_command;
    } else {
        for (i = 1; i <= n; i++) {
            if (lua_isstring(L, i)) {
                const char *s = lua_tostring(L, i);
                if (strcmp(s, "etex") == 0) {
                    mask |= etex_command;
                } else if (strcmp(s, "tex") == 0) {
                    mask |= tex_command;
                } else if (strcmp(s, "core") == 0) {
                    mask |= core_command;
                } else if (strcmp(s, "pdftex") == 0) {
                    mask |= pdftex_command;
                } else if (strcmp(s, "aleph") == 0) {
                    mask |= aleph_command;
                } else if (strcmp(s, "omega") == 0) {
                    mask |= omega_command;
                } else if (strcmp(s, "luatex") == 0) {
                    mask |= luatex_command;
                }
            }
        }
    }
    lua_newtable(L);
    i = 1;
    while (cs < prim_size) {
        s = get_prim_text(cs);
        if (s > 0) {
            if (get_prim_origin(cs) & mask) {
                char *ss = makecstring(s);
                lua_pushstring(L, ss);
                free(ss);
                lua_rawseti(L, -2, i++);
            }
        }
        cs++;
    }
    return 1;
}

static int tex_enableprimitives(lua_State * L)
{
    int n = lua_gettop(L);
    if (n != 2) {
        lua_pushstring(L, "wrong number of arguments");
        lua_error(L);
    } else {
        size_t l;
        int i;
        const char *pre = luaL_checklstring(L, 1, &l);
        if (lua_istable(L, 2)) {
            int nncs = no_new_control_sequence;
            no_new_control_sequence = true;
            i = 1;
            while (1) {
                lua_rawgeti(L, 2, i);
                if (lua_isstring(L, 3)) {
                    const char *prim = lua_tostring(L, 3);
                    str_number s = maketexstring(prim);
                    halfword prim_val = prim_lookup(s);
                    if (prim_val != undefined_primitive) {
                        char *newprim;
                        int val;
                        size_t newl;
                        halfword cur_cmd = get_prim_eq_type(prim_val);
                        halfword cur_chr = get_prim_equiv(prim_val);
                        if (strncmp(pre, prim, l) != 0) {       /* not a prefix */
                            newl = strlen(prim) + l;
                            newprim = (char *) xmalloc((unsigned)(newl + 1));
                            strcpy(newprim, pre);
                            strcat(newprim + l, prim);
                        } else {
                            newl = strlen(prim);
                            newprim = (char *) xmalloc((unsigned)(newl + 1));
                            strcpy(newprim, prim);
                        }
                        val = string_lookup(newprim, newl);
                        if (val == undefined_control_sequence ||
                            eq_type(val) == undefined_cs_cmd) {
                            primitive_def(newprim, newl, (quarterword) cur_cmd,
                                          cur_chr);
                        }
                        free(newprim);
                    }
                    flush_str(s);
                } else {
                    lua_pop(L, 1);
                    break;
                }
                lua_pop(L, 1);
                i++;
            }
            lua_pop(L, 1);      /* the table */
            no_new_control_sequence = nncs;
        } else {
            lua_pushstring(L, "Expected an array of names as second argument");
            lua_error(L);
        }
    }
    return 0;
}

static int tex_run_boot(lua_State * L)
{
    int n = lua_gettop(L);
    const char *format = NULL;
    if (n >= 1) {
        ini_version = 0;
        format = luaL_checkstring(L, 1);
    } else {
        ini_version = 1;
    }
    if (main_initialize()) {    /* > 0 = failure */
        lua_pushboolean(L, 0);  /* false */
        return 1;
    }
    if (format) {
        if (!zopen_w_input(&fmt_file, format, DUMP_FORMAT, FOPEN_RBIN_MODE)) {
            lua_pushboolean(L, 0);      /* false */
            return 1;
        }
        if (!load_fmt_file(format)) {
            zwclose(fmt_file);
            lua_pushboolean(L, 0);      /* false */
            return 1;
        }
        zwclose(fmt_file);
    }
    fix_date_and_time();
    if (format == NULL)
        make_pdftex_banner();
    random_seed = (microseconds * 1000) + (epochseconds % 1000000);
    init_randoms(random_seed);
    initialize_math();
    fixup_selector(log_opened);
    check_texconfig_init();
    text_dir_ptr = new_dir(0);
    history = spotless;         /* ready to go! */
    /* Initialize synctex primitive */
    synctexinitcommand();
    /* tex is ready to go, now */
    unhide_lua_table(Luas, "tex", tex_table_id);
    unhide_lua_table(Luas, "pdf", pdf_table_id);
    unhide_lua_table(Luas, "token", token_table_id);
    unhide_lua_table(Luas, "node", node_table_id);

    lua_pushboolean(L, 1);      /* true */
    return 1;

}

static int tex_run_main(lua_State * L)
{
    (void) L;
    main_control();
    return 0;
}

static int tex_run_end(lua_State * L)
{
    (void) L;
    final_cleanup();            /* prepare for death */
    close_files_and_terminate();
    do_final_end();
    return 0;
}

void init_tex_table(lua_State * L)
{
    lua_createtable(L, 0, 3);
    lua_pushcfunction(L, tex_run_boot);
    lua_setfield(L, -2, "initialize");
    lua_pushcfunction(L, tex_run_main);
    lua_setfield(L, -2, "run");
    lua_pushcfunction(L, tex_run_end);
    lua_setfield(L, -2, "finish");
    lua_setglobal(L, "tex");
}




static const struct luaL_reg texlib[] = {
    {"run", tex_run_main},      /* may be needed  */
    {"finish", tex_run_end},    /* may be needed  */
    {"write", luacwrite},
    {"write", luacwrite},
    {"print", luacprint},
    {"sprint", luacsprint},
    {"set", settex},
    {"get", gettex},
    {"setdimen", setdimen},
    {"getdimen", getdimen},
    {"setskip", setskip},
    {"getskip", getskip},
    {"setattribute", setattribute},
    {"getattribute", getattribute},
    {"setcount", setcount},
    {"getcount", getcount},
    {"settoks", settoks},
    {"gettoks", gettoks},
    {"setbox", setbox},
    {"getbox", getbox},
    {"setlist", setlist},
    {"getlist", getlist},
    {"setboxwd", setboxwd},
    {"getboxwd", getboxwd},
    {"setboxht", setboxht},
    {"getboxht", getboxht},
    {"setboxdp", setboxdp},
    {"getboxdp", getboxdp},
    {"round", tex_roundnumber},
    {"scale", tex_scaletable},
    {"fontname", getfontname},
    {"fontidentifier", getfontidentifier},
    {"pdffontname", getpdffontname},
    {"pdffontobjnum", getpdffontobjnum},
    {"pdffontsize", getpdffontsize},
    {"uniformdeviate", getuniformdeviate},
    {"number", getnumber},
    {"romannumeral", getromannumeral},
    {"pdfpageref", getpdfpageref},
    {"pdfxformname", getpdfxformname},
    {"definefont", tex_definefont},
    {"hashtokens", tex_hashpairs},
    {"primitives", tex_primitives},
    {"extraprimitives", tex_extraprimitives},
    {"enableprimitives", tex_enableprimitives},
    {"setmath", tex_setmathparm},
    {"getmath", tex_getmathparm},
    {NULL, NULL}                /* sentinel */
};

int luaopen_tex(lua_State * L)
{
    luaL_register(L, "tex", texlib);
    /* *INDENT-OFF* */
    make_table(L, "attribute",  "getattribute", "setattribute");
    make_table(L, "skip",       "getskip",      "setskip");
    make_table(L, "dimen",      "getdimen",     "setdimen");
    make_table(L, "count",      "getcount",     "setcount");
    make_table(L, "toks",       "gettoks",      "settoks");
    make_table(L, "box",        "getbox",       "setbox");
    make_table(L, "wd",         "getboxwd",     "setboxwd");
    make_table(L, "ht",         "getboxht",     "setboxht");
    make_table(L, "dp",         "getboxdp",     "setboxdp");
    make_table(L, "lists",      "getlist",      "setlist");
    /* *INDENT-ON* */
    /* make the meta entries */
    /* fetch it back */
    luaL_newmetatable(L, "tex_meta");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, gettex);
    lua_settable(L, -3);
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, settex);
    lua_settable(L, -3);
    lua_setmetatable(L, -2);    /* meta to itself */
    /* initialize the I/O stack: */
    spindles = xmalloc(sizeof(spindle));
    spindle_index = 0;
    spindles[0].head = NULL;
    spindles[0].tail = NULL;
    spindle_size = 1;
    /* a somewhat odd place for this assert, maybe */
    assert(command_names[data_cmd].command_offset == data_cmd);
    return 1;
}
