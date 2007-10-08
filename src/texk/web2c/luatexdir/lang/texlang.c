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

This is texlang.c
*/

#include "luatex-api.h"
#include <ptexlib.h>

#include <string.h>

#include "nodes.h"
#include "hyphen.h"

/* functions from the fontforge unicode library */

extern unsigned int *utf82u_strcpy(unsigned int *ubuf,const char *utf8buf);
extern unsigned int u_strlen(unsigned int *ubuf);
extern char *utf8_idpb(char *w,unsigned int i);

#define noVERBOSE

#define MAX_TEX_LANGUAGES 65536

static struct tex_language *tex_languages[MAX_TEX_LANGUAGES] = {NULL};
static int next_lang_id = 0;

struct tex_language *
new_language (void) {
  struct tex_language* lang;
  if (next_lang_id<MAX_TEX_LANGUAGES) {
	lang = xmalloc(sizeof(struct tex_language));
	tex_languages[next_lang_id] = lang;
	lang->id = next_lang_id++;
	lang->lhmin = 2;
	lang->rhmin = 3;
	lang->uchyph = 0;
	lang->exceptions = 0;
	lang->patterns = NULL;
	return lang;
  } else {
	return NULL;
  }
}

struct tex_language *
get_language (int n) {
  if (n>=0 && n<=MAX_TEX_LANGUAGES )  {
    if (tex_languages[n]!=NULL) {
      return tex_languages[n];
    } else {
      return new_language();
    }
  } else {
    return NULL;
  }
}

void 
load_patterns (struct tex_language *lang, unsigned char *buffer) {
  if (lang==NULL)
    return;
  if (lang->patterns==NULL) {
    lang->patterns = hnj_hyphen_new();
  }
  hnj_hyphen_load (lang->patterns,buffer);
}

void 
clear_patterns (struct tex_language *lang) {
  if (lang==NULL)
    return;
  if (lang->patterns!=NULL) {
    hnj_hyphen_clear(lang->patterns);
  }
}


void
load_tex_patterns(int curlang, halfword head) {
  char *s = tokenlist_to_cstring (head,1, NULL);
  load_patterns(get_language(curlang),(unsigned char *)s);
}


#define STORE_CHAR(x) { word[w] = x ; if (w<MAX_WORD_LEN) w++; }
 
char *
clean_hyphenation (char *buffer, char **cleaned) {
  int items;
  unsigned char word [MAX_WORD_LEN+1];
  int w = 0;
  char *s = buffer;
  while (*s && !isspace(*s)) {
    if (*s == '-') {	 /* skip */
    } else if (*s == '=') {
      STORE_CHAR('-');
    } else if (*s == '{') {
      s++;
      items=0;
      while (*s && *s!='}') { STORE_CHAR(*s); s++; }
      if (*s=='}') { items++; s++; }
      if (*s=='{') { while (*s && *s!='}') s++; }
      if (*s=='}') { items++; s++; }
      if (*s=='{') { while (*s && *s!='}') s++; }
      if (*s=='}') { items++; } else { s--; }
      if (items!=3) { /* syntax error */
	*cleaned = NULL;	
	while (*s && !isspace(*s)) { s++; }
	return s;
      }
    } else {
      STORE_CHAR(*s); 
    }
    s++;
  }
  word[w] = 0;
  *cleaned = xstrdup((char *)word);
  return s;
}


#define STORE_WORD(L,value,cleaned) {		\
  if ((s-value)>0) {				\
    lua_pushstring(L,cleaned);			\
    lua_pushlstring(L,value,(s-value));		\
    lua_rawset(L,-3);				\
  }						\
  free(cleaned); }

void
load_hyphenation (struct tex_language *lang, unsigned char *buffer) {
  char *s, *value, *cleaned;
  lua_State *L = Luas[0];
  if (lang==NULL)
    return;
  if (lang->exceptions==0) {
    lua_newtable(L);
    lang->exceptions = luaL_ref(L,LUA_REGISTRYINDEX);
  }
  lua_rawgeti(L, LUA_REGISTRYINDEX, lang->exceptions);
  s = (char *)buffer;
  while (*s) {
    while (isspace(*s)) s++;
    if (*s) {
      value = s;
      s = clean_hyphenation(s, &cleaned);
      if (cleaned!=NULL) {
	STORE_WORD(L,value,cleaned);
      } else {
#ifdef VERBOSE
	fprintf(stderr,"skipping invalid hyphenation exception: %s\n",value);      
#endif
      }
    }
  }
}



void
load_tex_hyphenation(int curlang, halfword head) {
  char *s = tokenlist_to_cstring (head,1, NULL);
  load_hyphenation(get_language(curlang),(unsigned char *)s);
}


