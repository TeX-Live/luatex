/* lcallbacklib.c
   
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
#include "lua/luatex-api.h"

static const char _svn_version[] =
    "$Id$ $URL$";

int callback_count = 0;
int saved_callback_count = 0;

int callback_set[total_callbacks] = { 0 };

/* See also callback_callback_type in luatexcallbackids.h: they must have the same order ! */
static const char *const callbacknames[] = {
    "",                         /* empty on purpose */
    "find_write_file",
    "find_output_file",
    "find_image_file",
    "find_format_file",
    "find_read_file", "open_read_file",
    "find_vf_file", "read_vf_file",
    "find_data_file", "read_data_file",
    "find_font_file", "read_font_file",
    "find_map_file", "read_map_file",
    "find_enc_file", "read_enc_file",
    "find_type1_file", "read_type1_file",
    "find_truetype_file", "read_truetype_file",
    "find_opentype_file", "read_opentype_file",
    "find_sfd_file", "read_sfd_file",
    "find_cidmap_file", "read_cidmap_file",
    "find_pk_file", "read_pk_file",
    "show_error_hook",
    "process_input_buffer", "process_output_buffer",
    "process_jobname",
    "start_page_number", "stop_page_number",
    "start_run", "stop_run",
    "define_font",
    "token_filter",
    "pre_output_filter",
    "buildpage_filter",
    "hpack_filter", "vpack_filter",
    "char_exists",
    "hyphenate",
    "ligaturing",
    "kerning",
    "pre_linebreak_filter",
    "linebreak_filter",
    "post_linebreak_filter",
    "mlist_to_hlist",
    "finish_pdffile",
    "finish_pdfpage",
    "pre_dump","start_file", "stop_file",
    "show_error_message","show_lua_error_hook",
    NULL
};

int callback_callbacks_id = 0;

int debug_callback_defined(int i) 
{
    printf ("callback_defined(%s)\n", callbacknames[i]);
    return callback_set[i];
}

void get_lua_boolean(const char *table, const char *name, boolean * target)
{
    int stacktop;
    stacktop = lua_gettop(Luas);
    luaL_checkstack(Luas, 2, "out of stack space");
    lua_getglobal(Luas, table);
    if (lua_istable(Luas, -1)) {
        lua_getfield(Luas, -1, name);
        if (lua_isboolean(Luas, -1)) {
            *target = (boolean) (lua_toboolean(Luas, -1));
        } else if (lua_isnumber(Luas, -1)) {
            *target = (boolean) (lua_tonumber(Luas, -1) == 0 ? 0 : 1);
        }
    }
    lua_settop(Luas, stacktop);
    return;
}

void get_saved_lua_boolean(int r, const char *name, boolean * target)
{
    int stacktop;
    stacktop = lua_gettop(Luas);
    luaL_checkstack(Luas, 2, "out of stack space");
    lua_rawgeti(Luas, LUA_REGISTRYINDEX, r);
    if (lua_istable(Luas, -1)) {
        lua_getfield(Luas, -1, name);
        if (lua_isboolean(Luas, -1)) {
            *target = (boolean) lua_toboolean(Luas, -1);
        } else if (lua_isnumber(Luas, -1)) {
            *target = (boolean) (lua_tonumber(Luas, -1) == 0 ? 0 : 1);
        }
    }
    lua_settop(Luas, stacktop);
    return;
}

void get_lua_number(const char *table, const char *name, int *target)
{
    int stacktop;
    stacktop = lua_gettop(Luas);
    luaL_checkstack(Luas, 2, "out of stack space");
    lua_getglobal(Luas, table);
    if (lua_istable(Luas, -1)) {
        lua_getfield(Luas, -1, name);
        if (lua_isnumber(Luas, -1)) {
            *target = (int)lua_tonumber(Luas, -1);
        }
    }
    lua_settop(Luas, stacktop);
    return;
}

void get_saved_lua_number(int r, const char *name, int *target)
{
    int stacktop;
    stacktop = lua_gettop(Luas);
    luaL_checkstack(Luas, 2, "out of stack space");
    lua_rawgeti(Luas, LUA_REGISTRYINDEX, r);
    if (lua_istable(Luas, -1)) {
        lua_getfield(Luas, -1, name);
        if (lua_isnumber(Luas, -1)) {
            *target=(int)lua_tonumber(Luas, -1);
        }
    }
    lua_settop(Luas, stacktop);
    return;
}


