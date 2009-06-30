/* textoken.h
   
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

/* $Id$ */

#ifndef TEXTOKEN_H
#  define TEXTOKEN_H

#  define token_list 0
#  define null 0
#  define cs_token_flag 0x1FFFFFFF

#  define left_brace_token 0x200000     /* $2^{21}\cdot|left_brace|$ */
#  define right_brace_token 0x400000    /* $2^{21}\cdot|right_brace|$ */
#  define left_brace_token 0x200000     /* $2^{21}\cdot|left_brace|$ */
#  define left_brace_limit 0x400000     /* $2^{21}\cdot(|left_brace|+1)$ */
#  define right_brace_token 0x400000    /* $2^{21}\cdot|right_brace|$ */
#  define right_brace_limit 0x600000    /* $2^{21}\cdot(|right_brace|+1)$ */
#  define math_shift_token 0x600000     /* $2^{21}\cdot|math_shift|$ */
#  define tab_token 0x800000            /* $2^{21}\cdot|tab_mark|$ */
#  define out_param_token 0xA00000      /* $2^{21}\cdot|out_param|$ */
#  define space_token 0x1400020         /* $2^{21}\cdot|spacer|+|" "|$ */
#  define letter_token 0x1600000        /* $2^{21}\cdot|letter|$ */
#  define other_token 0x1800000         /* $2^{21}\cdot|other_char|$ */
#  define match_token 0x1A00000         /* $2^{21}\cdot|match|$ */
#  define end_match_token 0x1C00000     /* $2^{21}\cdot|end_match|$ */
#  define protected_token 0x1C00001     /* $2^{21}\cdot|end_match|+1$ */

typedef struct smemory_word_ {
    halfword hhrh;
    halfword hhlh;
} smemory_word;

#  define fix_mem_init 10000

extern smemory_word *fixmem;
extern halfword fix_mem_min;
extern halfword fix_mem_max;

extern halfword garbage;        /* head of a junk list, write only */
extern halfword temp_token_head;        /* head of a temporary list of some kind */
extern halfword hold_token_head;        /* head of a temporary list of another kind */
extern halfword omit_template;  /* a constant token list */
extern halfword null_list;      /* permanently empty list */
extern halfword backup_head;    /* head of token list built by |scan_keyword| */

extern void initialize_tokens(void);

extern integer dyn_used;

#  define token_info(a)    fixmem[(a)].hhlh
#  define token_link(a)    fixmem[(a)].hhrh
#  define set_token_info(a,b) fixmem[(a)].hhlh=(b)
#  define set_token_link(a,b) fixmem[(a)].hhrh=(b)

extern halfword avail;          /* head of the list of available one-word nodes */
extern halfword fix_mem_end;    /* the last one-word node used in |mem| */

extern halfword get_avail(void);

/* A one-word node is recycled by calling |free_avail|.
This routine is part of \TeX's ``inner loop,'' so we want it to be fast.
*/

#  define free_avail(A) do { /* single-word node liberation */	\
    token_link(A)=avail; avail=(A); decr(dyn_used);		\
  } while (0)

/*
There's also a |fast_get_avail| routine, which saves the procedure-call
overhead at the expense of extra programming. This routine is used in
the places that would otherwise account for the most calls of |get_avail|.
*/

#  define fast_get_avail(A) do {						\
    (A)=avail; /* avoid |get_avail| if possible, to save time */	\
    if ((A)==null)  { (A)=get_avail(); }				\
    else  { avail=token_link((A)); token_link((A))=null; incr(dyn_used); } \
  } while (0)

extern void print_meaning (void);

extern void flush_list(halfword p);
extern void show_token_list(integer p, integer q, integer l);
extern void token_show(halfword p);

#  define token_ref_count(a) token_info((a))    /* reference count preceding a token list */
#  define set_token_ref_count(a,b) token_info((a))=b
#  define add_token_ref(a)   token_ref_count(a)++       /* new reference to a token list */

#  define store_new_token(a) do {				\
    q=get_avail(); token_link(p)=q; token_info(q)=(a); p=q;	\
  } while (0)

#  define fast_store_new_token(a) do {					\
    fast_get_avail(q); token_link(p)=q; token_info(q)=(a); p=q;	\
  } while (0)

extern void delete_token_ref(halfword p);

extern void make_token_table(lua_State * L, int cmd, int chr, int cs);

#  define  NO_CAT_TABLE      -2
#  define  DEFAULT_CAT_TABLE -1

extern boolean str_eq_cstr(str_number, char *, size_t);
extern void get_next(void);
extern void check_outer_validity(void);
extern boolean scan_keyword(char *);
extern halfword active_to_cs(int, int);
extern void get_token_lua(void);
extern int get_char_cat_code(int);


#endif
