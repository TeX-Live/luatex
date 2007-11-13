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

#define MAX_TEX_LANGUAGES  32767

static struct tex_language *tex_languages[MAX_TEX_LANGUAGES] = {NULL};
static int next_lang_id = 0;

struct tex_language *
new_language (void) {
  struct tex_language* lang;
  if (next_lang_id<MAX_TEX_LANGUAGES) {
	lang = xmalloc(sizeof(struct tex_language));
	tex_languages[next_lang_id] = lang;
	lang->id = next_lang_id++;
	lang->exceptions = 0;
	lang->patterns = NULL;
	lang->pre_hyphen_char = '-';
	lang->post_hyphen_char = 0;
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
set_pre_hyphen_char (integer n, integer v) {
  struct tex_language *l = get_language((int)n);
  l->pre_hyphen_char = (int)v;
}

void
set_post_hyphen_char (integer n, integer v) {
  struct tex_language *l = get_language((int)n);
  l->post_hyphen_char = (int)v;
}

integer
get_pre_hyphen_char (integer n) {
  struct tex_language *l = get_language((int)n);
  return (integer)l->pre_hyphen_char;
}

integer
get_post_hyphen_char (integer n) {
  struct tex_language *l = get_language((int)n);
  return (integer)l->post_hyphen_char;
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
 
/* todo change this! */

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
clear_hyphenation (struct tex_language *lang) {
  if (lang==NULL)
    return;
  if (lang->exceptions!=0) {
	luaL_unref(Luas[0],LUA_REGISTRYINDEX,lang->exceptions);
	lang->exceptions = 0;
  }
}


void
load_tex_hyphenation(int curlang, halfword head) {
  char *s = tokenlist_to_cstring (head,1, NULL);
  load_hyphenation(get_language(curlang),(unsigned char *)s);
}

/* TODO: check this. The delete_attribute_ref() statements are not very 
   nice, but needed. Also, in the post-break, it would be nicer to get the 
   attribute list from vlink(n) */

halfword insert_discretionary ( halfword t,  halfword pre,  halfword post,  halfword replace) {
  halfword g, n;
  n = new_node(disc_node,syllable_disc);
  couple_nodes(n,vlink(t));
  couple_nodes(t,n);
  for (g=pre;g!=null;g=vlink(g)) {
    font(g)=font(t);
    if (node_attr(t)!=null) {
	  delete_attribute_ref(node_attr(g));
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) += 1; 
    }
  }
  for (g=post;g!=null;g =vlink(g)) {
    font(g)=font(t);
    if (node_attr(t)!=null) {
	  delete_attribute_ref(node_attr(g));
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) += 1; 
    }
  }
  for (g=replace;g!=null;g =vlink(g)) {
    font(g)=font(t);
    if (node_attr(t)!=null) {
	  delete_attribute_ref(node_attr(g));
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) += 1; 
    }
  }
  if (node_attr(t)!=null) {
	delete_attribute_ref(node_attr(vlink(t)));
    node_attr(vlink(t)) = node_attr(t);
    attr_list_ref(node_attr(t)) += 1; 
  }
  t = vlink(t);
  if (pre!=null) {
	couple_nodes(pre_break(t),pre);
	tlink(pre_break(t)) = pre;
  }
  if (post!=null) {
	couple_nodes(post_break(t),post);
	tlink(post_break(t)) = post;
  }
  if (replace!=null) {
	couple_nodes(no_break(t),replace);
	tlink(no_break(t)) = replace;
  }
  return t;
}

halfword insert_syllable_discretionary ( halfword t,  lang_variables *lan) {
  halfword g, n;
  n = new_node(disc_node,syllable_disc);
  couple_nodes(n,vlink(t));
  couple_nodes(t,n);
  
  delete_attribute_ref(node_attr(n));
  node_attr(n) = node_attr(t);
  attr_list_ref(node_attr(t))++ ;

  if (lan->pre_hyphen_char >0) {
	g = raw_glyph_node();
	set_to_character(g); 
	character(g)=lan->pre_hyphen_char;
    font(g)=font(t);
	lang_data(g)=lang_data(t);
    if (node_attr(t)!=null) {
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) ++; 
    }
	couple_nodes(pre_break(n),g);
	tlink(pre_break(n)) = g;
  }

  if (lan->post_hyphen_char >0) {
	t = vlink(n);
	g = raw_glyph_node();
	set_to_character(g); 
	character(g)=lan->post_hyphen_char;
    font(g)=font(t);
	lang_data(g)=lang_data(t);
    if (node_attr(t)!=null) {
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) += 1; 
    }
	couple_nodes(post_break(n),g);
	tlink(post_break(n)) = g;
  }
  return n;
}

