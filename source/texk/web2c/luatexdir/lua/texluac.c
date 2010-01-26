/* texlua.c

* Copyright (C) 1994-2007 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>

   This file is part of LuaTeX.
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define luac_c
#define LUA_CORE

#include "lua51/lua.h"
#include "lua51/lauxlib.h"

#include "lua51/ldo.h"
#include "lua51/lfunc.h"
#include "lua51/lmem.h"
#include "lua51/lobject.h"
#include "lua51/lopcodes.h"
#include "lua51/lstring.h"
#include "lua51/lundump.h"

static const char _svn_version[] =
    "$Id$ $URL$";

#define PROGNAME        "texluac"       /* default program name */
#define OUTPUT          PROGNAME ".out" /* default output file */

static int dumping = 1;         /* dump bytecodes? */
static int stripping = 0;       /* strip debug information? */
static char Output[] = { OUTPUT };      /* default output file name */

static const char *output = Output;     /* actual output file name */
static const char *progname = PROGNAME; /* actual program name */

__attribute__ ((noreturn))
static void fatal(const char *message)
{
    fprintf(stderr, "%s: %s\n", progname, message);
    exit(EXIT_FAILURE);
}

__attribute__ ((noreturn))
static void cannot(const char *what)
{
    fprintf(stderr, "%s: cannot %s %s: %s\n", progname, what, output,
            strerror(errno));
    exit(EXIT_FAILURE);
}

__attribute__ ((noreturn))
static void usage(const char *message)
{
    if (*message == '-')
        fprintf(stderr, "%s: unrecognized option " LUA_QS "\n", progname,
                message);
    else
        fprintf(stderr, "%s: %s\n", progname, message);
    fprintf(stderr,
            "usage: %s [options] [filenames].\n"
            "Available options are:\n"
            "  -        process stdin\n"
            "  -o name  output to file " LUA_QL("name") " (default is \"%s\")\n"
            "  -p       parse only\n"
            "  -s       strip debug information\n"
            "  -v       show version information\n"
            "  --       stop handling options\n", progname, Output);
    exit(EXIT_FAILURE);
}

#define IS(s)   (strcmp(argv[i],s)==0)

static int doargs(int argc, char *argv[])
{
    int i;
    int version = 0;
    if (argv[0] != NULL && *argv[0] != 0)
        progname = argv[0];
    for (i = 1; i < argc; i++) {
        if (*argv[i] != '-')    /* end of options; keep it */
            break;
        else if (IS("--")) {    /* end of options; skip it */
            ++i;
            if (version)
                ++version;
            break;
        } else if (IS("-"))     /* end of options; use stdin */
            break;
        else if (IS("-o")) {    /* output file */
            output = argv[++i];
            if (output == NULL || *output == 0)
                usage(LUA_QL("-o") " needs argument");
            if (IS("-"))
                output = NULL;
        } else if (IS("-p"))    /* parse only */
            dumping = 0;
        else if (IS("-s"))      /* strip debug information */
            stripping = 1;
        else if (IS("-v"))      /* show version */
            ++version;
        else if (IS("--luaconly"))      /* ignore */
            ;
        else if (IS("--luac"))  /* ignore */
            ;
        else                    /* unknown option */
            usage(argv[i]);
    }
    if (i == argc && (!dumping)) {
        dumping = 0;
        argv[--i] = Output;
    }
    if (version) {
        printf("%s  %s\n", LUA_RELEASE, LUA_COPYRIGHT);
        if (version == argc - 1)
            exit(EXIT_SUCCESS);
    }
    return i;
}

#define toproto(L,i) (clvalue(L->top+(i))->l.p)

static const Proto *combine(lua_State * L, int n)
{
    if (n == 1)
        return toproto(L, -1);
    else {
        int i, pc;
        Proto *f = luaF_newproto(L);
        setptvalue2s(L, L->top, f);
        incr_top(L);
        f->source = luaS_newliteral(L, "=(" PROGNAME ")");
        f->maxstacksize = 1;
        pc = 2 * n + 1;
        f->code = luaM_newvector(L, (unsigned)pc, Instruction);
        f->sizecode = pc;
        f->p = luaM_newvector(L, (unsigned)n, Proto *);
        f->sizep = n;
        pc = 0;
        for (i = 0; i < n; i++) {
            f->p[i] = toproto(L, i - n - 1);
            f->code[pc++] = CREATE_ABx(OP_CLOSURE, 0, i);
            f->code[pc++] = CREATE_ABC(OP_CALL, 0, 1, 1);
        }
        f->code[pc++] = CREATE_ABC(OP_RETURN, 0, 1, 0);
        return f;
    }
}

static int writer(lua_State * L, const void *p, size_t size, void *u)
{
    UNUSED(L);
    return (fwrite(p, size, 1, (FILE *) u) != 1) && (size != 0);
}

struct Smain {
    int argc;
    char **argv;
};

static int pmain(lua_State * L)
{
    struct Smain *s = (struct Smain *) lua_touserdata(L, 1);
    int argc = s->argc;
    char **argv = s->argv;
    const Proto *f;
    int i;
    if (!lua_checkstack(L, argc))
        fatal("too many input files");
    for (i = 0; i < argc; i++) {
        const char *filename = IS("-") ? NULL : argv[i];
        if (luaL_loadfile(L, filename) != 0)
            fatal(lua_tostring(L, -1));
    }
    f = combine(L, argc);
    if (dumping) {
        FILE *D = (output == NULL) ? stdout : fopen(output, "wb");
        if (D == NULL)
            cannot("open");
        lua_lock(L);
        luaU_dump(L, f, writer, D, stripping);
        lua_unlock(L);
        if (ferror(D))
            cannot("write");
        if (fclose(D))
            cannot("close");
    }
    return 0;
}

int luac_main(int argc, char *argv[])
{
    lua_State *L;
    struct Smain s;
    int i = doargs(argc, argv);
    argc -= i;
    argv += i;
    if (argc <= 0)
        usage("no input files given");
    L = lua_open();
    if (L == NULL)
        fatal("not enough memory for state");
    s.argc = argc;
    s.argv = argv;
    if (lua_cpcall(L, pmain, &s) != 0)
        fatal(lua_tostring(L, -1));
    lua_close(L);
    return EXIT_SUCCESS;
}
