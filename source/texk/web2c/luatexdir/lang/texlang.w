% texlang.w
% 
% Copyright 2006-2010 Taco Hoekwater <taco@@luatex.org>

% This file is part of LuaTeX.

% LuaTeX is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free
% Software Foundation; either version 2 of the License, or (at your
% option) any later version.

% LuaTeX is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
% FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License for more details.

% You should have received a copy of the GNU General Public License along
% with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

@ @c
#include "ptexlib.h"

#include <string.h>

#include "lua/luatex-api.h"


static const char _svn_version[] =
    "$Id$ "
"$URL$";


@ Low-level helpers 

@c
#define ex_hyphen_char int_par(ex_hyphen_char_code)
static char *uni2string(char *utf8_text, unsigned ch)
{
    /* Increment and deposit character */
    if (ch >= 17 * 65536)
        return (utf8_text);

    if (ch <= 127)
        *utf8_text++ = (char) ch;
    else if (ch <= 0x7ff) {
        *utf8_text++ = (char) (0xc0 | (ch >> 6));
        *utf8_text++ = (char) (0x80 | (ch & 0x3f));
    } else if (ch <= 0xffff) {
        *utf8_text++ = (char) (0xe0 | (ch >> 12));
        *utf8_text++ = (char) (0x80 | ((ch >> 6) & 0x3f));
        *utf8_text++ = (char) (0x80 | (ch & 0x3f));
    } else {
        unsigned val = ch - 0x10000;
        unsigned u = ((val & 0xf0000) >> 16) + 1, z = (val & 0x0f000) >> 12, y =
            (val & 0x00fc0) >> 6, x = val & 0x0003f;
        *utf8_text++ = (char) (0xf0 | (u >> 2));
        *utf8_text++ = (char) (0x80 | ((u & 3) << 4) | z);
        *utf8_text++ = (char) (0x80 | y);
        *utf8_text++ = (char) (0x80 | x);
    }
    return (utf8_text);
}

static unsigned u_length(register unsigned int *str)
{
    register unsigned len = 0;
    while (*str++ != '\0')
        ++len;
    return (len);
}


static void utf82u_strcpy(unsigned int *ubuf, const char *utf8buf)
{
    int len = (int) strlen(utf8buf) + 1;
    unsigned int *upt = ubuf, *uend = ubuf + len - 1;
    const unsigned char *pt = (const unsigned char *) utf8buf, *end =
        pt + strlen(utf8buf);
    int w, w2;

    while (pt < end && *pt != '\0' && upt < uend) {
        if (*pt <= 127)
            *upt = *pt++;
        else if (*pt <= 0xdf) {
            *upt = (unsigned int) (((*pt & 0x1f) << 6) | (pt[1] & 0x3f));
            pt += 2;
        } else if (*pt <= 0xef) {
            *upt =
                (unsigned int) (((*pt & 0xf) << 12) | ((pt[1] & 0x3f) << 6) |
                                (pt[2] & 0x3f));
            pt += 3;
        } else {
            w = (((*pt & 0x7) << 2) | ((pt[1] & 0x30) >> 4)) - 1;
            w = (w << 6) | ((pt[1] & 0xf) << 2) | ((pt[2] & 0x30) >> 4);
            w2 = ((pt[2] & 0xf) << 6) | (pt[3] & 0x3f);
            *upt = (unsigned int) (w * 0x400 + w2 + 0x10000);
            pt += 4;
        }
        ++upt;
    }
    *upt = '\0';
}

@ @c
#define noVERBOSE

#define MAX_TEX_LANGUAGES  32768

static struct tex_language *tex_languages[MAX_TEX_LANGUAGES] = { NULL };

static int next_lang_id = 0;

struct tex_language *new_language(int n)
{
    struct tex_language *lang;
    unsigned l;
    if (n >= 0) {
        l = (unsigned) n;
        if (l != (MAX_TEX_LANGUAGES - 1))
            if (next_lang_id <= n)
                next_lang_id = n + 1;
    } else {
        while (tex_languages[next_lang_id] != NULL)
            next_lang_id++;
        l = (unsigned) next_lang_id++;
    }
    if (l < (MAX_TEX_LANGUAGES - 1) && tex_languages[l] == NULL) {
        lang = xmalloc(sizeof(struct tex_language));
        tex_languages[l] = lang;
        lang->id = (int) l;
        lang->exceptions = 0;
        lang->patterns = NULL;
        lang->pre_hyphen_char = '-';
        lang->post_hyphen_char = 0;
        lang->pre_exhyphen_char = 0;
        lang->post_exhyphen_char = 0;
        return lang;
    } else {
        return NULL;
    }
}

