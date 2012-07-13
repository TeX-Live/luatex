% texluac.w
%
% Copyright (C) 1994-2007 Lua.org, PUC-Rio.  All rights reserved.
% Copyright 2006-2012 Taco Hoekwater <taco@@luatex.org>
%
% Permission is hereby granted, free of charge, to any person obtaining
% a copy of this software and associated documentation files (the
% "Software"), to deal in the Software without restriction, including
% without limitation the rights to use, copy, modify, merge, publish,
% distribute, sublicense, and/or sell copies of the Software, and to
% permit persons to whom the Software is furnished to do so, subject to
% the following conditions:
%
% The above copyright notice and this permission notice shall be
% included in all copies or substantial portions of the Software.
%
% THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
% EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
% MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
% IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
% CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
% TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
% SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
%
% This file is part of LuaTeX.

@ @c
static const char _svn_version[] =
    "$Id$"
    "$URL$";

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

#include "lua/luatex-api.h"

@ @c
/*  fix for non-gcc compilation: */
#if !defined(__GNUC__) || (__GNUC__ < 2) 
# define __attribute__(x)
#endif /* !defined(__GNUC__) || (__GNUC__ < 2) */

@ @c
#define PROGNAME        "texluac"       /* default program name */
#define OUTPUT          PROGNAME ".out" /* default output file */

static int dumping = 1;         /* dump bytecodes? */
static int stripping = 0;       /* strip debug information? */
static char Output[] = { OUTPUT };      /* default output file name */

static const char *output = Output;     /* actual output file name */
static const char *progname = PROGNAME; /* actual program name */

@ @c
__attribute__ ((noreturn))
static void fatal(const char *message)
{
    fprintf(stderr, "%s: %s\n", progname, message);
    exit(EXIT_FAILURE);
}

@ @c
__attribute__ ((noreturn))
static void cannot(const char *what)
{
    fprintf(stderr, "%s: cannot %s %s: %s\n", progname, what, output,
            strerror(errno));
    exit(EXIT_FAILURE);
}

@ @c
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

@ @c
#define IS(s)   (strcmp(argv[i],s)==0)

static int doargs(int ac, char *av[])
{
    int i;
    int version = 0;
    if (av[0] != NULL && *av[0] != 0)
        progname = av[0];
    for (i = 1; i < ac; i++) {
        if (*av[i] != '-')    /* end of options; keep it */
            break;
        else if (IS("--")) {    /* end of options; skip it */
            ++i;
            if (version)
                ++version;
            break;
        } else if (IS("-"))     /* end of options; use stdin */
            break;
        else if (IS("-o")) {    /* output file */
            output = av[++i];
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
            usage(av[i]);
    }
    if (i == ac && (!dumping)) {
        dumping = 0;
        av[--i] = Output;
    }
    if (version) {
        printf("%s  %s\n", LUA_RELEASE, LUA_COPYRIGHT);
        if (version == ac - 1)
            exit(EXIT_SUCCESS);
    }
    return i;
}

@ @c
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
        f->code = luaM_newvector(L, (unsigned long) pc, Instruction);
        f->sizecode = pc;
        f->p = luaM_newvector(L, (unsigned long) n, Proto *);
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

@ @c
static int writer(lua_State * L, const void *p, size_t size, void *u)
{
    UNUSED(L);
    return (fwrite(p, size, 1, (FILE *) u) != 1) && (size != 0);
}

@ @c
struct Smain {
    int argc;
    char **argv;
};

@ @c
static int pmain(lua_State * L)
{
    struct Smain *s = (struct Smain *) lua_touserdata(L, 1);
    int ac = s->argc;
    char **av = s->argv;
    const Proto *f;
    int i;
    if (!lua_checkstack(L, ac))
        fatal("too many input files");
    for (i = 0; i < ac; i++) {
        const char *filename = IS("-") ? NULL : av[i];
        if (luaL_loadfile(L, filename) != 0)
            fatal(lua_tostring(L, -1));
    }
    f = combine(L, ac);
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

@ @c
int luac_main(int ac, char *av[])
{
    lua_State *L;
    struct Smain s;
    int i = doargs(ac, av);
    ac -= i;
    av += i;
    if (ac <= 0)
        usage("no input files given");
    L = lua_open();
    if (L == NULL)
        fatal("not enough memory for state");
    s.argc = ac;
    s.argv = av;
    if (lua_cpcall(L, pmain, &s) != 0)
        fatal(lua_tostring(L, -1));
    lua_close(L);
    return EXIT_SUCCESS;
}