halfword insert_discretionary ( halfword t,  halfword pre,  halfword post,  int replace) {
  halfword g;
#ifdef VERBOSE
  /* fprintf(stderr,"disc (%d,%d,%d) after %c\n", pre, post, replace, character(t));*/
#endif
  g = lua_node_new(disc_node,0);
  vlink(g) = vlink(t);
  vlink(t) = g;
  for (g=pre;g!=null;g =vlink(g)) {
    font(g)=font(t);
    if (node_attr(t)!=null) {
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) += 1; 
    }
  }
  for (g=post;g!=null;g =vlink(g)) {
    font(g)=font(t);
    if (node_attr(t)!=null) {
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) += 1; 
    }
  }
  if (node_attr(t)!=null) {
    node_attr(vlink(t)) = node_attr(t);
    attr_list_ref(node_attr(t)) += 1; 
  }
  t = vlink(t);
  pre_break(t) = pre;
  post_break(t) = post;
  replace_count(t) = replace;
  return t;
}

halfword insert_syllable_discretionary ( halfword t,  lang_variables *lan) {
  halfword pre = null, pos = null;
  if (lan->pre_hyphenchar >0) pre = insert_character ( null,  lan->pre_hyphenchar);
  if (lan->post_hyphenchar>0) pos = insert_character ( null,  lan->post_hyphenchar);
  return insert_discretionary ( t, pre, pos, 0);
}

halfword insert_word_discretionary ( halfword t,  lang_variables *lan) {
  halfword pre = null, pos = null;
  if (lan->pre_hyphenchar >0) pre = insert_character ( null,  lan->pre_hyphenchar);
  if (lan->post_hyphenchar>0) pos = insert_character ( null,  lan->post_hyphenchar);
  return insert_discretionary ( t, pre, pos, 0);
}

halfword insert_complex_discretionary ( halfword t, lang_variables *lan, 
					halfword pre,  halfword pos,  int replace) {
  return insert_discretionary ( t, pre, pos, replace);
}


halfword insert_character ( halfword t,  int c) {
  halfword g;
  g = lua_node_new(glyph_node,0);
  character(g)=c;
  if (t!=null) {
    vlink(t)=g;
  }
  return g;
}


char *hyphenation_exception(int curlang, char *w) {
  char *ret = NULL;
  lua_State *L = Luas[0];
  lua_checkstack(L,2);
  lua_rawgeti(L,LUA_REGISTRYINDEX,(tex_languages[curlang])->exceptions);
  if (lua_istable(L,-1)) { /* ?? */
    lua_pushstring(L,w);    /* word table */
    lua_rawget(L,-2);
    if (lua_isstring(L,-1)) {
      ret = xstrdup((char *)lua_tostring(L,-1));
    }
    lua_pop(L,2);
    return ret;
  } else {
    lua_pop(L,1);
    return ret;
  }
}

char *exception_strings(struct tex_language *lang) {
  char *value;
  int size = 0, current =0;
  unsigned l =0;
  char *ret = NULL;
  lua_State *L = Luas[0];
  if (lang->exceptions==0)
    return NULL;
  lua_checkstack(L,2);
  lua_rawgeti(L,LUA_REGISTRYINDEX,lang->exceptions);
  if (lua_istable(L,-1)) {
    /* iterate and join */
    lua_pushnil(L);  /* first key */
    while (lua_next(L,-2) != 0) {
      value = (char *)lua_tolstring(L, -1, &l);
      if (current + 2 + l > size ) {
	ret = xrealloc(ret, (1.2*size)+current+l+1024);
	size = (1.2*size)+current+l+1024;
      }
      *(ret+current) = ' ';
      strcpy(ret+current+1,value);
      current += l+1;
      lua_pop(L, 1);
    }
  }
  return ret;
}

/* TODO : this is all wrong */