struct tex_language *get_language(int n)
{
    if (n >= 0 && n < MAX_TEX_LANGUAGES) {
        if (tex_languages[n] != NULL) {
            return tex_languages[n];
        } else {
            return new_language(n);
        }
    } else {
        return NULL;
    }
}

@ @c
void set_pre_hyphen_char(int n, int v)
{
    struct tex_language *l = get_language((int) n);
    if (l != NULL)
        l->pre_hyphen_char = (int) v;
}

void set_post_hyphen_char(int n, int v)
{
    struct tex_language *l = get_language((int) n);
    if (l != NULL)
        l->post_hyphen_char = (int) v;
}


void set_pre_exhyphen_char(int n, int v)
{
    struct tex_language *l = get_language((int) n);
    if (l != NULL)
        l->pre_exhyphen_char = (int) v;
}

void set_post_exhyphen_char(int n, int v)
{
    struct tex_language *l = get_language((int) n);
    if (l != NULL)
        l->post_exhyphen_char = (int) v;
}


int get_pre_hyphen_char(int n)
{
    struct tex_language *l = get_language((int) n);
    if (l == NULL)
        return -1;
    return (int) l->pre_hyphen_char;
}

int get_post_hyphen_char(int n)
{
    struct tex_language *l = get_language((int) n);
    if (l == NULL)
        return -1;
    return (int) l->post_hyphen_char;
}


int get_pre_exhyphen_char(int n)
{
    struct tex_language *l = get_language((int) n);
    if (l == NULL)
        return -1;
    return (int) l->pre_exhyphen_char;
}

int get_post_exhyphen_char(int n)
{
    struct tex_language *l = get_language((int) n);
    if (l == NULL)
        return -1;
    return (int) l->post_exhyphen_char;
}

@ @c
void load_patterns(struct tex_language *lang, const unsigned char *buffer)
{
    if (lang == NULL || buffer == NULL || strlen((const char *) buffer) == 0)
        return;
    if (lang->patterns == NULL) {
        lang->patterns = hnj_hyphen_new();
    }
    hnj_hyphen_load(lang->patterns, buffer);
}

void clear_patterns(struct tex_language *lang)
{
    if (lang == NULL)
        return;
    if (lang->patterns != NULL) {
        hnj_hyphen_clear(lang->patterns);
    }
}

void load_tex_patterns(int curlang, halfword head)
{
    char *s = tokenlist_to_cstring(head, 1, NULL);
    load_patterns(get_language(curlang), (unsigned char *) s);
}


@ @c
#define STORE_CHAR(x) do {                          \
        word[w] = (unsigned char)x;                 \
        if (w<MAX_WORD_LEN) w++;                    \
    } while (0)

/* todo change this! */

const char *clean_hyphenation(const char *buffer, char **cleaned)
{
    int items;
    unsigned char word[MAX_WORD_LEN + 1];
    int w = 0;
    const char *s = buffer;
    while (*s && !isspace(*s)) {
        if (*s == '-') {        /* skip */
        } else if (*s == '=') {
            STORE_CHAR('-');
        } else if (*s == '{') {
            s++;
            items = 0;
            while (*s && *s != '}') {
                s++;
            }
            if (*s == '}') {
                items++;
                s++;
            }
            while (*s && *s != '}') {
                s++;
            }
            if (*s == '}') {
                items++;
                s++;
            }
            if (*s == '{') {
                s++;
            }
            while (*s && *s != '}') {
                STORE_CHAR(*s);
                s++;
            }
            if (*s == '}') {
                items++;
            } else {
                s--;
            }
            if (items != 3) {   /* syntax error */
                *cleaned = NULL;
                while (*s && !isspace(*s)) {
                    s++;
                }
                return s;
            }
        } else {
            STORE_CHAR(*s);
        }
        s++;
    }
    word[w] = 0;
    *cleaned = xstrdup((char *) word);
    return s;
}

