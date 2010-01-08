/* lpdflib.c

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

static const char _svn_version[] =
    "$Id$ "
    "$URL$";

#include "lua/luatex-api.h"
#include <ptexlib.h>

#define buf_to_pdfbuf_macro(p, s, l)              \
for (i = 0; i < (l); i++) {                       \
    if (i % 16 == 0)                              \
        pdf_room(p, 16);                          \
    pdf_quick_out(p, ((unsigned char *) (s))[i]); \
}

int luapdfprint(lua_State * L)
{
    int n;
    unsigned i;
    lstring st, modestr;
    ctm_transform_modes literal_mode;
    st.s = modestr.s = NULL;
    n = lua_gettop(L);
    if (!lua_isstring(L, -1)) {
        lua_pushstring(L, "no string to print");
        lua_error(L);
    }
    literal_mode = set_origin;
    if (n == 2) {
        if (!lua_isstring(L, -2)) {
            lua_pushstring(L, "invalid argument for print literal mode");
            lua_error(L);
        } else {
            modestr.s = (unsigned char *) lua_tolstring(L, -2, &modestr.l);
            if (modestr.l == 6
                && strncmp((const char *) modestr.s, "direct", 6) == 0)
                literal_mode = direct_always;
            else if (modestr.l == 4
                     && strncmp((const char *) modestr.s, "page", 4) == 0)
                literal_mode = direct_page;
            else {
                lua_pushstring(L, "invalid argument for print literal mode");
                lua_error(L);
            }
        }
    } else {
        if (n != 1) {
            lua_pushstring(L, "invalid number of arguments");
            lua_error(L);
        }
    }
    check_o_mode(static_pdf, "pdf.print()", 1 << OMODE_PDF, true);
    switch (literal_mode) {
    case (set_origin):
        pdf_goto_pagemode(static_pdf);
        pdf_set_pos(static_pdf, static_pdf->posstruct->pos);
        (void) calc_pdfpos(static_pdf->pstruct, static_pdf->posstruct->pos);
        break;
    case (direct_page):
        pdf_goto_pagemode(static_pdf);
        (void) calc_pdfpos(static_pdf->pstruct, static_pdf->posstruct->pos);
        break;
    case (direct_always):
        pdf_end_string_nl(static_pdf);
        break;
    default:
        assert(0);
    }
    st.s = (unsigned char *) lua_tolstring(L, n, &st.l);
    buf_to_pdfbuf_macro(static_pdf, st.s, st.l);
    return 0;
}

unsigned char *fread_to_buf(lua_State * L, char *filename, size_t * len)
{
    int i = 0;
    FILE *f;
    unsigned char *buf = NULL;
    if ((f = fopen(filename, "rb")) == NULL)
        luaL_error(L, "pdf.immediateobj() cannot open input file");
    if ((i = readbinfile(f, &buf, (int *) len)) == 0)
        luaL_error(L, "pdf.immediateobj() cannot read input file");
    fclose(f);
    return buf;
}

static int l_immediateobj(lua_State * L)
{
    int n, first_arg = 1;
    unsigned i;
    int k;
    lstring buf, st1, st2, st3;
    buf.s = st1.s = st2.s = st3.s = NULL;
    check_o_mode(static_pdf, "immediateobj()", 1 << OMODE_PDF, true);
    n = lua_gettop(L);
    if (n > 0 && lua_type(L, 1) == LUA_TNUMBER) {
        first_arg++;
        lua_number2int(k, lua_tonumber(L, 1));
        check_obj_exists(static_pdf, obj_type_obj, k);
        if (is_obj_scheduled(static_pdf, k) || obj_data_ptr(static_pdf, k) != 0)
            luaL_error(L, "pdf.immediateobj() object in use");
    } else {
        incr(static_pdf->obj_count);
        pdf_create_obj(static_pdf, obj_type_obj, static_pdf->sys_obj_ptr + 1);
        k = static_pdf->sys_obj_ptr;
    }
    pdf_last_obj = k;
    switch (n - first_arg + 1) {
    case 0:
        luaL_error(L, "pdf.immediateobj() needs at least one argument");
        break;
    case 1:
        if (!lua_isstring(L, first_arg))
            luaL_error(L, "pdf.immediateobj() 1st argument must be string");
        pdf_begin_obj(static_pdf, k, 1);
        st1.s = (unsigned char *) lua_tolstring(L, first_arg, &st1.l);
        buf_to_pdfbuf_macro(static_pdf, st1.s, st1.l);
        if (st1.s[st1.l - 1] != '\n')
            pdf_puts(static_pdf, "\n");
        pdf_end_obj(static_pdf);
        break;
    case 2:
    case 3:
        if (!lua_isstring(L, first_arg))
            luaL_error(L, "pdf.immediateobj() 1st argument must be string");
        if (!lua_isstring(L, first_arg + 1))
            luaL_error(L, "pdf.immediateobj() 2nd argument must be string");
        st1.s = (unsigned char *) lua_tolstring(L, first_arg, &st1.l);
        st2.s = (unsigned char *) lua_tolstring(L, first_arg + 1, &st2.l);
        if (st1.l == 4 && strncmp((const char *) st1.s, "file", 4) == 0) {
            if (n == first_arg + 2)
                luaL_error(L,
                           "pdf.immediateobj() 3rd argument forbidden in file mode");
            pdf_begin_obj(static_pdf, k, 1);
            buf.s = fread_to_buf(L, (char *) st2.s, &buf.l);
            buf_to_pdfbuf_macro(static_pdf, buf.s, buf.l);
            if (buf.s[buf.l - 1] != '\n')
                pdf_puts(static_pdf, "\n");
            xfree(buf.s);
            pdf_end_obj(static_pdf);
        } else {
            pdf_begin_dict(static_pdf, k, 0);   /* 0 = not an object stream candidate! */
            if (n == first_arg + 2) {   /* write attr text */
                if (!lua_isstring(L, first_arg + 2))
                    luaL_error(L,
                               "pdf.immediateobj() 3rd argument must be string");
                st3.s =
                    (unsigned char *) lua_tolstring(L, first_arg + 2, &st3.l);
                buf_to_pdfbuf_macro(static_pdf, st3.s, st3.l);
                if (st3.s[st3.l - 1] != '\n')
                    pdf_puts(static_pdf, "\n");
            }
            pdf_begin_stream(static_pdf);
            if (st1.l == 6 && strncmp((const char *) st1.s, "stream", 6) == 0) {
                buf_to_pdfbuf_macro(static_pdf, st2.s, st2.l);
            } else if (st1.l == 10
                       && strncmp((const char *) st1.s, "streamfile",
                                  10) == 0) {
                buf.s = fread_to_buf(L, (char *) st2.s, &buf.l);
                buf_to_pdfbuf_macro(static_pdf, buf.s, buf.l);
                xfree(buf.s);
            } else
                luaL_error(L, "pdf.immediateobj() invalid argument");
            pdf_end_stream(static_pdf);
        }
        break;
    default:
        luaL_error(L, "pdf.immediateobj() allows max. 3 arguments");
    }
    lua_pushinteger(L, k);
    return 1;
}

