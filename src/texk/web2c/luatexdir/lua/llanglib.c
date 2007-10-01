/*
Copyright (c) 2007 Taco Hoekwater <taco@latex.org>

This file is part of luatex.

luatex is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

luatex is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with luatex; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

This is llanglib.c 
*/

#include "luatex-api.h"
#include <ptexlib.h>

#include "../lua/nodes.h"

#define LANG_METATABLE "luatex.lang"

#define check_islang(L,b) (struct tex_language **)luaL_checkudata(L,b,LANG_METATABLE)

static int 
lang_new (lua_State *L) {
  struct tex_language **lang;
  if (lua_gettop(L)==0) {
    lang = lua_newuserdata(L, sizeof(struct tex_language *));
    *lang = new_language();
    if (!*lang) {
      lua_pushstring(L,"lang.new(): no room for a new language");
      return lua_error(L);
    }
  } else {
    lang = lua_newuserdata(L, sizeof(struct tex_language *));
    *lang = get_language(lua_tonumber(L,1));
    if (!*lang) {
      lua_pushstring(L,"lang.new(): no such language");
      return lua_error(L);
    }
  }
  luaL_getmetatable(L,LANG_METATABLE);
  lua_setmetatable(L,-2);
  return 1;
}

static int 
lang_use_new (lua_State *L) {
  int i = use_new_hyphenation;
  use_new_hyphenation = lua_toboolean(L,1);
  lua_pushboolean(L,i);
  return 1;
}


static int 
lang_uchyph (lua_State *L) {
  struct tex_language **lang_ptr;
  int i = -1;
  lang_ptr = check_islang(L,1);
  if (lua_gettop(L)>=2 && lua_isnumber(L,2))
    i = lua_toboolean(L,2);
  lua_pushnumber(L,(*lang_ptr)->uchyph);
  if (i>=0)
    (*lang_ptr)->uchyph = i;
  return 1;
}


static int 
lang_rhmin (lua_State *L) {
  struct tex_language **lang_ptr;
  int i = 0;
  lang_ptr = check_islang(L,1);
  if (lua_gettop(L)>=2 && lua_isnumber(L,2))
    i = lua_tointeger(L,2);
  lua_pushnumber(L,(*lang_ptr)->rhmin);
  if (i>0)
    (*lang_ptr)->rhmin = (i>MAX_WORD_LEN? MAX_WORD_LEN : i);
  return 1;
}

static int 
lang_lhmin (lua_State *L) {
  struct tex_language **lang_ptr;
  int i = 0;
  lang_ptr = check_islang(L,1);
  if (lua_gettop(L)>=2 && lua_isnumber(L,2))
    i = lua_tonumber(L,2);
  lua_pushnumber(L,(*lang_ptr)->lhmin);
  if (i>0)
    (*lang_ptr)->lhmin = (i>MAX_WORD_LEN? MAX_WORD_LEN : i);
  return 1;
}

static int 
lang_id (lua_State *L) {
  struct tex_language **lang_ptr;
  lang_ptr = check_islang(L,1);
  lua_pushnumber(L,(*lang_ptr)->id);
  return 1;
}

static int 
lang_exceptions (lua_State *L) {
  struct tex_language **lang_ptr;
  lang_ptr = check_islang(L,1);
  if (L!=Luas[0]) {
    lua_pushstring(L,"lang:exceptions(): only accessible from lua state 0");
    return lua_error(L);
  }
  if (lua_gettop(L)==2 && lua_istable(L,2)) {
    (*lang_ptr)->exceptions = luaL_ref(L,LUA_REGISTRYINDEX);
    return 0;
  } else {
    if ((*lang_ptr)->exceptions==0) {
      lua_pushnil(L);
    } else {
      lua_rawgeti(L,LUA_REGISTRYINDEX,(*lang_ptr)->exceptions);
    }
    return 1;
  }
}