void get_lua_string(const char *table, const char *name, char **target)
{
    int stacktop;
    stacktop = lua_gettop(Luas);
    luaL_checkstack(Luas, 2, "out of stack space");
    lua_getglobal(Luas, table);
    if (lua_istable(Luas, -1)) {
        lua_getfield(Luas, -1, name);
        if (lua_isstring(Luas, -1)) {
            *target = xstrdup(lua_tostring(Luas, -1));
        }
    }
    lua_settop(Luas, stacktop);
    return;
}

void get_saved_lua_string(int r, const char *name, char **target)
{
    int stacktop;
    stacktop = lua_gettop(Luas);
    luaL_checkstack(Luas, 2, "out of stack space");
    lua_rawgeti(Luas, LUA_REGISTRYINDEX, r);
    if (lua_istable(Luas, -1)) {
        lua_getfield(Luas, -1, name);
        if (lua_isstring(Luas, -1)) {
            *target = xstrdup(lua_tostring(Luas, -1));
        }
    }
    lua_settop(Luas, stacktop);
    return;
}


#define CALLBACK_BOOLEAN        'b'
#define CALLBACK_INTEGER        'd'
#define CALLBACK_LINE           'l'
#define CALLBACK_STRNUMBER      's'
#define CALLBACK_STRING         'S'
#define CALLBACK_CHARNUM        'c'
#define CALLBACK_LSTRING        'L'


int run_saved_callback(int r, const char *name, const char *values, ...)
{
    va_list args;
    int ret = 0;
    lua_State *L = Luas;
    int stacktop = lua_gettop(L);
    va_start(args, values);
    luaL_checkstack(L, 2, "out of stack space");
    lua_rawgeti(L, LUA_REGISTRYINDEX, r);
    lua_pushstring(L, name);
    lua_rawget(L, -2);
    if (lua_isfunction(L, -1)) {
        saved_callback_count++;
        callback_count++;
        ret = do_run_callback(2, values, args);
    }
    va_end(args);
    lua_settop(L, stacktop);
    return ret;
}


boolean get_callback(lua_State * L, int i)
{
    luaL_checkstack(L, 2, "out of stack space");
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback_callbacks_id);
    lua_rawgeti(L, -1, i);
    if (lua_isfunction(L, -1)) {
        callback_count++;
        return true;
    } else {
        return false;
    }
}