/**********************************************************************/
/* for LUA_ENVIRONINDEX table lookup (instead of repeated strcmp()) */

typedef enum { P__ZERO, P_RAW, P_STREAM, P__SENTINEL } parm_idx;

typedef struct {
    const char *name;           /* parameter name */
    parm_idx idx;               /* index within img_parms array */
} parm_struct;

const parm_struct obj_parms[] = {
    {NULL, P__ZERO},            /* dummy; lua indices run from 1 */
    {"raw", P_RAW},
    {"stream", P_STREAM},
    {NULL, P__SENTINEL}
};

/**********************************************************************/

static int table_obj(lua_State * L)
{
    int k, type;
    int compress_level = -1;    /* unset */
    int os_level = 1;           /* default: put non-stream objects into object streams */
    int saved_compress_level = static_pdf->compress_level;
    lstring attr, st, buf;
    size_t i;
    int immediate = 0;          /* default: not immediate */
    attr.s = st.s = buf.s = NULL;
    assert(lua_istable(L, 1));  /* t */

    /* get object "type" */

    lua_pushstring(L, "type");  /* ks t */
    lua_gettable(L, -2);        /* vs? t */
    if (lua_isnil(L, -1))       /* !vs t */
        luaL_error(L, "pdf.obj(): object \"type\" missing");
    if (!lua_isstring(L, -1))   /* !vs t */
        luaL_error(L, "pdf.obj(): object \"type\" must be string");
    lua_pushvalue(L, -1);       /* vs vs t */
    lua_gettable(L, LUA_ENVIRONINDEX);  /* i? vs t */
    if (!lua_isnumber(L, -1))   /* !i vs t */
        luaL_error(L, "pdf.obj(): \"%s\" is not a valid object type",
                   lua_tostring(L, -2));
    type = lua_tointeger(L, -1);        /* i vs t */
    lua_pop(L, 1);              /* vs t */
    switch (type) {
    case P_RAW:
    case P_STREAM:
        break;
    default:
        assert(0);
    }
    lua_pop(L, 1);              /* t */

    /* get optional "immediate" */

    lua_pushstring(L, "immediate");     /* ks t */
    lua_gettable(L, -2);        /* b? t */
    if (!lua_isnil(L, -1)) {    /* b? t */
        if (!lua_isboolean(L, -1))      /* !b t */
            luaL_error(L, "pdf.obj(): \"immediate\" must be boolean");
        immediate = lua_toboolean(L, -1);       /* 0 or 1 */
    }
    lua_pop(L, 1);              /* t */

    /* is a reserved object referenced by "objnum"? */

    lua_pushstring(L, "objnum");        /* ks t */
    lua_gettable(L, -2);        /* vi? t */
    if (!lua_isnil(L, -1)) {    /* vi? t */
        if (!lua_isnumber(L, -1))       /* !vi t */
            luaL_error(L, "pdf.obj(): \"objnum\" must be integer");
        k = lua_tointeger(L, -1);       /* vi t */
        check_obj_exists(static_pdf, obj_type_obj, k);
        if (is_obj_scheduled(static_pdf, k) || obj_data_ptr(static_pdf, k) != 0)
            luaL_error(L, "pdf.obj() object in use");
    } else {
        incr(static_pdf->obj_count);
        pdf_create_obj(static_pdf, obj_type_obj, static_pdf->sys_obj_ptr + 1);
        k = static_pdf->sys_obj_ptr;
    }
    pdf_last_obj = k;
    if (immediate == 0) {
        obj_data_ptr(static_pdf, k) = pdf_get_mem(static_pdf, pdfmem_obj_size);
        init_obj_obj(static_pdf, k);
    }
    lua_pop(L, 1);              /* t */

    /* get optional "attr" (allowed only for stream case) */

    lua_pushstring(L, "attr");  /* ks t */
    lua_gettable(L, -2);        /* attr-s? t */
    if (!lua_isnil(L, -1)) {    /* attr-s? t */
        if (type != P_STREAM)
            luaL_error(L,
                       "pdf.obj(): \"attr\" key not allowed for non-stream object");
        if (!lua_isstring(L, -1))       /* !attr-s t */
            luaL_error(L, "pdf.obj(): object \"attr\" must be string");
        if (immediate == 1) {
            attr.s = (unsigned char *) lua_tolstring(L, -1, &attr.l);   /* attr-s t */
            lua_pop(L, 1);      /* t */
        } else
            obj_obj_stream_attr(static_pdf, k) = luaL_ref(Luas, LUA_REGISTRYINDEX);     /* t */
    } else
        lua_pop(L, 1);          /* t */

    /* get optional "compresslevel" (allowed only for stream case) */

    lua_pushstring(L, "compresslevel"); /* ks t */
    lua_gettable(L, -2);        /* vi? t */
    if (!lua_isnil(L, -1)) {    /* vi? t */
        if (type == P_RAW)
            luaL_error(L,
                       "pdf.obj(): \"compresslevel\" key not allowed for raw object");
        if (!lua_isnumber(L, -1))       /* !vi t */
            luaL_error(L, "pdf.obj(): \"compresslevel\" must be integer");
        compress_level = lua_tointeger(L, -1);  /* vi t */
        if (compress_level > 9)
            luaL_error(L, "pdf.obj(): \"compresslevel\" must be <= 9");
        else if (compress_level < 0)
            luaL_error(L, "pdf.obj(): \"compresslevel\" must be >= 0");
        if (immediate == 0)
            obj_obj_pdfcompresslevel(static_pdf, k) = compress_level;
    }
    lua_pop(L, 1);              /* t */

    /* get optional "objcompression" (allowed only for non-stream case) */

    lua_pushstring(L, "objcompression");        /* ks t */
    lua_gettable(L, -2);        /* b? t */
    if (!lua_isnil(L, -1)) {    /* b? t */
        if (type == P_STREAM)
            luaL_error(L,
                       "pdf.obj(): \"objcompression\" key not allowed for stream object");
        if (!lua_isboolean(L, -1))      /* !b t */
            luaL_error(L, "pdf.obj(): \"objcompression\" must be boolean");
        os_level = lua_toboolean(L, -1);        /* 0 or 1 */
        /* 0: never compress; 1: depends then on \pdfobjcompresslevel */
        if (immediate == 0)
            obj_obj_pdfoslevel(static_pdf, k) = os_level;
    }
    lua_pop(L, 1);              /* t */

    /* now the object contents for all cases are handled */

    lua_pushstring(L, "string");        /* ks t */
    lua_gettable(L, -2);        /* string-s? t */
    lua_pushstring(L, "file");  /* ks string-s? t */
    lua_gettable(L, -3);        /* file-s? string-s? t */
    if (!lua_isnil(L, -1) && !lua_isnil(L, -2)) /* file-s? string-s? t */
        luaL_error(L,
                   "pdf.obj(): \"string\" and \"file\" must not be given together");
    if (lua_isnil(L, -1) && lua_isnil(L, -2))   /* nil nil t */
        luaL_error(L, "pdf.obj(): no \"string\" or \"file\" given");

    switch (type) {
    case P_RAW:
        if (immediate == 1)
            pdf_begin_obj(static_pdf, k, os_level);
        if (!lua_isnil(L, -2)) {        /* file-s? string-s? t */
            /* from string */
            lua_pop(L, 1);      /* string-s? t */
            if (!lua_isstring(L, -1))   /* !string-s t */
                luaL_error(L,
                           "pdf.obj(): \"string\" must be string for raw object");
            if (immediate == 1) {
                st.s = (unsigned char *) lua_tolstring(L, -1, &st.l);
                buf_to_pdfbuf_macro(static_pdf, st.s, st.l);
                if (st.s[st.l - 1] != '\n')
                    pdf_puts(static_pdf, "\n");
            } else
                obj_obj_data(static_pdf, k) = luaL_ref(L, LUA_REGISTRYINDEX);   /* t */
        } else {
            /* from file */
            if (!lua_isstring(L, -1))   /* !file-s nil t */
                luaL_error(L,
                           "pdf.obj(): \"file\" name must be string for raw object");
            if (immediate == 1) {
                st.s = (unsigned char *) lua_tolstring(L, -1, &st.l);   /* file-s nil t */
                buf.s = fread_to_buf(L, (char *) st.s, &buf.l);
                buf_to_pdfbuf_macro(static_pdf, buf.s, buf.l);
                if (buf.s[buf.l - 1] != '\n')
                    pdf_puts(static_pdf, "\n");
                xfree(buf.s);
            } else {
                set_obj_obj_is_file(static_pdf, k);
                obj_obj_data(static_pdf, k) = luaL_ref(L, LUA_REGISTRYINDEX);   /* nil t */
            }
        }
        if (immediate == 1)
            pdf_end_obj(static_pdf);
        break;
    case P_STREAM:
        if (immediate == 1) {
            pdf_begin_dict(static_pdf, k, 0);   /* 0 = not an object stream candidate! */
            if (attr.s != NULL) {
                buf_to_pdfbuf_macro(static_pdf, attr.s, attr.l);
                if (attr.s[attr.l - 1] != '\n')
                    pdf_puts(static_pdf, "\n");
            }
            if (compress_level > -1)
                static_pdf->compress_level = compress_level;
            pdf_begin_stream(static_pdf);
        } else {
            set_obj_obj_is_stream(static_pdf, k);
            if (compress_level > -1)
                obj_obj_pdfcompresslevel(static_pdf, k) = compress_level;
        }
        if (!lua_isnil(L, -2)) {        /* file-s? string-s? t */
            /* from string */
            lua_pop(L, 1);      /* string-s? t */
            if (!lua_isstring(L, -1))   /* !string-s t */
                luaL_error(L,
                           "pdf.obj(): \"string\" must be string for stream object");
            if (immediate == 1) {
                st.s = (unsigned char *) lua_tolstring(L, -1, &st.l);   /* string-s t */
                buf_to_pdfbuf_macro(static_pdf, st.s, st.l);
            } else
                obj_obj_data(static_pdf, k) = luaL_ref(L, LUA_REGISTRYINDEX);   /* t */
        } else {
            /* from file */
            if (!lua_isstring(L, -1))   /* !file-s nil t */
                luaL_error(L,
                           "pdf.obj(): \"file\" name must be string for stream object");
            if (immediate == 1) {
                st.s = (unsigned char *) lua_tolstring(L, -1, &st.l);   /* file-s nil t */
                buf.s = fread_to_buf(L, (char *) st.s, &buf.l);
                buf_to_pdfbuf_macro(static_pdf, buf.s, buf.l);
                xfree(buf.s);
            } else {
                set_obj_obj_is_file(static_pdf, k);
                obj_obj_data(static_pdf, k) = luaL_ref(L, LUA_REGISTRYINDEX);   /* nil t */
            }
        }
        if (immediate == 1)
            pdf_end_stream(static_pdf);
        break;
    default:
        assert(0);
    }
    static_pdf->compress_level = saved_compress_level;
    return k;
}

