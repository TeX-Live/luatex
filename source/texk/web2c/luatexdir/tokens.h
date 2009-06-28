/* tokens.h
   
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

#define token_list 0
#define cs_token_flag 0x1FFFFFFF

#define left_brace_token 0x200000   /* $2^{21}\cdot|left_brace|$ */
#define right_brace_token 0x400000      /* $2^{21}\cdot|right_brace|$ */


#define token_info(a)    fixmem[(a)].hhlh
#define token_link(a)    fixmem[(a)].hhrh

#  define token_ref_count(a) token_info((a))  /* reference count preceding a token list */
#  define add_token_ref(a)   token_ref_count(a)++ /* new reference to a token list */
#  define store_new_token(a) { q=get_avail(); token_link(p)=q; token_info(q)=(a); p=q; }
#  define free_avail(a)      { token_link((a))=avail; avail=(a); decr(dyn_used); }

extern void make_token_table(lua_State *L, int cmd, int chr, int cs);