@ @c
void load_hyphenation(struct tex_language *lang, const unsigned char *buffer)
{
    const char *s;
    const char *value;
    char *cleaned;
    lua_State *L = Luas;
    if (lang == NULL)
        return;
    if (lang->exceptions == 0) {
        lua_newtable(L);
        lang->exceptions = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, lang->exceptions);
    s = (const char *) buffer;
    while (*s) {
        while (isspace(*s))
            s++;
        if (*s) {
            value = s;
            s = clean_hyphenation(s, &cleaned);
            if (cleaned != NULL) {
                if ((s - value) > 0) {
                    lua_pushstring(L, cleaned);
                    lua_pushlstring(L, value, (size_t) (s - value));
                    lua_rawset(L, -3);
                }
                free(cleaned);
            } else {
#ifdef VERBOSE
                fprintf(stderr, "skipping invalid hyphenation exception: %s\n",
                        value);
#endif
            }
        }
    }
}

void clear_hyphenation(struct tex_language *lang)
{
    if (lang == NULL)
        return;
    if (lang->exceptions != 0) {
        luaL_unref(Luas, LUA_REGISTRYINDEX, lang->exceptions);
        lang->exceptions = 0;
    }
}


void load_tex_hyphenation(int curlang, halfword head)
{
    char *s = tokenlist_to_cstring(head, 1, NULL);
    load_hyphenation(get_language(curlang), (unsigned char *) s);
}

@ TODO: clean this up. The |delete_attribute_ref()| statements are not very 
   nice, but needed. Also, in the post-break, it would be nicer to get the 
   attribute list from |vlink(n)|. No rush, as it is currently not used much. 

@c
halfword insert_discretionary(halfword t, halfword pre, halfword post,
                              halfword replace)
{
    halfword g, n;
    int f;
    n = new_node(disc_node, syllable_disc);
    try_couple_nodes(n, vlink(t));
    couple_nodes(t, n);
    if (replace != null)
        f = font(replace);
    else
        f = get_cur_font();     /* for compound words following explicit hyphens */
    for (g = pre; g != null; g = vlink(g)) {
        font(g) = f;
        if (node_attr(t) != null) {
            delete_attribute_ref(node_attr(g));
            node_attr(g) = node_attr(t);
            attr_list_ref(node_attr(t)) += 1;
        }
    }
    for (g = post; g != null; g = vlink(g)) {
        font(g) = f;
        if (node_attr(t) != null) {
            delete_attribute_ref(node_attr(g));
            node_attr(g) = node_attr(t);
            attr_list_ref(node_attr(t)) += 1;
        }
    }
    for (g = replace; g != null; g = vlink(g)) {
        if (node_attr(t) != null) {
            delete_attribute_ref(node_attr(g));
            node_attr(g) = node_attr(t);
            attr_list_ref(node_attr(t)) += 1;
        }
    }
    if (node_attr(t) != null) {
        delete_attribute_ref(node_attr(vlink(t)));
        node_attr(vlink(t)) = node_attr(t);
        attr_list_ref(node_attr(t)) += 1;
    }
    t = vlink(t);
    set_disc_field(pre_break(t), pre);
    set_disc_field(post_break(t), post);
    set_disc_field(no_break(t), replace);
    return t;
}

halfword insert_syllable_discretionary(halfword t, lang_variables * lan)
{
    halfword g, n;
    n = new_node(disc_node, syllable_disc);
    couple_nodes(n, vlink(t));
    couple_nodes(t, n);
    delete_attribute_ref(node_attr(n));
    if (node_attr(t) != null) {
        node_attr(n) = node_attr(t);
        attr_list_ref(node_attr(t))++;
    } else {
        node_attr(n) = null;
    }
    if (lan->pre_hyphen_char > 0) {
        g = raw_glyph_node();
        set_to_character(g);
        character(g) = lan->pre_hyphen_char;
        font(g) = font(t);
        lang_data(g) = lang_data(t);
        if (node_attr(t) != null) {
            node_attr(g) = node_attr(t);
            attr_list_ref(node_attr(t))++;
        }
        set_disc_field(pre_break(n), g);
    }

    if (lan->post_hyphen_char > 0) {
        t = vlink(n);
        g = raw_glyph_node();
        set_to_character(g);
        character(g) = lan->post_hyphen_char;
        font(g) = font(t);
        lang_data(g) = lang_data(t);
        if (node_attr(t) != null) {
            node_attr(g) = node_attr(t);
            attr_list_ref(node_attr(t)) += 1;
        }
        set_disc_field(post_break(n), g);
    }
    return n;
}

