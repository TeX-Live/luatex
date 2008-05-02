/* luastuff.c
   
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

#include "luatex-api.h"
#include <ptexlib.h>

static const char _svn_version[] =
    "$Id$ $URL$";

lua_State *Luas[65536];

extern char *startup_filename;
extern int safer_option;

int luastate_max = 0;
int luastate_bytes = 0;

void make_table(lua_State * L, char *tab, char *getfunc, char *setfunc)
{
    /* make the table *//* [{<tex>}] */
    lua_pushstring(L, tab);     /* [{<tex>},"dimen"] */
    lua_newtable(L);            /* [{<tex>},"dimen",{}] */
    lua_settable(L, -3);        /* [{<tex>}] */
    /* fetch it back */
    lua_pushstring(L, tab);     /* [{<tex>},"dimen"] */
    lua_gettable(L, -2);        /* [{<tex>},{<dimen>}] */
    /* make the meta entries */
    luaL_newmetatable(L, tab);  /* [{<tex>},{<dimen>},{<dimen_m>}] */
    lua_pushstring(L, "__index");       /* [{<tex>},{<dimen>},{<dimen_m>},"__index"] */
    lua_pushstring(L, getfunc); /* [{<tex>},{<dimen>},{<dimen_m>},"__index","getdimen"] */
    lua_gettable(L, -5);        /* [{<tex>},{<dimen>},{<dimen_m>},"__index",<tex.getdimen>]  */
    lua_settable(L, -3);        /* [{<tex>},{<dimen>},{<dimen_m>}]  */
    lua_pushstring(L, "__newindex");    /* [{<tex>},{<dimen>},{<dimen_m>},"__newindex"] */
    lua_pushstring(L, setfunc); /* [{<tex>},{<dimen>},{<dimen_m>},"__newindex","setdimen"] */
    lua_gettable(L, -5);        /* [{<tex>},{<dimen>},{<dimen_m>},"__newindex",<tex.setdimen>]  */
    lua_settable(L, -3);        /* [{<tex>},{<dimen>},{<dimen_m>}]  */
    lua_setmetatable(L, -2);    /* [{<tex>},{<dimen>}] : assign the metatable */
    lua_pop(L, 1);              /* [{<tex>}] : clean the stack */
}

static
const char *getS(lua_State * L, void *ud, size_t * size)
{
    LoadS *ls = (LoadS *) ud;
    (void) L;
    if (ls->size == 0)
        return NULL;
    *size = ls->size;
    ls->size = 0;
    return ls->s;
}

void *my_luaalloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    void *ret = NULL;
    if (nsize == 0)
        free(ptr);
    else
        ret = realloc(ptr, nsize);
    luastate_bytes += (nsize - osize);
    return ret;
}

static int my_luapanic(lua_State * L)
{
    (void) L;                   /* to avoid warnings */
    fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n",
            lua_tostring(L, -1));
    return 0;
}


void luainterpreter(int n)
{
    lua_State *L;
    L = lua_newstate(my_luaalloc, NULL);
    if (L == NULL) {
        fprintf(stderr, "Can't create a new Lua state (%d).", n);
        return;
    }
    lua_atpanic(L, &my_luapanic);

    luastate_max++;
    luaL_openlibs(L);

    open_oslibext(L, safer_option);

    lua_getglobal(L, "package");
    lua_pushstring(L, "");
    lua_setfield(L, -2, "cpath");
    lua_pop(L, 1);              /* pop the table */

    /*luaopen_unicode(L); */
    lua_pushcfunction(L, luaopen_unicode);
    lua_pushstring(L, "unicode");
    lua_call(L, 1, 0);

    /*luaopen_zip(L); */
    lua_pushcfunction(L, luaopen_zip);
    lua_pushstring(L, "zip");
    lua_call(L, 1, 0);

    /*luaopen_lpeg(L); */
    lua_pushcfunction(L, luaopen_lpeg);
    lua_pushstring(L, "lpeg");
    lua_call(L, 1, 0);

    /*luaopen_md5(L); */
    lua_pushcfunction(L, luaopen_md5);
    lua_pushstring(L, "md5");
    lua_call(L, 1, 0);

    /*luaopen_lfs(L); */
    lua_pushcfunction(L, luaopen_lfs);
    lua_pushstring(L, "lfs");
    lua_call(L, 1, 0);

    /* zlib. slightly odd calling convention */
    luaopen_zlib(L);
    lua_setglobal(L, "zlib");
    luaopen_gzip(L);
    /* fontforge */
    luaopen_ff(L);

    luaopen_pdf(L);
    luaopen_tex(L);
    luaopen_token(L);
    luaopen_node(L);
    luaopen_texio(L);
    luaopen_kpse(L);
    if (n == 0) {
        luaopen_callback(L);
        lua_createtable(L, 0, 0);
        lua_setglobal(L, "texconfig");
    }
    luaopen_lua(L, n, startup_filename);
    luaopen_stats(L);
    luaopen_font(L);
    luaopen_lang(L);

    /* luaopen_img(L); */
    lua_pushcfunction(L, luaopen_img);
    lua_pushstring(L, "img");
    lua_call(L, 1, 0);

    luaopen_mp(L);

    if (safer_option) {
        /* disable some stuff if --safer */
        (void) hide_lua_value(L, "os", "execute");
        (void) hide_lua_value(L, "os", "rename");
        (void) hide_lua_value(L, "os", "remove");
        (void) hide_lua_value(L, "io", "popen");
        /* make io.open only read files */
        luaL_checkstack(L, 2, "out of stack space");
        lua_getglobal(L, "io");
        lua_getfield(L, -1, "open_ro");
        lua_setfield(L, -2, "open");
        (void) hide_lua_value(L, "io", "tmpfile");
        (void) hide_lua_value(L, "io", "output");
        (void) hide_lua_value(L, "lfs", "chdir");
        (void) hide_lua_value(L, "lfs", "lock");
        (void) hide_lua_value(L, "lfs", "touch");
        (void) hide_lua_value(L, "lfs", "rmdir");
        (void) hide_lua_value(L, "lfs", "mkdir");
    }
    Luas[n] = L;
}