static int orig_obj(lua_State * L)
{
    int n, first_arg = 1;
    int k;
    lstring st;
    st.s = NULL;
    n = lua_gettop(L);
    if (n > 0 && lua_type(L, 1) == LUA_TNUMBER) {
        first_arg++;
        lua_number2int(k, lua_tonumber(L, 1));
        check_obj_exists(static_pdf, obj_type_obj, k);
        if (is_obj_scheduled(static_pdf, k) || obj_data_ptr(static_pdf, k) != 0)
            luaL_error(L, "pdf.obj() object in use");
    } else {
        incr(static_pdf->obj_count);
        pdf_create_obj(static_pdf, obj_type_obj, static_pdf->sys_obj_ptr + 1);
        k = static_pdf->sys_obj_ptr;
    }
    pdf_last_obj = k;
    obj_data_ptr(static_pdf, k) = pdf_get_mem(static_pdf, pdfmem_obj_size);
    init_obj_obj(static_pdf, k);
    switch (n - first_arg + 1) {
    case 0:
        luaL_error(L, "pdf.obj() needs at least one argument");
        break;
    case 1:
        if (!lua_isstring(L, first_arg))
            luaL_error(L, "pdf.obj() 1st argument must be string");
        break;
    case 2:
    case 3:
        if (!lua_isstring(L, first_arg))
            luaL_error(L, "pdf.obj() 1st argument must be string");
        if (!lua_isstring(L, first_arg + 1))
            luaL_error(L, "pdf.obj() 2nd argument must be string");
        st.s = (unsigned char *) lua_tolstring(L, first_arg, &st.l);
        if (st.l == 4 && strncmp((const char *) st.s, "file", 4) == 0) {
            if (n == first_arg + 2)
                luaL_error(L, "pdf.obj() 3rd argument forbidden in file mode");
            set_obj_obj_is_file(static_pdf, k);
        } else {
            if (n == first_arg + 2) {   /* write attr text */
                if (!lua_isstring(L, -1))
                    luaL_error(L, "pdf.obj() 3rd argument must be string");
                obj_obj_stream_attr(static_pdf, k) =
                    luaL_ref(Luas, LUA_REGISTRYINDEX);
            }
            if (st.l == 6 && strncmp((const char *) st.s, "stream", 6) == 0) {
                set_obj_obj_is_stream(static_pdf, k);
            } else if (st.l == 10
                       && strncmp((const char *) st.s, "streamfile", 10) == 0) {
                set_obj_obj_is_stream(static_pdf, k);
                set_obj_obj_is_file(static_pdf, k);
            } else
                luaL_error(L, "pdf.obj() invalid argument");
        }
        break;
    default:
        luaL_error(L, "pdf.obj() allows max. 3 arguments");
    }
    obj_obj_data(static_pdf, k) = luaL_ref(L, LUA_REGISTRYINDEX);
    return k;
}

