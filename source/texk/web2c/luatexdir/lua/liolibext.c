/* liolibext.c

   Copyright 2014 Taco Hoekwater <taco@luatex.org>

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
   with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

*/

#include "ptexlib.h"
#include "lua/luatex-api.h"

#ifdef LuajitTeX
#include "lua/lauxlib_bridge.h"
#else
#include "lauxlib.h"
#endif
#include "lualib.h"

static FILE *tofile (lua_State *L) {
#ifdef LuajitTeX
    FILE **f = luaL_checkudata(L,1,LUA_FILEHANDLE);
    if (*f == NULL)
        luaL_error(L,"attempt to use a closed file");
    return *f;
#else
    luaL_Stream *p = ((luaL_Stream *)luaL_checkudata(L, 1, LUA_FILEHANDLE));
    if ((p)->closef == NULL)
        luaL_error(L, "attempt to use a closed file");
    lua_assert(p->f);
    return p->f;
#endif
}

/*
    HH: A few helpers to avoid reading numbers as strings. For now we put them in their
    own namespace. We also have a few helpers that can make io functions tex friendly.
*/

static int readcardinal1(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    if (a == EOF)
        lua_pushnil(L);
    else
        lua_pushinteger(L, a);
    return 1;
}

static int readcardinal2(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    if (b == EOF)
        lua_pushnil(L);
    else
        lua_pushinteger(L, 0x100 * a + b);
    return 1;
}

static int readcardinal3(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    int c = getc(f);
    if (c == EOF)
        lua_pushnil(L);
    else
        lua_pushinteger(L, 0x10000 * a + 0x100 * b + c);
    return 1;
}

static int readcardinal4(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    int c = getc(f);
    int d = getc(f);
    if (d == EOF)
        lua_pushnil(L);
    else
        lua_pushinteger(L,0x1000000 * a + 0x10000 * b + 0x100 * c + d);
    return 1;
}

static int readinteger1(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    if (a == EOF)
        lua_pushnil(L);
    else if (a >= 0x80)
        lua_pushinteger(L, a - 0xFF - 1);
    else
        lua_pushinteger(L, a);
    return 1;
}

static int readinteger2(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    if (b == EOF)
        lua_pushnil(L);
    else if (a >= 0x80)
        lua_pushinteger(L, 0x100 * a + b - 0x10000);
    else
        lua_pushinteger(L, 0x100 * a + b);
    return 1;
}

static int readinteger3(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    int c = getc(f);
    if (c == EOF)
        lua_pushnil(L);
    else if (a >= 0x80)
        lua_pushinteger(L, 0x10000 * a + 0x100 * b + c - 0x1000000);
    else
        lua_pushinteger(L, 0x10000 * a + 0x100 * b + c);
    return 1;
}

static int readinteger4(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    int c = getc(f);
    int d = getc(f);
    if (d == EOF)
        lua_pushnil(L);
    else if (a >= 0x80)
        lua_pushinteger(L, 0x1000000 * a + 0x10000 * b + 0x100 * c + d - 0x100000000);
    else
        lua_pushinteger(L, 0x1000000 * a + 0x10000 * b + 0x100 * c + d);
    return 1;
}

static int readfixed2(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    if (b == EOF)
        lua_pushnil(L);
    else if (a >= 0x80)
        lua_pushinteger(L, a + b/0xFFFF - 0x100);
    else
        lua_pushinteger(L, a + b/0xFFFF);
    return 1;
}

static int readfixed4(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    int c = getc(f);
    int d = getc(f);
    if (d == EOF)
        lua_pushnil(L);
    else if (a >= 0x80)
        lua_pushnumber(L, (0x1000000 * a + 0x10000 * b + 0x100 * c + d - 0x100000000)/65536.0);
    else
        lua_pushnumber(L, (0x1000000 * a + 0x10000 * b + 0x100 * c + d)/65536.0);
    return 1;
}

static int read2dot14(lua_State *L) {
    FILE *f = tofile(L);
    int a = getc(f);
    int b = getc(f);
    int c = getc(f);
    int d = getc(f);
    if (d == EOF) {
        lua_pushnil(L);
    } else {
        int n = (0x1000000 * a + 0x10000 * b + 0x100 * c + d);
        lua_pushnumber(L, (n >> 14) + ((n & 0x3fff) / 16384.0));
    }
    return 1;
}

static int getposition(lua_State *L) {
    FILE *f = tofile(L);
    long p = ftell(f);
    if (p<0)
        lua_pushnil(L);
    else
        lua_pushinteger(L, p);
    return 1;
}

static int setposition(lua_State *L) {
    FILE *f = tofile(L);
    long p = lua_tointeger(L,2);
    p = fseek(f,p,SEEK_SET);
    if (p<0)
        lua_pushnil(L);
    else
        lua_pushinteger(L, p);
    return 1;
}