int hide_lua_table(lua_State * L, char *name)
{
    int r = 0;
    lua_getglobal(L, name);
    if (lua_istable(L, -1)) {
        r = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_pushnil(L);
        lua_setglobal(L, name);
    }
    return r;
}

void unhide_lua_table(lua_State * L, char *name, int r)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, r);
    lua_setglobal(L, name);
    luaL_unref(L, LUA_REGISTRYINDEX, r);
}

int hide_lua_value(lua_State * L, char *name, char *item)
{
    int r = 0;
    lua_getglobal(L, name);
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, item);
        r = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_pushnil(L);
        lua_setfield(L, -2, item);
    }
    return r;
}

void unhide_lua_value(lua_State * L, char *name, char *item, int r)
{
    lua_getglobal(L, name);
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, r);
        lua_setfield(L, -2, item);
        luaL_unref(L, LUA_REGISTRYINDEX, r);
    }
}


static int lua_traceback(lua_State * L)
{
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return 1;
    }
    lua_pushvalue(L, 1);        /* pass error message */
    lua_pushinteger(L, 2);      /* skip this function and traceback */
    lua_call(L, 2, 1);          /* call debug.traceback */
    return 1;
}

void luacall(int n, int s)
{
    LoadS ls;
    int i;
    char lua_id[20];
    if (Luas[n] == NULL) {
        luainterpreter(n);
    }
    luatex_load_init(s, &ls);
    if (ls.size > 0) {
        snprintf((char *) lua_id, 20, "\\latelua%d", n);
        i = lua_load(Luas[n], getS, &ls, lua_id);
        if (i != 0) {
            Luas[n] = luatex_error(Luas[n], (i == LUA_ERRSYNTAX ? 0 : 1));
        } else {
            int base = lua_gettop(Luas[n]);     /* function index */
            lua_pushcfunction(Luas[n], lua_traceback);  /* push traceback function */
            lua_insert(Luas[n], base);  /* put it under chunk and args */
            i = lua_pcall(Luas[n], 0, 0, base);
            lua_remove(Luas[n], 1);     /* remove traceback function */
            if (i != 0) {
                lua_gc(Luas[n], LUA_GCCOLLECT, 0);
                Luas[n] = luatex_error(Luas[n], (i == LUA_ERRRUN ? 0 : 1));
            }
        }
    }
}

void luatokencall(int n, int p, int nameptr)
{
    LoadS ls;
    int i, l;
    char *s = NULL;
    char *lua_id;
    if (Luas[n] == NULL) {
        luainterpreter(n);
    }
    l = 0;
    s = tokenlist_to_cstring(p, 1, &l);
    ls.s = s;
    ls.size = l;
    if (ls.size > 0) {
        if (nameptr > 0) {
            lua_id = tokenlist_to_cstring(nameptr, 1, &l);
        } else {
            lua_id = xmalloc(20);
            snprintf((char *) lua_id, 20, "\\directlua%d", n);
        }
        i = lua_load(Luas[n], getS, &ls, lua_id);
        xfree(s);
        if (i != 0) {
            Luas[n] = luatex_error(Luas[n], (i == LUA_ERRSYNTAX ? 0 : 1));
        } else {
            int base = lua_gettop(Luas[n]);     /* function index */
            lua_pushcfunction(Luas[n], lua_traceback);  /* push traceback function */
            lua_insert(Luas[n], base);  /* put it under chunk and args */
            i = lua_pcall(Luas[n], 0, 0, base);
            lua_remove(Luas[n], base);  /* remove traceback function */
            if (i != 0) {
                lua_gc(Luas[n], LUA_GCCOLLECT, 0);
                Luas[n] = luatex_error(Luas[n], (i == LUA_ERRRUN ? 0 : 1));
            }
        }
        xfree(lua_id);
    }
}



void closelua(int n)
{
    if (n != 0 && Luas[n] != NULL) {
        lua_close(Luas[n]);
        luastate_max--;
        Luas[n] = NULL;
    }
}


void luatex_load_init(int s, LoadS * ls)
{
    ls->s = (const char *) &(str_pool[str_start[s]]);
    ls->size = str_start[s + 1] - str_start[s];
}


lua_State *luatex_error(lua_State * L, int is_fatal)
{

    strnumber s;
    const char *luaerr;
    size_t len = 0;
    char *err = NULL;
    if (lua_isstring(L, -1)) {
        luaerr = lua_tolstring(L, -1, &len);
        err = (char *) xmalloc(len + 1);
        len = snprintf(err, (len + 1), "%s", luaerr);
    }
    if (is_fatal > 0) {
        /* Normally a memory error from lua. 
           The pool may overflow during the maketexlstring(), but we 
           are crashing anyway so we may as well abort on the pool size */
        s = maketexlstring(err, len);
        lua_fatal_error(s);
        /* never reached */
        xfree(err);
        lua_close(L);
        luastate_max--;
        return (lua_State *) NULL;
    } else {
        s = maketexlstring(err, len);
        lua_norm_error(s);
        flush_str(s);
        xfree(err);
        return L;
    }
}
