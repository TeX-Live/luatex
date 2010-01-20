/* stringpool.c
   
   Copyright 2009 Taco Hoekwater <taco@luatex.org>

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

static const char _svn_version[] =
    "$Id$ $URL$";

/*
Control sequence names and diagnostic messages are variable-length strings
of eight-bit characters. Since \PASCAL\ did not have a well-developed string
mechanism, \TeX\ did all of its string processing by homegrown methods.

Elaborate facilities for dynamic strings are not needed, so all of the
necessary operations can be handled with a simple data structure.
The array |str_pool| contains all of the (eight-bit) bytes off all
of the strings, and the array |str_start| contains indices of the starting
points of each string. Strings are referred to by integer numbers, so that
string number |s| comprises the characters |str_pool[j]| for
|str_start_macro(s)<=j<str_start_macro(s+1)|. Additional integer variables
|pool_ptr| and |str_ptr| indicate the number of entries used so far
in |str_pool| and |str_start|, respectively; locations
|str_pool[pool_ptr]| and |str_start_macro(str_ptr)| are
ready for the next string to be allocated.

String numbers 0 to |biggest_char| are reserved for strings that correspond to 
single UNICODE characters. This is in accordance with the conventions of \.{WEB}
which converts single-character strings into the ASCII code number of the
single character involved, while it converts other strings into integers
and builds a string pool file. Thus, when the string constant \.{"."} appears
in the \PASCAL\ program, \.{WEB} converts it into the integer 46, which is the
ASCII code for a period, while \.{WEB} will convert a string like \.{"hello"}
into some integer greater than~|STRING_OFFSET|.
*/

lstring *string_pool;           /* the array of strings */
lstring *_string_pool;          /* this variable lives STRING_OFFSET below |string_pool| 
                                   (handy for debugging: 
                                   |_string_pool[str_ptr] == str_string(str_ptr)| */

str_number str_ptr = (STRING_OFFSET + 1);       /* number of the current string being created */
str_number init_str_ptr;        /* the starting value of |str_ptr| */

unsigned char *cur_string;      /*  current string buffer */
unsigned cur_length;            /* current index in that buffer */
unsigned cur_string_size;       /*  malloced size of |cur_string| */
unsigned pool_size;             /* occupied byte count */

/*
Once a sequence of characters has been appended to |cur_string|, it
officially becomes a string when the function |make_string| is called.
This function returns the identification number of the new string as its
value.
*/

void reset_cur_string(void)
{
    cur_length = 0;
    cur_string_size = 255;
    cur_string = (unsigned char *) xmalloc(256);
    memset(cur_string, 0, 256);
}

/* current string enters the pool */
str_number make_string(void)
{
    if (str_ptr == (max_strings + STRING_OFFSET))
        overflow("number of strings",
                 max_strings - init_str_ptr + STRING_OFFSET);
    str_room(1);
    cur_string[cur_length] = '\0';      /* now |lstring.s| is always a valid C string */
    str_string(str_ptr) = (unsigned char *) cur_string;
    str_length(str_ptr) = cur_length;
    pool_size += cur_length;
    reset_cur_string();
    /* printf("Made a string: %s (s=%d)\n", (char *)str_string(str_ptr), (int)str_ptr); */
    str_ptr++;
    return (str_ptr - 1);
}


static void utf_error(void)
{
    char *hlp[] = { "A funny symbol that I can't read has just been (re)read.",
        "Just continue, I'll change it to 0xFFFD.",
        NULL
    };
    deletions_allowed = false;
    tex_error("String contains an invalid utf-8 sequence", hlp);
    deletions_allowed = true;
}

unsigned str2uni(unsigned char *k)
{
    register int ch;
    unsigned val = 0xFFFD;
    unsigned char *text = k;
    if ((ch = *text++) < 0x80) {
        val = ch;
    } else if (ch <= 0xbf) {    /* error */
    } else if (ch <= 0xdf) {
        if (*text >= 0x80 && *text < 0xc0)
            val = ((ch & 0x1f) << 6) | (*text++ & 0x3f);
    } else if (ch <= 0xef) {
        if (*text >= 0x80 && *text < 0xc0 && text[1] >= 0x80 && text[1] < 0xc0) {
            val =
                ((ch & 0xf) << 12) | ((text[0] & 0x3f) << 6) | (text[1] & 0x3f);
        }
    } else {
        int w = (((ch & 0x7) << 2) | ((text[0] & 0x30) >> 4)) - 1, w2;
        w = (w << 6) | ((text[0] & 0xf) << 2) | ((text[1] & 0x30) >> 4);
        w2 = ((text[1] & 0xf) << 6) | (text[2] & 0x3f);
        val = w * 0x400 + w2 + 0x10000;
        if (*text < 0x80 || text[1] < 0x80 || text[2] < 0x80 ||
            *text >= 0xc0 || text[1] >= 0xc0 || text[2] >= 0xc0)
            val = 0xFFFD;
    }
    if (val == 0xFFFD)
        utf_error();
    return (val);
}

/* This is a very basic helper */