halfword insert_word_discretionary(halfword t, lang_variables * lan)
{
    halfword pre = null, pos = null;
    if (lan->pre_exhyphen_char > 0)
        pre = insert_character(null, lan->pre_exhyphen_char);
    if (lan->post_exhyphen_char > 0)
        pos = insert_character(null, lan->post_exhyphen_char);
    return insert_discretionary(t, pre, pos, null);
}

@ @c
halfword compound_word_break(halfword t, int clang)
{
    int disc;
    lang_variables langdata;
    langdata.pre_exhyphen_char = get_pre_exhyphen_char(clang);
    langdata.post_exhyphen_char = get_post_exhyphen_char(clang);
    disc = insert_word_discretionary(t, &langdata);
    return disc;
}


halfword insert_complex_discretionary(halfword t, lang_variables * lan,
                                      halfword pre, halfword pos,
                                      halfword replace)
{
    (void) lan;
    return insert_discretionary(t, pre, pos, replace);
}


halfword insert_character(halfword t, int c)
{
    halfword p;
    p = new_node(glyph_node, 0);
    set_to_character(p);
    character(p) = c;
    if (t != null) {
        couple_nodes(t, p);
    }
    return p;
}

@ @c
void set_disc_field(halfword f, halfword t)
{
    if (t != null) {
        couple_nodes(f, t);
        tlink(f) = tail_of_list(t);
    } else {
        vlink(f) = null;
        tlink(f) = null;
    }
}



@ @c
static char *hyphenation_exception(int exceptions, char *w)
{
    char *ret = NULL;
    lua_State *L = Luas;
    lua_checkstack(L, 2);
    lua_rawgeti(L, LUA_REGISTRYINDEX, exceptions);
    if (lua_istable(L, -1)) {   /* ?? */
        lua_pushstring(L, w);   /* word table */
        lua_rawget(L, -2);
        if (lua_isstring(L, -1)) {
            ret = xstrdup(lua_tostring(L, -1));
        }
        lua_pop(L, 2);
    } else {
        lua_pop(L, 1);
    }
    return ret;
}


@ @c
char *exception_strings(struct tex_language *lang)
{
    const char *value;
    size_t size = 0, current = 0;
    size_t l = 0;
    char *ret = NULL;
    lua_State *L = Luas;
    if (lang->exceptions == 0)
        return NULL;
    lua_checkstack(L, 2);
    lua_rawgeti(L, LUA_REGISTRYINDEX, lang->exceptions);
    if (lua_istable(L, -1)) {
        /* iterate and join */
        lua_pushnil(L);         /* first key */
        while (lua_next(L, -2) != 0) {
            value = lua_tolstring(L, -1, &l);
            if (current + 2 + l > size) {
                ret =
                    xrealloc(ret,
                             (unsigned) ((size + size / 5) + current + l +
                                         1024));
                size = (size + size / 5) + current + l + 1024;
            }
            *(ret + current) = ' ';
            strcpy(ret + current + 1, value);
            current += l + 1;
            lua_pop(L, 1);
        }
    }
    return ret;
}


@ the sequence from |wordstart| to |r| can contain only normal characters 
it could be faster to modify a halfword pointer and return an integer 

@c
static halfword find_exception_part(unsigned int *j, unsigned int *uword, int len)
{
    halfword g = null, gg = null;
    register unsigned i = *j;
    i++;                        /* this puts uword[i] on the |{| */
    while (i < (unsigned) len && uword[i + 1] != '}') {
        if (g == null) {
            gg = new_char(0, (int) uword[i + 1]);
            g = gg;
        } else {
            halfword s = new_char(0, (int) uword[i + 1]);
            couple_nodes(g, s);
            g = vlink(g);
        }
        i++;
    }
    *j = ++i;
    return gg;
}

static int count_exception_part(unsigned int *j, unsigned int *uword, int len)
{
    int ret = 0;
    register unsigned i = *j;
    i++;                        /* this puts uword[i] on the |{| */
    while (i < (unsigned) len && uword[i + 1] != '}') {
        ret++;
        i++;
    }
    *j = ++i;
    return ret;
}


@ @c
static const char *PAT_ERROR[] = {
    "Exception discretionaries should contain three pairs of braced items.",
    "No intervening spaces are allowed.",
    NULL
};