int run_and_save_callback(int i, const char *values, ...)
{
    va_list args;
    int ret = 0;
    lua_State *L = Luas;
    int stacktop = lua_gettop(L);
    va_start(args, values);
    if (get_callback(L, i)) {
        ret = do_run_callback(1, values, args);
    }
    va_end(args);
    if (ret > 0) {
        ret = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_settop(L, stacktop);
    return ret;
}


int run_callback(int i, const char *values, ...)
{
    va_list args;
    int ret = 0;
    lua_State *L = Luas;
    int stacktop = lua_gettop(L);
    va_start(args, values);
    if (get_callback(L, i)) {
        ret = do_run_callback(0, values, args);
    }
    va_end(args);
    lua_settop(L, stacktop);
    return ret;
}

int do_run_callback(int special, const char *values, va_list vl)
{
    int ret;
    size_t len;
    int narg, nres;
    const char *s;
    lstring *lstr;
    char cs;
    int *bufloc;
    char *ss = NULL;
    int retval = 0;
    lua_State *L = Luas;
    if (special == 2) {         /* copy the enclosing table */
        luaL_checkstack(L, 1, "out of stack space");
        lua_pushvalue(L, -2);
    }
    ss = strchr(values, '>');
    assert(ss);
    luaL_checkstack(L, (int) (ss - values + 1), "out of stack space");
    ss = NULL;
    for (narg = 0; *values; narg++) {
        switch (*values++) {
        case CALLBACK_CHARNUM: /* an ascii char! */
            cs = (char) va_arg(vl, int);
            lua_pushlstring(L, &cs, 1);
            break;
        case CALLBACK_STRING:  /* C string */
            s = va_arg(vl, char *);
            lua_pushstring(L, s);
            break;
        case CALLBACK_LSTRING:  /* 'lstring' */
            lstr = va_arg(vl, lstring *);
            lua_pushlstring(L, (const char *)lstr->s, lstr->l);
            break;
        case CALLBACK_INTEGER: /* int */
            lua_pushnumber(L, va_arg(vl, int));
            break;
        case CALLBACK_STRNUMBER:       /* TeX string */
            s = makeclstring(va_arg(vl, int), &len);
            lua_pushlstring(L, s, len);
            break;
        case CALLBACK_BOOLEAN: /* boolean */
            lua_pushboolean(L, va_arg(vl, int));
            break;
        case CALLBACK_LINE:    /* a buffer section, with implied start */
            lua_pushlstring(L, (char *) (buffer + first),
                            (size_t) va_arg(vl, int));
            break;
        case '-':
            narg--;
            break;
        case '>':
            goto ENDARGS;
        default:
            ;
        }
    }
  ENDARGS:
    nres = (int) strlen(values);
    if (special == 1) {
        nres++;
    }
    if (special == 2) {
        narg++;
    }
    {
        int i;
        lua_active++;
        i = lua_pcall(L, narg, nres, 0);
        lua_active--;
        /* lua_remove(L, base); *//* remove traceback function */
        if (i != 0) {
            /* Can't be more precise here, could be called before 
             * TeX initialization is complete 
             */
            if (!log_opened_global) {
                fprintf(stderr, "This went wrong: %s\n", lua_tostring(L, -1));
                error();
            } else {
                lua_gc(L, LUA_GCCOLLECT, 0);
                luatex_error(L, (i == LUA_ERRRUN ? 0 : 1));
            }
            return 0;
        }
    }
    if (nres == 0) {
        return 1;
    }
    nres = -nres;
    while (*values) {
        int b;
        switch (*values++) {
        case CALLBACK_BOOLEAN:
            if (!lua_isboolean(L, nres)) {
                fprintf(stderr, "Expected a boolean, not: %s\n",
                        lua_typename(L, lua_type(L, nres)));
                goto EXIT;
            }
            b = lua_toboolean(L, nres);
            *va_arg(vl, boolean *) = (boolean) b;
            break;
        case CALLBACK_INTEGER:
            if (!lua_isnumber(L, nres)) {
                fprintf(stderr, "Expected a number, not: %s\n",
                        lua_typename(L, lua_type(L, nres)));
                goto EXIT;
            }
	    b=(int)lua_tonumber(L, nres);
            *va_arg(vl, int *) = b;
            break;
        case CALLBACK_LINE:    /* TeX line */
            if (!lua_isstring(L, nres)) {
                if (!lua_isnil(L, nres))
                    fprintf(stderr, "Expected a string for (l), not: %s\n",
                            lua_typename(L, lua_type(L, nres)));
                goto EXIT;
            }
            s = lua_tolstring(L, nres, &len);
            if (s != NULL) {    /* |len| can be zero */
                bufloc = va_arg(vl, int *);
                if (len != 0) {
                    ret = *bufloc;
                    check_buffer_overflow(ret + (int) len);
                    strncpy((char *) (buffer + ret), s, len);
                    *bufloc += (int) len;
                    /* while (len--) {  buffer[(*bufloc)++] = *s++; } */
                    while ((*bufloc) - 1 > ret && buffer[(*bufloc) - 1] == ' ')
                        (*bufloc)--;
                }
            } else {
                bufloc = 0;
            }
            break;
        case CALLBACK_STRNUMBER:       /* TeX string */
            if (!lua_isstring(L, nres)) {
                if (!lua_isnil(L, nres)) {
                    fprintf(stderr, "Expected a string for (s), not: %s\n",
                            lua_typename(L, lua_type(L, nres)));
                    goto EXIT;
                }
            }
            s = lua_tolstring(L, nres, &len);
            if (s == NULL)      /* |len| can be zero */
                *va_arg(vl, int *) = 0;
            else {
                *va_arg(vl, int *) = maketexlstring(s, len);
            }
            break;
        case CALLBACK_STRING:  /* C string aka buffer */
            if (!lua_isstring(L, nres)) {
                if (!lua_isnil(L, nres)) {
                    fprintf(stderr, "Expected a string for (S), not: %s\n",
                            lua_typename(L, lua_type(L, nres)));
                    goto EXIT;
                }
            }
            s = lua_tolstring(L, nres, &len);

            if (s == NULL)      /* |len| can be zero */
                *va_arg(vl, int *) = 0;
            else {
                ss = xmalloc((unsigned) (len + 1));
                (void) memcpy(ss, s, (len + 1));
                *va_arg(vl, char **) = ss;
            }
            break;
        case CALLBACK_LSTRING:  /* lstring */
            if (!lua_isstring(L, nres)) {
                if (!lua_isnil(L, nres)) {
                    fprintf(stderr, "Expected a string for (S), not: %s\n",
                            lua_typename(L, lua_type(L, nres)));
                    goto EXIT;
                }
            }
            s = lua_tolstring(L, nres, &len);

            if (s == NULL)      /* |len| can be zero */
                *va_arg(vl, int *) = 0;
            else {
	        lstring *ret = xmalloc(sizeof(lstring));
                ret->s = xmalloc((unsigned) (len + 1));
                (void) memcpy(ret->s, s, (len + 1));
		ret->l = len;
                *va_arg(vl, lstring **) = ret;
            }
            break;
        default:
            fprintf(stdout, "invalid return value type");
            goto EXIT;
        }
        nres++;
    }
    retval = 1;
  EXIT:
    return retval;
}

void destroy_saved_callback(int i)
{
    luaL_unref(Luas, LUA_REGISTRYINDEX, i);
}

static int callback_register(lua_State * L)
{
    int cb;
    const char *s;
    if (!lua_isstring(L, 1) ||
        ((!lua_isfunction(L, 2)) &&
         (!lua_isnil(L, 2)) &&
         (!(lua_isboolean(L, 2) && lua_toboolean(L, 2) == 0)))) {
        lua_pushnil(L);
        lua_pushstring(L, "Invalid arguments to callback.register.");
        return 2;
    }
    s = lua_tostring(L, 1);
    for (cb = 0; cb < total_callbacks; cb++) {
        if (strcmp(callbacknames[cb], s) == 0)
            break;
    }
    if (cb == total_callbacks) {
        lua_pushnil(L);
        lua_pushstring(L, "No such callback exists.");
        return 2;
    }
    if (lua_isfunction(L, 2)) {
        callback_set[cb] = cb;
    } else if (lua_isboolean(L, 2)) {
        callback_set[cb] = -1;
    } else {
        callback_set[cb] = 0;
    }
    luaL_checkstack(L, 2, "out of stack space");
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback_callbacks_id);   /* push the table */
    lua_pushvalue(L, 2);        /* the function or nil */
    lua_rawseti(L, -2, cb);
    lua_rawseti(L, LUA_REGISTRYINDEX, callback_callbacks_id);
    lua_pushnumber(L, cb);
    return 1;
}