unsigned char *uni2str(unsigned unic)
{
    unsigned char *buf = xmalloc(5);
    unsigned char *pt = buf;
    if (unic < 0x80)
        *pt++ = (unsigned char)unic;
    else if (unic < 0x800) {
        *pt++ = (unsigned char)(0xc0 | (unic >> 6));
        *pt++ = (unsigned char)(0x80 | (unic & 0x3f));
    } else if (unic >= 0x110000) {
        *pt++ = (unsigned char)(unic - 0x110000);
    } else if (unic < 0x10000) {
        *pt++ = (unsigned char)(0xe0 | (unic >> 12));
        *pt++ = (unsigned char)(0x80 | ((unic >> 6) & 0x3f));
        *pt++ = (unsigned char)(0x80 | (unic & 0x3f));
    } else {
        int u, z, y, x;
        unsigned val = unic - 0x10000;
        u = ((val & 0xf0000) >> 16) + 1;
        z = (val & 0x0f000) >> 12;
        y = (val & 0x00fc0) >> 6;
        x = val & 0x0003f;
        *pt++ = (unsigned char)(0xf0 | (u >> 2));
        *pt++ = (unsigned char)(0x80 | ((u & 3) << 4) | z);
        *pt++ = (unsigned char)(0x80 | y);
        *pt++ = (unsigned char)(0x80 | x);
    }
    *pt = '\0';
    return buf;
}

/*
|buffer_to_unichar| converts a sequence of bytes in the |buffer|
into a unicode character value. It does not check for overflow
of the |buffer|, but it is careful to check the validity of the 
UTF-8 encoding.
*/

#define test_sequence_byte(A) do {                      \
        if (((A)<0x80) || ((A)>=0xC0)) {                \
            utf_error();                                \
            return 0xFFFD;                              \
        }                                               \
  } while (0)


static int buffer_to_unichar(int k)
{
    int a;                      /* a utf char */
    int b;                      /* a utf nibble */
    b = buffer[k];
    if (b < 0x80) {
        a = b;
    } else if (b >= 0xF8) {
        /* the 5- and 6-byte UTF-8 sequences generate integers 
           that are outside of the valid UCS range, and therefore
           unsupported 
         */
        test_sequence_byte(-1);
    } else if (b >= 0xF0) {
        a = (b - 0xF0) * 64;
        b = buffer[k + 1];
        test_sequence_byte(b);
        a = (a + (b - 128)) * 64;
        b = buffer[k + 2];
        test_sequence_byte(b);
        a = (a + (b - 128)) * 64;
        b = buffer[k + 3];
        test_sequence_byte(b);
        a = a + (b - 128);
    } else if (b >= 0xE0) {
        a = (b - 0xE0) * 64;
        b = buffer[k + 1];
        test_sequence_byte(b);
        a = (a + (b - 128)) * 64;
        b = buffer[k + 2];
        test_sequence_byte(b);
        a = a + (b - 128);
    } else if (b >= 0xC0) {
        a = (b - 0xC0) * 64;
        b = buffer[k + 1];
        test_sequence_byte(b);
        a = a + (b - 128);
    } else {
        /* This is an encoding error */
        test_sequence_byte(-1);
    }
    return a;
}

int pool_to_unichar(unsigned char *t)
{
    return (int) str2uni(t);
}


/*
The following subroutine compares string |s| with another string of the
same length that appears in |buffer| starting at position |k|;
the result is |true| if and only if the strings are equal.
Empirical tests indicate that |str_eq_buf| is used in such a way that
it tends to return |true| about 80 percent of the time.
*/

boolean str_eq_buf(str_number s, int k)
{                               /* test equality of strings */
    int a;                      /* a unicode character */
    if (s < STRING_OFFSET) {
        a = buffer_to_unichar(k);
        if (a != s)
            return false;
    } else {
        unsigned char *j = str_string(s);
        unsigned char *l = j + str_length(s);
        while (j < l) {
            if (*j++ != buffer[k++])
                return false;
        }
    }
    return true;
}

/*
Here is a similar routine, but it compares two strings in the string pool,
and it does not assume that they have the same length.
*/

boolean str_eq_str(str_number s, str_number t)
{                               /* test equality of strings */
    int a = 0;                  /* a utf char */
    unsigned char *j, *k, *l;   /* running indices */
    if (s < STRING_OFFSET) {
        if (t >= STRING_OFFSET) {
            k = str_string(t);
            if (s <= 0x7F && (str_length(t) == 1) && *k == s)
                return true;
            a = pool_to_unichar(k);
            if (a != s)
                return false;
        } else {
            if (t != s)
                return false;
        }
    } else if (t < STRING_OFFSET) {
        j = str_string(s);
        if (t <= 0x7F && (str_length(s) == 1) && *j == t)
            return true;
        a = pool_to_unichar(j);
        if (a != t)
            return false;
    } else {
        if (str_length(s) != str_length(t))
            return false;
        k = str_string(t);
        j = str_string(s);
        l = j + str_length(s);
        while (j < l) {
            if (*j++ != *k++)
                return false;
        }
    }
    return true;
}

/* string compare */