static void do_exception(halfword wordstart, halfword r, char *replacement)
{
    unsigned i;
    halfword t;
    unsigned len;
    int clang;
    lang_variables langdata;
    unsigned uword[MAX_WORD_LEN + 1] = { 0 };
    utf82u_strcpy(uword, replacement);
    len = u_length(uword);
    i = 0;
    t = wordstart;
    clang = char_lang(wordstart);
    langdata.pre_hyphen_char = get_pre_hyphen_char(clang);
    langdata.post_hyphen_char = get_post_hyphen_char(clang);

    for (i = 0; i < len; i++) {
        if (uword[i + 1] == '-') {      /* a hyphen follows */
            while (vlink(t) != r
                   && (type(t) != glyph_node || !is_simple_character(t)))
                t = vlink(t);
            if (vlink(t) == r)
                break;
            insert_syllable_discretionary(t, &langdata);
            t = vlink(t);       /* skip the new disc */
        } else if (uword[i + 1] == '=') {
            /* do nothing ? */
            t = vlink(t);
        } else if (uword[i + 1] == '{') {
            halfword gg, hh, replace = null;
            int repl;
            gg = find_exception_part(&i, uword, (int) len);
            if (i == len || uword[i + 1] != '{') {
                tex_error("broken pattern 1", PAT_ERROR);
            }
            hh = find_exception_part(&i, uword, (int) len);
            if (i == len || uword[i + 1] != '{') {
                tex_error("broken pattern 2", PAT_ERROR);
            }
            repl = count_exception_part(&i, uword, (int) len);
            if (i == len) {
                tex_error("broken pattern 3", PAT_ERROR);
            }
            /*i++;  *//* jump over the last right brace */
            if (vlink(t) == r)
                break;
            if (repl > 0) {
                halfword q = t;
                replace = vlink(q);
                while (repl > 0 && q != null) {
                    q = vlink(q);
                    if (type(q) == glyph_node) {
                        repl--;
                    }
                }
                try_couple_nodes(t, vlink(q));
                vlink(q) = null;
            }
            t = insert_discretionary(t, gg, hh, replace);
            t = vlink(t);       /* skip the new disc */
        } else {
            t = vlink(t);
        }
    }
}

@ This is a documentation section from the pascal web file. It is not 
true any more, but I do not have time right now to rewrite it -- Taco

When the line-breaking routine is unable to find a feasible sequence of
breakpoints, it makes a second pass over the paragraph, attempting to
hyphenate the hyphenatable words. The goal of hyphenation is to insert
discretionary material into the paragraph so that there are more
potential places to break.

The general rules for hyphenation are somewhat complex and technical,
because we want to be able to hyphenate words that are preceded or
followed by punctuation marks, and because we want the rules to work
for languages other than English. We also must contend with the fact
that hyphens might radically alter the ligature and kerning structure
of a word.

A sequence of characters will be considered for hyphenation only if it
belongs to a ``potentially hyphenatable part'' of the current paragraph.
This is a sequence of nodes $p_0p_1\ldots p_m$ where $p_0$ is a glue node,
$p_1\ldots p_{m-1}$ are either character or ligature or whatsit or
implicit kern nodes, and $p_m$ is a glue or penalty or insertion or adjust
or mark or whatsit or explicit kern node.  (Therefore hyphenation is
disabled by boxes, math formulas, and discretionary nodes already inserted
by the user.) The ligature nodes among $p_1\ldots p_{m-1}$ are effectively
expanded into the original non-ligature characters; the kern nodes and
whatsits are ignored. Each character |c| is now classified as either a
nonletter (if |lc_code(c)=0|), a lowercase letter (if
|lc_code(c)=c|), or an uppercase letter (otherwise); an uppercase letter
is treated as if it were |lc_code(c)| for purposes of hyphenation. The
characters generated by $p_1\ldots p_{m-1}$ may begin with nonletters; let
$c_1$ be the first letter that is not in the middle of a ligature. Whatsit
nodes preceding $c_1$ are ignored; a whatsit found after $c_1$ will be the
terminating node $p_m$. All characters that do not have the same font as
$c_1$ will be treated as nonletters. The |hyphen_char| for that font
must be between 0 and 255, otherwise hyphenation will not be attempted.
\TeX\ looks ahead for as many consecutive letters $c_1\ldots c_n$ as
possible; however, |n| must be less than 64, so a character that would
otherwise be $c_{64}$ is effectively not a letter. Furthermore $c_n$ must
not be in the middle of a ligature.  In this way we obtain a string of
letters $c_1\ldots c_n$ that are generated by nodes $p_a\ldots p_b$, where
|1<=a<=b+1<=m|. If |n>=l_hyf+r_hyf|, this string qualifies for hyphenation;
however, |uc_hyph| must be positive, if $c_1$ is uppercase.