static int l_obj(lua_State * L)
{
    int n;
    int k;
    n = lua_gettop(L);
    if (n == 1 && lua_istable(L, 1))
        k = table_obj(L);       /* new */
    else
        k = orig_obj(L);
    lua_pushinteger(L, k);
    return 1;
}

static int l_reserveobj(lua_State * L)
{
    int n;
    lstring st;
    st.s = 0;
    n = lua_gettop(L);
    switch (n) {
    case 0:
        incr(static_pdf->obj_count);
        pdf_create_obj(static_pdf, obj_type_obj, static_pdf->sys_obj_ptr + 1);
        pdf_last_obj = static_pdf->sys_obj_ptr;
        break;
    case 1:
        if (!lua_isstring(L, -1))
            luaL_error(L, "pdf.reserveobj() optional argument must be string");
        st.s = (unsigned char *) lua_tolstring(L, 1, &st.l);
        if (st.l == 5 && strncmp((const char *) st.s, "annot", 5) == 0) {
            pdf_create_obj(static_pdf, obj_type_annot, 0);
            pdf_last_annot = static_pdf->sys_obj_ptr;
        } else {
            luaL_error(L, "pdf.reserveobj() optional string must be \"annot\"");
        }
        lua_pop(L, 1);
        break;
    default:
        luaL_error(L, "pdf.reserveobj() allows max. 1 argument");
    }
    lua_pushinteger(L, static_pdf->obj_ptr);
    return 1;
}