boolean str_eq_cstr(str_number r, char *s, size_t l)
{
    if (l != (size_t) str_length(r))
        return false;
    return (strncmp((const char *) (str_string(r)), s, l) == 0);
}

/*
The initial values of |str_pool|, |str_start|, |pool_ptr|,
and |str_ptr| are computed by the \.{INITEX} program, based in part
on the information that \.{WEB} has output while processing \TeX.

The first |string_offset| strings are single-characters strings matching
Unicode. There is no point in generating all of these. But |str_ptr| has
initialized properly, otherwise |print_char| cannot see the difference
between characters and strings.
*/




/* initializes the string pool, but returns |false| if something goes wrong */
boolean get_strings_started(void)
{
    reset_cur_string();
    return true;
}

/* Declare additional routines for string recycling */
/* from the change file */

/* The string recycling routines.  \TeX{} uses 2
   upto 4 {\it new\/} strings when scanning a filename in an \.{\\input},
   \.{\\openin}, or \.{\\openout} operation.  These strings are normally
   lost because the reference to them are not saved after finishing the
   operation.  |search_string| searches through the string pool for the
   given string and returns either 0 or the found string number.
*/

str_number search_string(str_number search)
{
    str_number s;               /* running index */
    unsigned len;               /* length of searched string */
    len = str_length(search);
    if (len == 0) {
        return get_nullstr();
    } else {
        s = search - 1;         /* start search with newest string below |s|; |search>1|! */
        while (s >= STRING_OFFSET) {
            /* first |string_offset| strings depend on implementation!! */
            if (str_length(s) == len)
                if (str_eq_str(s, search))
                    return s;
            s--;
        }
    }
    return 0;
}

str_number maketexstring(const char *s)
{
    if (s == NULL || *s == 0)
        return get_nullstr();
    return maketexlstring(s, strlen(s));
}

str_number maketexlstring(const char *s, size_t l)
{
    if (s == NULL || l == 0)
        return get_nullstr();
    str_string(str_ptr) = xmalloc(l + 1);
    memcpy(str_string(str_ptr), s, (l + 1));
    str_length(str_ptr) = (unsigned) l;
    str_ptr++;
    return (str_ptr - 1);
}

/* append a C string to a TeX string */
void append_string(unsigned char *s, unsigned l)
{
    if (s == NULL || *s == 0)
        return;
    l = strlen((char *) s);
    str_room(l);
    memcpy(cur_string + cur_length, s, l);
    cur_length += l;
    return;
}

char *makecstring(int s)
{
    size_t l;
    return makeclstring(s, &l);
}

char *makeclstring(int s, size_t * len)
{
    if (s < STRING_OFFSET) {
        *len = utf8_size(s);
        return (char *) uni2str(s);
    } else {
        unsigned l = str_length(s);
        char *cstrbuf = xmalloc(l + 1);
        memcpy(cstrbuf, str_string(s), l);
        cstrbuf[l] = '\0';
        *len = l;
        return cstrbuf;
    }
}

int dump_string_pool(void)
{
    int j;
    int l;
    int k = str_ptr;
    dump_int(k - STRING_OFFSET);
    for (j = STRING_OFFSET + 1; j < k; j++) {
        l = (int)str_length(j);
        if (str_string(j) == NULL)
            l = -1;
        dump_int(l);
        if (l > 0)
            dump_things(*str_string(j), str_length(j));
    }
    return (k - STRING_OFFSET);
}

int undump_string_pool(void)
{
    int j;
    int x;
    undump_int(str_ptr);
    if (max_strings < str_ptr + strings_free)
        max_strings = str_ptr + strings_free;
    str_ptr += STRING_OFFSET;
    if (ini_version)
        libcfree(string_pool);
    init_string_pool_array(max_strings);
    for (j = STRING_OFFSET + 1; j < str_ptr; j++) {
        undump_int(x);
        if (x >= 0) {
            str_length(j) = (unsigned) x;
            pool_size += x;
            str_string(j) = xmallocarray(unsigned char, (unsigned) (x + 1));
            undump_things(*str_string(j), (unsigned) x);
            *(str_string(j) + str_length(j)) = '\0';
        } else {
            str_length(j) = 0;
        }
    }
    init_str_ptr = str_ptr;
    return str_ptr;
}

void init_string_pool_array(int s)
{
    string_pool = xmallocarray(lstring, s);
    _string_pool = string_pool - STRING_OFFSET;
    memset(string_pool, 0, s * sizeof(lstring));
    /* seed the null string */
    string_pool[0].s = xmalloc(1);
    string_pool[0].s[0] = '\0';
}

/* To destroy an already made string, we say |flush_str|. */

void flush_str(str_number s)
{
    /* printf("Flushing a string: %s (s=%d,str_ptr=%d)\n", (char *)str_string(s), (int)s, (int)str_ptr); */
    if (s > STRING_OFFSET) {    /* don't ever delete the null string */
        pool_size -= str_length(s);
        str_length(s) = 0;
        xfree(str_string(s));
        str_string(s) = NULL;
    }
    while (str_string((str_ptr - 1)) == NULL)
        str_ptr--;
}