The hyphenation process takes place in three stages. First, the candidate
sequence $c_1\ldots c_n$ is found; then potential positions for hyphens
are determined by referring to hyphenation tables; and finally, the nodes
$p_a\ldots p_b$ are replaced by a new sequence of nodes that includes the
discretionary breaks found.

Fortunately, we do not have to do all this calculation very often, because
of the way it has been taken out of \TeX's inner loop. For example, when
the second edition of the author's 700-page book {\sl Seminumerical
Algorithms} was typeset by \TeX, only about 1.2 hyphenations needed to be
@^Knuth, Donald Ervin@>
tried per paragraph, since the line breaking algorithm needed to use two
passes on only about 5 per cent of the paragraphs.


When a word been set up to contain a candidate for hyphenation,
\TeX\ first looks to see if it is in the user's exception dictionary. If not,
hyphens are inserted based on patterns that appear within the given word,
using an algorithm due to Frank~M. Liang.
@^Liang, Franklin Mark@>


@ This is incompatible with TEX because the first word of a paragraph
can be hyphenated, but most european users seem to agree that
prohibiting hyphenation there was not the best idea ever.

@c
static halfword find_next_wordstart(halfword r)
{
    register int l;
    register int start_ok = 1;
    int mathlevel = 1;
    while (r != null) {
        switch (type(r)) {
        case whatsit_node:
            break;
        case glue_node:
            start_ok = 1;
            break;
        case math_node:
            while (mathlevel > 0) {
                r = vlink(r);
                if (r == null)
                    return r;
                if (type(r) == math_node) {
                    if (subtype(r) == before) {
                        mathlevel++;
                    } else {
                        mathlevel--;
                    }
                }
            }
            break;
        case glyph_node:
            if (start_ok &&
                is_simple_character(r) &&
                (l = get_lc_code(character(r))) > 0 &&
                (char_uchyph(r) || l == character(r)))
                return r;
            break;
        default:
            start_ok = 0;
            break;
        }
        r = vlink(r);
    }
    return r;
}

@ @c
static int valid_wordend(halfword s)
{
    register halfword r = s;
    register int clang = char_lang(s);
    if (r == null)
        return 1;
    while ((r != null) && ((type(r) == glyph_node && is_simple_character(r)
                            && clang == char_lang(r)) ||
                           (type(r) == kern_node && (subtype(r) == normal))
           )) {
        r = vlink(r);
    }
    if (r == null || (type(r) == glyph_node && is_simple_character(r)
                      && clang != char_lang(r)) || type(r) == glue_node
        || type(r) == whatsit_node || type(r) == ins_node
        || type(r) == adjust_node || type(r) == penalty_node
        || (type(r) == kern_node
            && (subtype(r) == explicit || subtype(r) == acc_kern)))
        return 1;
    return 0;
}