halfword insert_word_discretionary ( halfword t,  lang_variables *lan) {
  halfword pre = null, pos = null;
  if (lan->pre_hyphen_char >0) pre = insert_character ( null,  lan->pre_hyphen_char);
  if (lan->post_hyphen_char>0) pos = insert_character ( null,  lan->post_hyphen_char);
  return insert_discretionary ( t, pre, pos, null);
}

halfword insert_complex_discretionary ( halfword t, lang_variables *lan, 
					halfword pre,  halfword pos,  halfword replace) {
  return insert_discretionary ( t, pre, pos, replace);
}


halfword insert_character ( halfword t,  int c) {
  halfword p;
  p=new_node(glyph_node,0);
  set_to_character(p); 
  character(p)=c;
  if (t!=null) {
    couple_nodes(t,p);
  }
  return p;
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



/* the sequence from |wordstart| to |r| can contain:
   normal characters, 
   right ghosts, 
   and left ghosts.
*/

halfword find_exception_part(int *j, int *uword, int len) {
  halfword g,gg;
  int i = *j;
  i++; /* this puts uword[i] on the '{' */
  g = null; gg =null;
  while (i<len && uword[i+1] != '}') {
    if (g==null) {
      gg = new_char_node(0,uword[i+1]);
      g = gg;
    } else {
	  halfword s = new_char_node(0,uword[i+1]);
      couple_nodes(g,s);
      g = vlink(g);
    }
    i++;
  }
  *j = i;
  return gg;
}

static char *PAT_ERROR[] =  { 
  "Exception discretionaries should contain three pairs of braced items.", 
  "No intervening spaces are allowed.", 
  NULL };

void do_exception (halfword wordstart, halfword r, char *replacement) {
  int i;
  halfword t, g, gg, hh, repl;
  unsigned len;
  int uword[MAX_WORD_LEN+1] = {0};
  (void)utf82u_strcpy((unsigned int *)uword,replacement);
  len = u_strlen((unsigned int *)uword); 
  i = 0;
  t=wordstart;
  while (i<len) { /* still stuff to do */
    if (uword[i+1] == '-') { /* a hyphen follows */
      g = new_char_node(font(t),'-');
      while (vlink(t)!=r && (type(t)!=glyph_node || !is_simple_character(t)))
	t = vlink(t);
      if (vlink(t)==r)
	break;
      t = insert_discretionary(t,g,null,null);	  
    } else if (uword[i+1] == '=') { 
      /* do nothing ? */
      t = vlink(t);
    } else if (uword[i+1] == '{') {
      gg = find_exception_part(&i,uword,len);
      if (i==len || uword[i+1] != '{') 
	tex_error ("broken pattern 1", PAT_ERROR);
      hh = find_exception_part(&i,uword,len);
      if (i==len || uword[i+1] != '{') 
	tex_error ("broken pattern 2",  PAT_ERROR); 
      repl = find_exception_part(&i,uword,len);
      if (i==len) 
	tex_error ("broken pattern 3",  PAT_ERROR);
      i++;   /* jump over the last right brace */
      if (vlink(t)==r)
	break;
      t = insert_discretionary(t,gg,hh,repl);
    } else {
      t = vlink(t);
    }
    i++;
  }
}

/* This is a documentation section from the pascal web file. It is not 
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
*/

/* 
 * This is incompatible with TEX because the first word of a paragraph
 * can be hyphenated, but most users seem to agree that prohibiting
 * hyphenation there was not a good idea.
 */

halfword find_next_wordstart(halfword r) {
  int l, mathlevel;
  int start_ok = 1;
  while (r!=null) {
    switch (type(r)) {
    case glue_node:
      start_ok = 1;
      break;
    case glyph_node:
      if (start_ok && 
	  is_simple_character(r) &&
	  (l = get_lc_code(character(r)))>0 &&
	  (char_uchyph(r) || l == character(r)))
	return r;
      if (is_ghost(r))
	break;
      start_ok = 0;
      break;
    case math_node:
      mathlevel = 1;
      while (r && mathlevel>0 ){
	r = vlink(r);
	if (type(r)==math_node) {
	  if (subtype(r)==before) {
	    mathlevel++;
	  } else {
	    mathlevel--;
	  }
	}
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

int valid_wordend( halfword s) {
  halfword r = s;
  int clang = char_lang(s);
  if (r==null)
    return 1;
  while ((r!=null) &&
		 ((type(r)==glyph_node && is_simple_character(r) && clang == char_lang(r)) ||
		  (type(r)==glyph_node && is_ghost(r)) ||
		  (type(r)==kern_node  && subtype(r)==normal))) {
    r = vlink(r);
  }
  if (r==null ||
	  (type(r)==glyph_node && 
	   (is_simple_character(r) && clang != char_lang(r))) ||
      type(r)==glue_node ||
      type(r)==whatsit_node ||
      type(r)==ins_node ||
      type(r)==adjust_node ||
      type(r)==penalty_node ||
      (type(r)==kern_node && subtype(r)==explicit))
    return 1;
  return 0;
}

void 
hnj_hyphenation (halfword head, halfword tail) {
  int lchar, i;
  struct tex_language* lang;
  lang_variables langdata;
  char utf8word[(4*MAX_WORD_LEN)+1] = {0};
  int wordlen = 0;
  char *hy = utf8word;
  char *replacement = NULL;
  halfword s, r = head, wordstart = null, save_tail = null, left = null, right = null;

  assert (tail!=null);

  save_tail = vlink(tail);
  s = new_penalty(0);
  couple_nodes(tail, s);

  /* this first movement assures two things: 
   * a) that we won't waste lots of time on something that has been
   * handled already (in that case, none of the subtypes match |subtype_character|).  
   * b) that the first word can be hyphenated. if the movement was
   * not explicit, then the indentation at the start of a paragraph
   * list would make find_next_wordstart() look too far ahead.
   */
 
  while (r!=null && (type(r)!=glyph_node || !is_simple_character(r)))
    r =vlink(r);
  /* this will make |r| a glyph node with subtype_character */
  r = find_next_wordstart(r); 

  while (r!=null) { /* could be while(1), but let's be paranoid */
    wordstart = r;
    assert (is_simple_character(wordstart));
    int clang = char_lang(wordstart);
    int lhmin = char_lhmin(wordstart);
    int rhmin = char_rhmin(wordstart);
    langdata.pre_hyphen_char = get_pre_hyphen_char(clang);
    langdata.post_hyphen_char = get_post_hyphen_char(clang);
    while (r!=null && 
		   (type(r)==glyph_node && 
			(
			 (is_simple_character(r) && clang == char_lang(r) && get_lc_code(character(r))>0) 
			 ||
			 (is_ghost(r))
			 )
			)
		   ) {
      if (is_simple_character(r)) { 
		wordlen++;
		/* -Wall warns if inline assigment done above */
		lchar=get_lc_code(character(r));
		hy = utf8_idpb(hy,lchar);
      }
	  /* TODO: this should not be needed */
      if (vlink(r)!=null)
		alink(vlink(r))=r;
	  r = vlink(r);
    }
    if (valid_wordend(r) && wordlen>=lhmin+rhmin && (lang=tex_languages[clang])!=NULL) {
      *hy=0;
      if (lang->exceptions!=0 && 
		  (replacement = hyphenation_exception(clang,utf8word))!=NULL) {
#ifdef VERBOSE
		fprintf(stderr,"replacing %s (c=%d) by %s\n",utf8word,clang,replacement);
#endif		
		do_exception(wordstart,r,replacement);
		free(replacement);
      } else if (lang->patterns!=NULL) {

		left = wordstart;
		for (i=lhmin;i>1;i--) {
		  left = vlink(left);
		  while (!is_simple_character(left))
			left = vlink(left);	    
		}
		right = r;
		for (i=rhmin;i>0;i--) {
		  right = alink(right);
		  while (!is_simple_character(right))
			right = alink(right);
		}
		
#ifdef VERBOSE
		fprintf(stderr,"hyphenate %s (c=%d,l=%d,r=%d) from %c to %c\n",utf8word,
				clang,lhmin,rhmin,
				character(left), character(right));
#endif		
		(void)hnj_hyphen_hyphenate(lang->patterns,wordstart,r,wordlen,left,right, &langdata); 
      }
    }
    wordlen = 0;
    hy = utf8word;
    if (r==null)
      break;
    r = find_next_wordstart(r);
  }
  flush_node(vlink(tail));
  vlink(tail) = save_tail;
}


void 
new_hyphenation (halfword head, halfword tail) {
  int callback_id = 0;
  lua_State *L = Luas[0];
  if (head==null || vlink(head)==null)
    return;
  fix_node_list (head); /* TODO: use couple_nodes() in append_tail()!*/
  callback_id = callback_defined(hyphenate_callback);
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
    if (lua_pcall(L,2,0,0) != 0) {
      fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
      lua_pop(L,2);
      lua_error(L);
      return;
    } 
    lua_pop(L,1);
  }  else {
    hnj_hyphenation(head,tail);
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
  dump_int(lang->pre_hyphen_char);
  dump_int(lang->post_hyphen_char);
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
  undump_int(x); lang->pre_hyphen_char = x;
  undump_int(x); lang->post_hyphen_char = x;
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

