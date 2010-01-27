/* llanglib.c
   
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

#include "lua/luatex-api.h"
#include "ptexlib.h"



static const char _svn_version[] =
    "$Id$ $URL$";

#define LANG_METATABLE "luatex.lang"

#define check_islang(L,b) (struct tex_language **)luaL_checkudata(L,b,LANG_METATABLE)

static int lang_new(lua_State * L)
{
    struct tex_language **lang;
    if (lua_gettop(L) == 0) {
        lang = lua_newuserdata(L, sizeof(struct tex_language *));
        *lang = new_language(-1);
        if (!*lang) {
            lua_pushstring(L, "lang.new(): no room for a new language");
            return lua_error(L);
        }
    } else {
        int lualang;
        lang = lua_newuserdata(L, sizeof(struct tex_language *));
        lua_number2int(lualang, lua_tonumber(L, 1));
        *lang = get_language(lualang);
        if (!*lang) {
            lua_pushfstring(L, "lang.new(%d): undefined language",
                            lua_tonumber(L, 1));
            return lua_error(L);
        }
    }
    luaL_getmetatable(L, LANG_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int lang_id(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    lua_pushnumber(L, (*lang_ptr)->id);
    return 1;
}

static int lang_patterns(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    if (lua_gettop(L) != 1) {
        if (!lua_isstring(L, 2)) {
            lua_pushstring(L, "lang.patterns(): argument should be a string");
            return lua_error(L);
        }
        load_patterns(*lang_ptr, (const unsigned char *) lua_tostring(L, 2));
        return 0;
    } else {
        if ((*lang_ptr)->patterns != NULL) {
            lua_pushstring(L, (char *) hnj_serialize((*lang_ptr)->patterns));
        } else {
            lua_pushnil(L);
        }
        return 1;
    }
}

static int lang_clear_patterns(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    clear_patterns(*lang_ptr);
    return 0;
}


static int lang_hyphenation(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    if (lua_gettop(L) != 1) {
        if (!lua_isstring(L, 2)) {
            lua_pushstring(L,
                           "lang.hyphenation(): argument should be a string");
            return lua_error(L);
        }
        load_hyphenation(*lang_ptr, (const unsigned char *) lua_tostring(L, 2));
        return 0;
    } else {
        if ((*lang_ptr)->exceptions != 0) {
            lua_pushstring(L, exception_strings(*lang_ptr));
        } else {
            lua_pushnil(L);
        }
        return 1;
    }
}

static int lang_pre_hyphen_char(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    if (lua_gettop(L) != 1) {
        if (!lua_isnumber(L, 2)) {
            lua_pushstring(L,
                           "lang.prehyphenchar(): argument should be a character number");
            return lua_error(L);
        }
        lua_number2int((*lang_ptr)->pre_hyphen_char, lua_tonumber(L, 2));
        return 0;
    } else {
        lua_pushnumber(L, (*lang_ptr)->pre_hyphen_char);
        return 1;
    }
}

static int lang_post_hyphen_char(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    if (lua_gettop(L) != 1) {
        if (!lua_isnumber(L, 2)) {
            lua_pushstring(L,
                           "lang.posthyphenchar(): argument should be a character number");
            return lua_error(L);
        }
        lua_number2int((*lang_ptr)->post_hyphen_char, lua_tonumber(L, 2));
        return 0;
    } else {
        lua_pushnumber(L, (*lang_ptr)->post_hyphen_char);
        return 1;
    }
}


static int lang_pre_exhyphen_char(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    if (lua_gettop(L) != 1) {
        if (!lua_isnumber(L, 2)) {
            lua_pushstring(L,
                           "lang.preexhyphenchar(): argument should be a character number");
            return lua_error(L);
        }
        lua_number2int((*lang_ptr)->pre_exhyphen_char, lua_tonumber(L, 2));
        return 0;
    } else {
        lua_pushnumber(L, (*lang_ptr)->pre_exhyphen_char);
        return 1;
    }
}

static int lang_post_exhyphen_char(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    if (lua_gettop(L) != 1) {
        if (!lua_isnumber(L, 2)) {
            lua_pushstring(L,
                           "lang.postexhyphenchar(): argument should be a character number");
            return lua_error(L);
        }
        lua_number2int((*lang_ptr)->post_exhyphen_char, lua_tonumber(L, 2));
        return 0;
    } else {
        lua_pushnumber(L, (*lang_ptr)->post_exhyphen_char);
        return 1;
    }
}


static int lang_clear_hyphenation(lua_State * L)
{
    struct tex_language **lang_ptr;
    lang_ptr = check_islang(L, 1);
    clear_hyphenation(*lang_ptr);
    return 0;
}


static int do_lang_clean(lua_State * L)
{
    char *cleaned;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "lang.clean(): argument should be a string");
        return lua_error(L);
    }
    (void) clean_hyphenation(lua_tostring(L, 1), &cleaned);
    lua_pushstring(L, cleaned);
    return 1;
}

static int do_lang_hyphenate(lua_State * L)
{
    halfword *h, *t, tt;
    h = check_isnode(L, 1);
    if (lua_isuserdata(L, 2)) {
        t = check_isnode(L, 2);
        tt = *t;
        lua_pop(L, 1);
    } else {
        tt = *h;
        while (vlink(tt) != null)
            tt = vlink(tt);
    }
    hnj_hyphenation(*h, tt);
    lua_pushboolean(L, 1);
    return 1;
}


static const struct luaL_reg langlib_d[] = {
    /* *INDENT-OFF* */
    {"clear_patterns",    lang_clear_patterns},
    {"clear_hyphenation", lang_clear_hyphenation},
    {"patterns",          lang_patterns},
    {"hyphenation",       lang_hyphenation},
    {"prehyphenchar",     lang_pre_hyphen_char},
    {"posthyphenchar",    lang_post_hyphen_char},
    {"preexhyphenchar",   lang_pre_exhyphen_char},
    {"postexhyphenchar",  lang_post_exhyphen_char},
    {"id",                lang_id},
    /* *INDENT-ON* */
    {NULL, NULL}                /* sentinel */
};


static const struct luaL_reg langlib[] = {
    /* *INDENT-OFF* */
    {"clear_patterns",    lang_clear_patterns},
    {"clear_hyphenation", lang_clear_hyphenation},
    {"patterns",          lang_patterns},
    {"hyphenation",       lang_hyphenation},
    {"prehyphenchar",     lang_pre_hyphen_char},
    {"posthyphenchar",    lang_post_hyphen_char},
    {"preexhyphenchar",   lang_pre_exhyphen_char},
    {"postexhyphenchar",  lang_post_exhyphen_char},
    {"id",                lang_id},
    {"clean",             do_lang_clean},
    {"hyphenate",         do_lang_hyphenate},
    {"new",               lang_new},
    /* *INDENT-ON* */
    {NULL, NULL}                /* sentinel */
};


int luaopen_lang(lua_State * L)
{
    luaL_newmetatable(L, LANG_METATABLE);
    lua_pushvalue(L, -1);       /* push metatable */
    lua_setfield(L, -2, "__index");     /* metatable.__index = metatable */
    luaL_register(L, NULL, langlib_d);  /* dict methods */
    luaL_register(L, "lang", langlib);
    return 1;
}