@ @c
void hnj_hyphenation(halfword head, halfword tail)
{
    int lchar, i;
    struct tex_language *lang;
    lang_variables langdata;
    char utf8word[(4 * MAX_WORD_LEN) + 1] = { 0 };
    int wordlen = 0;
    char *hy = utf8word;
    char *replacement = NULL;
    boolean explicit_hyphen = false;
    halfword s, r = head, wordstart = null, save_tail = null, left =
        null, right = null;

    /* this first movement assures two things: 
     \item{a)} that we won't waste lots of time on something that has been
      handled already (in that case, none of the glyphs match |simple_character|).  
     \item{b)} that the first word can be hyphenated. if the movement was
     not explicit, then the indentation at the start of a paragraph
     list would make |find_next_wordstart()| look too far ahead.
     */

    while (r != null && (type(r) != glyph_node || !is_simple_character(r))) {
        r = vlink(r);
    }
    /* this will make |r| a glyph node with subtype character */
    r = find_next_wordstart(r);
    if (r == null)
        return;

    assert(tail != null);
    save_tail = vlink(tail);
    s = new_penalty(0);
    couple_nodes(tail, s);

    while (r != null) {         /* could be while(1), but let's be paranoid */
        int clang, lhmin, rhmin;
        halfword hyf_font;
        halfword end_word = r;
        wordstart = r;
        assert(is_simple_character(wordstart));
        hyf_font = font(wordstart);
        if (hyphen_char(hyf_font) < 0)  /* for backward compat */
            hyf_font = 0;
        clang = char_lang(wordstart);
        lhmin = char_lhmin(wordstart);
        rhmin = char_rhmin(wordstart);
        langdata.pre_hyphen_char = get_pre_hyphen_char(clang);
        langdata.post_hyphen_char = get_post_hyphen_char(clang);
        while (r != null &&
               type(r) == glyph_node &&
               is_simple_character(r) &&
               clang == char_lang(r) &&
               (((lchar = get_lc_code(character(r))) > 0)
                ||
                (character(r) == ex_hyphen_char && (lchar = ex_hyphen_char)))) {
            if (character(r) == ex_hyphen_char)
    	        explicit_hyphen = true;
            wordlen++;
            hy = uni2string(hy, (unsigned) character(r));
            /* this should not be needed  any more */
            /*if (vlink(r)!=null) alink(vlink(r))=r; */
            end_word = r;
            r = vlink(r);
        }
        if (valid_wordend(r) && wordlen >= lhmin + rhmin
            && (hyf_font != 0) && (lang = tex_languages[clang]) != NULL) {
            *hy = 0;
            if (lang->exceptions != 0 &&
                (replacement =
                 hyphenation_exception(lang->exceptions, utf8word)) != NULL) {
#ifdef VERBOSE
                fprintf(stderr, "replacing %s (c=%d) by %s\n", utf8word, clang,
                        replacement);
#endif
                do_exception(wordstart, r, replacement);
                free(replacement);
            } else if (explicit_hyphen == true) {
                /* insert an explicit discretionary after each of the last in a 
	           set of explicit hyphens */
                halfword rr = r;
                halfword t = null;
#ifdef VERBOSE
                fprintf(stderr, "explicit hyphen(s) found in %s (c=%d)\n", utf8word, clang);
#endif
                while (rr != wordstart) {
	            if (is_simple_character(rr)) {
                        if (character(rr) == ex_hyphen_char) {
                            t = compound_word_break(rr, clang);
                            subtype(t) = automatic_disc;
	                    while(character(alink(rr)) == ex_hyphen_char) 
	                       rr = alink(rr);
                        }
                    }
                    rr = alink(rr);
                }


            } else if (lang->patterns != NULL) {

                left = wordstart;
                for (i = lhmin; i > 1; i--) {
                    left = vlink(left);
                    while (!is_simple_character(left))
                        left = vlink(left);
                }
                right = r;
                for (i = rhmin; i > 0; i--) {
                    right = alink(right);
                    while (!is_simple_character(right))
                        right = alink(right);
                }

#ifdef VERBOSE
                fprintf(stderr, "hyphenate %s (c=%d,l=%d,r=%d) from %c to %c\n",
                        utf8word, clang, lhmin, rhmin, character(left),
                        character(right));
#endif
                (void) hnj_hyphen_hyphenate(lang->patterns, wordstart, end_word,
                                            wordlen, left, right, &langdata);
            }
        }
        wordlen = 0;
        hy = utf8word;
        if (r == null)
            break;
        r = find_next_wordstart(r);
    }
    flush_node(vlink(tail));
    vlink(tail) = save_tail;
}


@ @c
void new_hyphenation(halfword head, halfword tail)
{
    register int callback_id = 0;
    if (head == null || vlink(head) == null)
        return;
    fix_node_list(head);
    callback_id = callback_defined(hyphenate_callback);
    if (callback_id > 0) {
        lua_State *L = Luas;
        if (!get_callback(L, callback_id)) {
            lua_pop(L, 2);
            return;
        }
        nodelist_to_lua(L, head);
        nodelist_to_lua(L, tail);
        if (lua_pcall(L, 2, 0, 0) != 0) {
            fprintf(stdout, "error: %s\n", lua_tostring(L, -1));
            lua_pop(L, 2);
            lua_error(L);
            return;
        }
        lua_pop(L, 1);
    } else if (callback_id == 0) {
        hnj_hyphenation(head, tail);
    }
}