void do_exception (halfword wordstart, halfword r, char *replacement) {
  int i;
  halfword g,gg,t,h,hh;
  unsigned len;
  int uword[MAX_WORD_LEN+1] = {0};
  (void)utf82u_strcpy((unsigned int *)uword,replacement);
  len = u_strlen((unsigned int *)uword); 
  i = 0;
  t=wordstart;
  while (i<len) { /* still stuff to do */
    while (vlink(t)!=r && type(t)!=glyph_node)
      t = vlink(t);
    if (vlink(t)==r)
      break;
    if (uword[i+1] == '-') { /* a hyphen follows */
      g = lua_node_new(glyph_node,0);
      font(g) = font(t);
      character(g) = '-';    
      insert_discretionary(t,g,null,0);
      i++;
    } else if (uword[i+1] == '=') { 
      t = vlink(t);
      i++;
    } else if (uword[i+1] == '{') {
      /* a three-part exception */
      int repl = 0;
      i++;
      while (i<len && uword[i+1] != '}') { /* find the replace count */
	repl++; i++;
      }
      i++;
      if (i==len || uword[i+1] != '{') { perror ("broken pattern 1");  uexit(1); }
      i++; 
#ifdef VERBOSE	
      fprintf (stderr,"count: %d\n",repl);
#endif
      g = null; gg =null;
      while (i<len && uword[i+1] != '}') { /* find the prebreak text */
	if (g==null) {
	  gg = lua_node_new(glyph_node,0);
	  g = gg;
	} else {
	  vlink(g) = lua_node_new(glyph_node,0);
	  g = vlink(g);
	}
 	character(g) = uword[i+1];
#ifdef VERBOSE	
	fprintf (stderr,"prebreak: %c\n",uword[i+1]);
#endif
	i++;
      }
      i++;
      if (i==len || uword[i+1] != '{') { perror ("broken pattern 2");  uexit(1); }
      i++;
      h = null; hh =null;
      while (i<len && uword[i+1] != '}') {  /* find the postbreak text */
	if (h==null) {
	  hh = lua_node_new(glyph_node,0);
	  h = hh;
	} else {
	  vlink(h) = lua_node_new(glyph_node,0);
	  h = vlink(h);
	}
	character(h) = uword[i+1];
#ifdef VERBOSE	
	fprintf (stderr,"postbreak: %c\n",uword[i+1]);
#endif
	i++;
      }
      if (i==len) {
	perror ("broken pattern 3");
	uexit(1);
      }
      /* jump over the last right brace */
      t = insert_discretionary(t,gg,hh,repl);

    }
    t = vlink(t);
    i++;
  }
}

/* todo:  \hyphenation exceptions
 * incomp: first word hyphenated 
 */

halfword find_next_wordstart(halfword r, lang_variables *langdata) {
  int start_ok = 1;
  int l;
  while (r!=null) {
    switch (type(r)) {
    case glue_node:
      start_ok = 1;
      break;
    case glyph_node:
      if (start_ok) {
	l = get_lc_code(character(r));
	if (l>0) {
	  if (langdata->uc_hyph || l == character(r)) {
	    return r;
	  } else {
	    start_ok = 0;
	  }
	}
      }
      break;
    case math_node:
      r = vlink(r);
      while (r && type(r)!=math_node) 
	r = vlink(r);
      break;
    case whatsit_node:
      if (subtype(r)==language_node) {
	langdata->curlang = what_lang(r);
	langdata->lhmin   = what_lhm(r)>0 ? what_lhm(r) : 1;
	langdata->rhmin   = what_rhm(r)>0 ? what_rhm(r) : 1;
      }
      break;
    default:
      start_ok = 0;
      break;
    }
    r = vlink(r);
  }
  return r;
}

void 
hnj_hyphenation (halfword head, halfword tail, int init_cur_lang, int init_lhyf, int init_rhyf, int uc_hyph) {
  int lchar, i;
  struct tex_language* lang;
  lang_variables langdata;
  char utf8word[(4*MAX_WORD_LEN)+1] = {0};
  int wordlen = 0;
  char *hy = utf8word;
  char *replacement = NULL;
  halfword r = head, wordstart = null, save_tail = null, left = null, right = null;

  assert (init_cur_lang>=0);
  assert (init_cur_lang<MAX_TEX_LANGUAGES);
  assert (init_lhyf>0);
  assert (init_rhyf>0);
  assert (tail!=null);

  langdata.lhmin = init_lhyf;
  langdata.rhmin = init_rhyf;
  langdata.curlang = init_cur_lang;
  langdata.uc_hyph = uc_hyph;
  langdata.pre_hyphenchar = '-';
  langdata.post_hyphenchar = -1;

  save_tail = vlink(tail);
  vlink(tail) = null;
  /* find the first character */
  while (r!=null && type(r)!=glyph_node)
    r =vlink(r);
  /* this will return r, unless the glyph was not a valid start letter */
  r = find_next_wordstart(r, &langdata); 
  while (r!=null) { /* could be while(1), but let's be paranoid */
    wordstart = r;
    while (r!=null && type(r)==glyph_node && (lchar=get_lc_code(character(r)))>0) {
      wordlen++;
      if (wordlen>=langdata.rhmin) {
	if (wordlen==langdata.rhmin)
	  right = wordstart;
	else
	  right = vlink(right);
      }
      r = vlink(r);
      hy = utf8_idpb(hy,lchar);
    }
    if (wordlen >= langdata.lhmin+langdata.rhmin &&
	(lang=tex_languages[langdata.curlang])!=NULL) {
      *hy=0;
      if (lang->exceptions!=0 && 
	  (replacement = hyphenation_exception(langdata.curlang,utf8word))!=NULL) {
#ifdef VERBOSE
	fprintf(stderr,"replacing %s (c=%d) by %s\n",utf8word,langdata.curlang,replacement);
#endif		
	do_exception(wordstart,r,replacement);
	free(replacement);
      } else if (lang->patterns!=NULL) {
	left = wordstart;
	for (i=langdata.lhmin;i>1;i--)
	  left = vlink(left);
#ifdef VERBOSE
	fprintf(stderr,"hyphenate %s (c=%d,l=%d,r=%d) from %c to %c\n",utf8word,
		langdata.curlang,langdata.lhmin,langdata.rhmin,
		character(left), character(right));
#endif		
	(void)hnj_hyphen_hyphenate(lang->patterns,wordstart,r,wordlen,left,right, &langdata); 
      }
    }
    wordlen = 0;
    hy = utf8word;
    right = null;
    if (r==null)
      break;
    r = find_next_wordstart(r, &langdata);
  }
  vlink(tail) = save_tail;
}