static int 
lang_patterns (lua_State *L) {
  struct tex_language **lang_ptr;
  lang_ptr = check_islang(L,1);
  if (!lua_isstring(L,2)) {
    lua_pushstring(L,"lang.patterns(): argument should be a string");
    return lua_error(L);
  }
  load_patterns(*lang_ptr, (unsigned char *)lua_tostring(L,2));
  return 0;
}

static int 
lang_hyphenation (lua_State *L) {
  struct tex_language **lang_ptr;
  lang_ptr = check_islang(L,1);
  if (!lua_isstring(L,2)) {
    lua_pushstring(L,"lang.hyphenation(): argument should be a string");
    return lua_error(L);
  }
  load_hyphenation(*lang_ptr,(unsigned char *)lua_tostring(L,2));
  return 0;
}

static int 
do_lang_hyphenate (lua_State *L) {
  halfword *h,*t;
  int uc = 0, i = 6, l = 0 , r = 0, id = 0;
  h = check_isnode(L,1);
  if (lua_isuserdata(L,2)) {
    t = check_isnode(L,2);
  } else {
	i = 5;
    t = h;
    while (vlink(*t)!=null)
      *t = vlink(*t);
  }
  if (lua_gettop(L)!=i) {
	lua_pushstring(L,"lang.hyphenation(): not enough arguments");
	return lua_error(L);
  }
  id = lua_tointeger(L,(i-3));
  l  = lua_tointeger(L,(i-2));
  r  = lua_tointeger(L,(i-1));
  uc = lua_toboolean(L,i);
  hnj_hyphenation(*h,*t,id,l,r,uc);
}


static int 
lang_hyphenate (lua_State *L) {
  struct tex_language **lang_ptr;
  lang_ptr = check_islang(L,1);
  if (lua_isstring(L,2)) {
    char *w, *ret = NULL;
    w = (char *)lua_tostring(L,2);
    if (hyphenate_string(*lang_ptr,w, &ret)) {
      lua_pushstring(L,ret);
      return 1;
    } else {
      lua_pushfstring(L,"lang:hyphenate(): %s",ret);
      return lua_error(L);
    }
  } else if (lua_isuserdata(L,2)) {
    halfword *h,*t;
    int uc = 0, i = 3, l = 0 , r = 0;
    h = check_isnode(L,2);
    if (lua_isuserdata(L,3)) {
      t = check_isnode(L,3);
      i = 4;
    } else {
      t = h;
      while (vlink(*t)!=null)
	*t = vlink(*t);
    }
    l = (*lang_ptr)->lhmin;
    if (lua_gettop(L)>=i) {
      l = lua_tointeger(L,i); i++;
    }
    r = (*lang_ptr)->rhmin;
    if (lua_gettop(L)>=i) {
      r = lua_tointeger(L,i); i++;
    }
    uc = (*lang_ptr)->uchyph;
    if (lua_gettop(L)>=i) {
      uc = lua_toboolean(L,i);
    }
    hnj_hyphenation(*h,*t,(*lang_ptr)->id,l,r,uc);
    return 0;
  } else {
    lua_pushstring(L,"lang.hyphenate(): argument should be a string or a node list");
    return lua_error(L);
  }
}




static const struct luaL_reg langlib_d [] = {
  {"hyphenate",       lang_hyphenate},
  {"patterns",        lang_patterns},
  {"hyphenation",     lang_hyphenation},
  {"lefthyphenmin",   lang_lhmin},
  {"righthyphenmin",  lang_rhmin},
  {"uchyph",          lang_uchyph},
  {"exceptions",      lang_exceptions},
  {"id",              lang_id},
  {NULL, NULL}  /* sentinel */
};


static const struct luaL_reg langlib[] = {
  {"hyphenate",  do_lang_hyphenate},
  {"use_new",    lang_use_new},
  {"new",        lang_new},
  {NULL, NULL}                /* sentinel */
};


int 
luaopen_lang (lua_State *L) {
  luaL_newmetatable(L,LANG_METATABLE);
  lua_pushvalue(L, -1);  /* push metatable */
  lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  luaL_register(L, NULL, langlib_d);  /* dict methods */
  luaL_register(L, "lang", langlib);
  return 1;
}