@ dumping and undumping languages

@c
#define dump_string(a)                          \
  if (a!=NULL) {                                \
      x = (int)strlen(a)+1;                     \
    dump_int(x);  dump_things(*a, x);           \
  } else {                                      \
    x = 0; dump_int(x);                         \
  }


static void dump_one_language(int i)
{
    char *s = NULL;
    int x = 0;
    struct tex_language *lang;
    lang = tex_languages[i];
    dump_int(lang->id);
    dump_int(lang->pre_hyphen_char);
    dump_int(lang->post_hyphen_char);
    dump_int(lang->pre_exhyphen_char);
    dump_int(lang->post_exhyphen_char);
    if (lang->patterns != NULL) {
        s = (char *) hnj_serialize(lang->patterns);
    }
    dump_string(s);
    if (s != NULL) {
        free(s);
        s = NULL;
    }
    if (lang->exceptions != 0)
        s = exception_strings(lang);
    dump_string(s);
    if (s != NULL) {
        free(s);
    }
    free(lang);
}

void dump_language_data(void)
{
    int i;
    dump_int(next_lang_id);
    for (i = 0; i < next_lang_id; i++) {
        if (tex_languages[i]) {
            dump_int(1);
            dump_one_language(i);
        } else {
            dump_int(0);
        }
    }
}


static void undump_one_language(int i)
{
    char *s = NULL;
    int x = 0;
    struct tex_language *lang = get_language(i);
    undump_int(x);
    lang->id = x;
    undump_int(x);
    lang->pre_hyphen_char = x;
    undump_int(x);
    lang->post_hyphen_char = x;
    undump_int(x);
    lang->pre_exhyphen_char = x;
    undump_int(x);
    lang->post_exhyphen_char = x;
    /* patterns */
    undump_int(x);
    if (x > 0) {
        s = xmalloc((unsigned) x);
        undump_things(*s, x);
        load_patterns(lang, (unsigned char *) s);
        free(s);
    }
    /* exceptions */
    undump_int(x);
    if (x > 0) {
        s = xmalloc((unsigned) x);
        undump_things(*s, x);
        load_hyphenation(lang, (unsigned char *) s);
        free(s);
    }
}

void undump_language_data(void)
{
    int i, x, numlangs;
    undump_int(numlangs);
    next_lang_id = numlangs;
    for (i = 0; i < numlangs; i++) {
        undump_int(x);
        if (x == 1) {
            undump_one_language(i);
        }
    }
}


@ When \TeX\ has scanned `\.{\\hyphenation}', it calls on a procedure named
|new_hyph_exceptions| to do the right thing.

@c
void new_hyph_exceptions(void)
{                               /* enters new exceptions */
    (void) scan_toks(false, true);
    load_tex_hyphenation(int_par(language_code), def_ref);
    flush_list(def_ref);
}

@ Similarly, when \TeX\ has scanned `\.{\\patterns}', it calls on a
procedure named |new_patterns|.

@c
void new_patterns(void)
{                               /* initializes the hyphenation pattern data */
    (void) scan_toks(false, true);
    load_tex_patterns(int_par(language_code), def_ref);
    flush_list(def_ref);
}

@ `\.{\\prehyphenchar}', sets the |pre_break| character, and
`\.{\\posthyphenchar}' the |post_break| character. Their respective
defaults are ascii hyphen ("-") and zero (nul).

@c
void new_pre_hyphen_char(void)
{
    scan_optional_equals();
    scan_int();
    set_pre_hyphen_char(int_par(language_code), cur_val);
}

void new_post_hyphen_char(void)
{
    scan_optional_equals();
    scan_int();
    set_post_hyphen_char(int_par(language_code), cur_val);
}


@ `\.{\\preexhyphenchar}', sets the |pre_break| character, and
`\.{\\postexhyphenchar}' the |post_break| character. Their
defaults are both zero (nul).

@c
void new_pre_exhyphen_char(void)
{
    scan_optional_equals();
    scan_int();
    set_pre_exhyphen_char(int_par(language_code), cur_val);
}

void new_post_exhyphen_char(void)
{
    scan_optional_equals();
    scan_int();
    set_post_exhyphen_char(int_par(language_code), cur_val);
}