static int l_registerannot(lua_State * L)
{
    int n, i;
    n = lua_gettop(L);
    switch (n) {
    case 1:
        if (!is_shipping_page)
            luaL_error(L, "pdf.registerannot() can only be used in late lua");
        i = luaL_checkinteger(L, 1);
        if (i <= 0)
            luaL_error(L,
                       "pdf.registerannot() can only register positive object numbers");
        addto_page_resources(static_pdf, obj_type_annot, -i);
        break;
    default:
        luaL_error(L, "pdf.registerannot() needs exactly 1 argument");
    }
    return 0;
}

static int getpdf(lua_State * L)
{
    char *st, *s;
    int l;
    if (lua_isstring(L, 2)) {
        st = (char *) lua_tostring(L, 2);
        if (st) {
            if (strcmp(st, "h") == 0) {
                lua_pushnumber(L, static_pdf->posstruct->pos.h);
            } else if (strcmp(st, "v") == 0) {
                lua_pushnumber(L, static_pdf->posstruct->pos.v);
            } else if (strcmp(st, "pdfcatalog") == 0) {
                s = tokenlist_to_cstring(pdf_catalog_toks, true, &l);
                lua_pushlstring(L, s, l);
            } else if (strcmp(st, "pdfinfo") == 0) {
                s = tokenlist_to_cstring(pdf_info_toks, true, &l);
                lua_pushlstring(L, s, l);
            } else if (strcmp(st, "pdfnames") == 0) {
                s = tokenlist_to_cstring(pdf_names_toks, true, &l);
                lua_pushlstring(L, s, l);
            } else if (strcmp(st, "pdftrailer") == 0) {
                s = tokenlist_to_cstring(pdf_trailer_toks, true, &l);
                lua_pushlstring(L, s, l);
            } else {
                lua_rawget(L, -2);
            }
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int setpdf(lua_State * L)
{
    char *st;
    if (lua_gettop(L) != 3) {
        return 0;
    }
    /* can't set |h| and |v| yet */
    st = (char *) luaL_checkstring(L, 2);
    if (strcmp(st, "pdfcatalog") == 0) {
        pdf_catalog_toks = tokenlist_from_lua(L);
    } else if (strcmp(st, "pdfinfo") == 0) {
        pdf_info_toks = tokenlist_from_lua(L);
    } else if (strcmp(st, "pdfnames") == 0) {
        pdf_names_toks = tokenlist_from_lua(L);
    } else if (strcmp(st, "pdftrailer") == 0) {
        pdf_trailer_toks = tokenlist_from_lua(L);
    } else if (strcmp(st, "pdfmapline") == 0) {
        char *s = (char *) lua_tostring(L, -1);
        process_map_item(s, MAPLINE);
    } else if (strcmp(st, "pdfmapfile") == 0) {
        char *s = (char *) lua_tostring(L, -1);
        process_map_item(s, MAPFILE);
    } else {
        lua_rawset(L, -3);
    }
    return 0;
}

static const struct luaL_reg pdflib[] = {
    {"print", luapdfprint},
    {"immediateobj", l_immediateobj},
    {"obj", l_obj},
    {"registerannot", l_registerannot},
    {"reserveobj", l_reserveobj},
    {NULL, NULL}                /* sentinel */
};

/**********************************************************************/

static void preset_environment(lua_State * L, const parm_struct * p)
{
    int i;
    assert(L != NULL);
    lua_newtable(L);            /* t */
    for (i = 1, ++p; p->name != NULL; i++, p++) {
        assert(i == (int) p->idx);
        lua_pushstring(L, p->name);     /* k t */
        lua_pushinteger(L, (int) p->idx);       /* v k t */
        lua_settable(L, -3);    /* t */
    }
    lua_replace(L, LUA_ENVIRONINDEX);   /* - */
}

int luaopen_pdf(lua_State * L)
{
    preset_environment(L, obj_parms);
    luaL_register(L, "pdf", pdflib);
    /* build meta table */
    luaL_newmetatable(L, "pdf_meta");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, getpdf);
    /* do these later, NYI */
    lua_settable(L, -3);
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, setpdf);
    lua_settable(L, -3);
    lua_setmetatable(L, -2);    /* meta to itself */
    return 1;
}