static int callback_find(lua_State * L)
{
    int cb;
    const char *s;
    if (!lua_isstring(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "Invalid arguments to callback.find.");
        return 2;
    }
    s = lua_tostring(L, 1);
    for (cb = 0; cb < total_callbacks; cb++) {
        if (strcmp(callbacknames[cb], s) == 0)
            break;
    }
    if (cb == total_callbacks) {
        lua_pushnil(L);
        lua_pushstring(L, "No such callback exists.");
        return 2;
    }
    luaL_checkstack(L, 2, "out of stack space");
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback_callbacks_id);   /* push the table */
    lua_rawgeti(L, -1, cb);
    return 1;
}


static int callback_listf(lua_State * L)
{
    int i;
    luaL_checkstack(L, 3, "out of stack space");
    lua_newtable(L);
    for (i = 1; callbacknames[i]; i++) {
        lua_pushstring(L, callbacknames[i]);
        if (callback_defined(i)) {
            lua_pushboolean(L, 1);
        } else {
            lua_pushboolean(L, 0);
        }
        lua_rawset(L, -3);
    }
    return 1;
}

static const struct luaL_Reg callbacklib[] = {
    {"find", callback_find},
    {"register", callback_register},
    {"list", callback_listf},
    {NULL, NULL}                /* sentinel */
};

int luaopen_callback(lua_State * L)
{
    luaL_register(L, "callback", callbacklib);
    luaL_checkstack(L, 1, "out of stack space");
    lua_newtable(L);
    callback_callbacks_id = luaL_ref(L, LUA_REGISTRYINDEX);
    return 1;
}