static int skipposition(lua_State *L) {
    FILE *f = tofile(L);
    long p = lua_tointeger(L,2);
    p = fseek(f,ftell(f)+p,SEEK_SET);
    if (p<0)
        lua_pushnil(L);
    else
        lua_pushinteger(L, p);
    return 1;
}

static int readbytetable(lua_State *L) {
    FILE *f = tofile(L);
    int n = lua_tointeger(L,2);
    int i ;
    lua_createtable(L, n, 0);
    for (i=1;i<=n;i++) {
        int a = getc(f);
        if (a == EOF) {
            break;
        } else {
            /*
                lua_pushinteger(L, i);
                lua_pushinteger(L, a);
                lua_rawset(L, -3);
            */
            lua_pushinteger(L, a);
            lua_rawseti(L,-2,i);
        }
    }
    return 1;
}

static int readbytes(lua_State *L) {
    FILE *f = tofile(L);
    int n = lua_tointeger(L,2);
    int i = 0;
    for (i=1;i<=n;i++) {
        int a = getc(f);
        if (a == EOF) {
            return i-1;
        } else {
            lua_pushinteger(L, a);
        }
    }
    return n;
}

static int recordfilename(lua_State *L)
{
    const char *fname = luaL_checkstring(L, 1);
    const char *ftype = lua_tostring(L, 2);
    if (fname != NULL && ftype != NULL) {
        switch (ftype[1]) {
            case 'r':
                recorder_record_input(fname);
                break;
            case 'w':
                recorder_record_output(fname);
                break;
            default:
                /* silently ignore */
                break;
        }
    } else {
        /* silently ignore */
    }
    return 0;
}

static int checkpermission(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 1);
    if (filename == NULL) {
        lua_pushboolean(L,0);
        lua_pushliteral(L,"no command name given");
    } else if (shellenabledp <= 0) {
        lua_pushboolean(L,0);
        lua_pushliteral(L,"all command execution is disabled");
    } else if (restrictedshell == 0) {
        lua_pushboolean(L,1);
        lua_pushliteral(L,"all commands are permitted");
    } else {
        char *safecmd = NULL;
        char *cmdname = NULL;
        switch (shell_cmd_is_allowed(filename, &safecmd, &cmdname)) {
            case 0:
                lua_pushboolean(L,0);
                lua_pushliteral(L, "specific command execution disabled");
                break;
            case 1:
                lua_pushboolean(L,1);
                lua_pushstring(L,filename);
                break;
            case 2:
                lua_pushboolean(L,1);
                lua_pushstring(L,safecmd);
                break;
            default:
                lua_pushboolean(L,0);
                lua_pushliteral(L, "bad command line quoting");
                break;
        }
    }
    return 2;
}

static int readline(lua_State *L)
{
    luaL_Buffer buf;
    int c, d;
    FILE *f = tofile(L);
    luaL_buffinit(L, &buf);
    while (1) {
        c = fgetc(f);
        if (c == EOF) {
            luaL_pushresult(&buf);
            if (lua_rawlen(L, -1) == 0) {
                lua_pop(L, 1);
                lua_pushnil(L);
            }
            return 1;
        } else if (c == '\n') {
            luaL_pushresult(&buf);
            return 1;
        } else if (c == '\r') {
            d = fgetc(f);
            if (d != EOF && d != '\n') {
                ungetc(d, f);
            }
            luaL_pushresult(&buf);
            return 1;
        } else {
            luaL_addchar(&buf, c);
        }
    }
}

static const luaL_Reg fiolib[] = {
    /* helpers */
    { "readcardinal1",   readcardinal1 },
    { "readcardinal2",   readcardinal2 },
    { "readcardinal3",   readcardinal3 },
    { "readcardinal4",   readcardinal4 },
    { "readinteger1",    readinteger1 },
    { "readinteger2",    readinteger2 },
    { "readinteger3",    readinteger3 },
    { "readinteger4",    readinteger4 },
    { "readfixed2",      readfixed2 },
    { "readfixed4",      readfixed4 },
    { "read2dot14",      read2dot14 },
    { "setposition",     setposition },
    { "getposition",     getposition },
    { "skipposition",    skipposition },
    { "readbytes",       readbytes },
    { "readbytetable",   readbytetable },
    { "readline",        readline },
    /* extras */
    { "recordfilename",  recordfilename },
    { "checkpermission", checkpermission },
    /* done */
    {NULL, NULL}
};


int luaopen_fio(lua_State *L) {
     luaL_register(L, "fio", fiolib);
#if defined(_MSC_VER)
    return luaopen_io(L);
#else
     return 1;
#endif /* _MSC_VER */
}