void 
new_hyphenation (halfword head, halfword tail, int init_cur_lang, 
		 int init_lhyf, int init_rhyf, int uc_hyph) {
  int callback_id = 0;
  lua_State *L = Luas[0];
  callback_id = callback_defined(hyphenate_callback);
  if (head==null || vlink(head)==null)
    return;
  if (callback_id>0) {
    /* */
    lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
    lua_rawgeti(L,-1, callback_id);
    if (!lua_isfunction(L,-1)) {
      lua_pop(L,2);
      return;
    }
    nodelist_to_lua(L,head);
    nodelist_to_lua(L,tail);
    lua_pushnumber(L,init_cur_lang);
    lua_pushnumber(L,init_lhyf);
    lua_pushnumber(L,init_rhyf);
    lua_pushnumber(L,uc_hyph);
    if (lua_pcall(L,6,0,0) != 0) {
      fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
      lua_pop(L,2);
      lua_error(L);
      return;
    } 
    lua_pop(L,1);
  }  else {
    hnj_hyphenation(head,tail,init_cur_lang,init_lhyf,init_rhyf,uc_hyph);
  }
}

/* dumping and undumping fonts */

#define dump_string(a)				\
  if (a!=NULL) {				\
    x = strlen(a)+1;				\
    dump_int(x);  dump_things(*a, x);		\
  } else {					\
    x = 0; dump_int(x);				\
  }


void dump_one_language (int i) {
  char *s = NULL;
  unsigned x = 0;
  struct tex_language *lang;
  lang = tex_languages[i];
  dump_int(lang->id);
  dump_int(lang->lhmin);
  dump_int(lang->rhmin);
  dump_int(lang->uchyph);  
  if (lang->patterns!=NULL) {
    s = (char *)hnj_serialize(lang->patterns);
  }
  dump_string(s);
  if (s!=NULL) {
    free(s);
    s = NULL;
  }
  if (lang->exceptions!=0)
   s = exception_strings(lang);
  dump_string(s);
  if (s!=NULL) {
    free(s);
  }
  free (lang);
}

void dump_language_data (void) {
  int i;
  dump_int(next_lang_id);
  for (i=0;i<next_lang_id;i++) {
    if (tex_languages[i]) {
      dump_int(1);
      dump_one_language(i);
    } else {
      dump_int(0);
    }
  }
}


void undump_one_language (int i) {
  char *s = NULL;
  unsigned x = 0;
  struct tex_language *lang = get_language(i);
  undump_int(x); lang->id = x;
  undump_int(x); lang->lhmin = x;
  undump_int(x); lang->rhmin = x;
  undump_int(x); lang->uchyph = x;
  /* patterns */
  undump_int (x);
  if (x>0) {
    s = xmalloc(x); 
    undump_things(*s,x);
    load_patterns(lang,(unsigned char *)s);
    free(s);
  }
  /* exceptions */
  undump_int (x);
  if (x>0) {
    s = xmalloc(x); 
    undump_things(*s,x);
    load_hyphenation(lang,(unsigned char *)s);
    free(s);
  }
}

void undump_language_data (void) {
  int i;
  unsigned x, numlangs;
  undump_int(numlangs);
  for (i=0;i<numlangs;i++) {
    undump_int(x);
    if (x==1) {
      undump_one_language(i);
    }
  }
}
