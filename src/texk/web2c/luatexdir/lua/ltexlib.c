/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

typedef struct {
  char *text;
  unsigned int tsize;
  void *next;
  unsigned char partial;
  int cattable;
} rope;

typedef struct {
  rope *head;
  rope *tail;
  char complete; /* currently still writing ? */
} spindle;

#define  PARTIAL_LINE       1
#define  FULL_LINE          0

#define  NO_CAT_TABLE      -2
#define  DEFAULT_CAT_TABLE -1

#define  write_spindle spindles[spindle_index]
#define  read_spindle  spindles[(spindle_index-1)]

static int      spindle_size  = 0;
static spindle *spindles      = NULL;
static int      spindle_index = 0;


static int 
do_luacprint (lua_State * L, int partial, int deftable) {
  int i, n;
  unsigned int tsize;
  char *st, *sttemp;
  rope *rn;
  int cattable = deftable;
  int startstrings = 1;
  n = lua_gettop(L);
  if (cattable != NO_CAT_TABLE) {
    if (lua_type(L,1)==LUA_TNUMBER && n>1) {
      cattable = lua_tonumber(L, 1);
      startstrings = 2;
    }
  }
  for (i = startstrings; i <= n; i++) {
    if (!lua_isstring(L, i)) {
      lua_pushstring(L, "no string to print");
      lua_error(L);
    }
    sttemp = (char *)lua_tolstring(L, i,&tsize);
    st = xmalloc((tsize+1)); 
    memcpy(st,sttemp,(tsize+1));
    if (st) {
      // fprintf(stderr,"W[%d]:=%s\n",spindle_index,st);
      luacstrings++; 
      rn = (rope *)xmalloc(sizeof(rope)); /* valgrind says we leak here */
      rn->text      = st;
      rn->tsize     = tsize;
      rn->partial   = partial;
      rn->cattable  = cattable;
      rn->next = NULL;
      if (write_spindle.head == NULL) {
	assert(write_spindle.tail == NULL);
	write_spindle.head = rn;
      } else {
	write_spindle.tail->next  = rn;
      }
      write_spindle.tail = rn;
      write_spindle.complete = 0;
    }
  }
  return 0;
}

int luacwrite(lua_State * L) {
  return do_luacprint(L,FULL_LINE,NO_CAT_TABLE);
}

int luacprint(lua_State * L) {
  return do_luacprint(L,FULL_LINE,DEFAULT_CAT_TABLE);
}

int luacsprint(lua_State * L) {
  return do_luacprint(L,PARTIAL_LINE,DEFAULT_CAT_TABLE);
}

int 
luacstring_detokenized (void) {
  return (read_spindle.tail->cattable == NO_CAT_TABLE);
}

int
luacstring_defaultcattable (void) {
  return (read_spindle.tail->cattable == DEFAULT_CAT_TABLE);
}

integer
luacstring_cattable (void) {
  return (integer)read_spindle.tail->cattable;
}

int 
luacstring_simple (void) {
  return (read_spindle.tail->partial == PARTIAL_LINE);
}

int 
luacstring_penultimate (void) {
  return (read_spindle.tail->next == NULL);
}

int 
luacstring_input (void) {
  char *st;
  int ret;
  rope *t = read_spindle.head;
  if (!read_spindle.complete) {
    read_spindle.complete =1;
    read_spindle.tail = NULL;
  }
  if (t == NULL) {
    if(read_spindle.tail != NULL)
      free(read_spindle.tail);
    read_spindle.tail = NULL;
    return 0;
  }
  if (t->text != NULL) {
    st = t->text;
    /* put that thing in the buffer */
    last = first;
    ret = last;
    check_buffer_overflow (last + t->tsize);
    while (t->tsize-->0)
      buffer[last++] = *st++;
    if (!t->partial) {
      while (last-1>ret && buffer[last-1] == ' ')
	last--;
    }
    free (t->text);
    t->text = NULL;
  }
  if (read_spindle.tail != NULL) { /* not a one-liner */
    free(read_spindle.tail);
  }
  read_spindle.tail = t;
  read_spindle.head = t->next;
  return 1;
}

/* open for reading, and make a new one for writing */
void 
luacstring_start (int n) {
  spindle_index++;
  if(spindle_size == spindle_index) { /* add a new one */
    spindles = xrealloc(spindles,sizeof(spindle)*(spindle_size+1));
    spindles[spindle_index].head = NULL;
    spindles[spindle_index].tail = NULL;
    spindles[spindle_index].complete = 0;
    spindle_size++;
  }
}

/* close for reading */

void 
luacstring_close (int n) {
  rope *next, *t;
  next = read_spindle.head;
  while (next != NULL) {
    if (next->text != NULL)
      free(next->text);
    t = next;
    next = next->next;
    free(t);
  }
  read_spindle.head = NULL;
  if(read_spindle.tail != NULL)
    free(read_spindle.tail);
  read_spindle.tail = NULL;
  read_spindle.complete = 0;
  spindle_index--;
}

/* local (static) versions */

#define width_offset 1
#define depth_offset 2
#define height_offset 3

#define check_index_range(j)                            \
   if (j<0 || j > 65535) {                                \
	lua_pushstring(L, "incorrect index value");	\
	lua_error(L);  }


static int dimen_to_number (lua_State *L,char *s){
  double v;
  char *d;
  int j;
  v = strtod(s,&d);
  if      (strcmp (d,"in") == 0) { j = (int)(((v*7227)/100)   *65536); }
  else if (strcmp (d,"pc") == 0) { j = (int)((v*12)           *65536); } 
  else if (strcmp (d,"cm") == 0) { j = (int)(((v*7227)/254)   *65536); }
  else if (strcmp (d,"mm") == 0) { j = (int)(((v*7227)/2540)  *65536); }
  else if (strcmp (d,"bp") == 0) { j = (int)(((v*7227)/7200)  *65536); }
  else if (strcmp (d,"dd") == 0) { j = (int)(((v*1238)/1157)  *65536); }
  else if (strcmp (d,"cc") == 0) { j = (int)(((v*14856)/1157) *65536); }
  else if (strcmp (d,"nd") == 0) { j = (int)(((v*21681)/20320)*65536); }
  else if (strcmp (d,"nc") == 0) { j = (int)(((v*65043)/5080) *65536); }
  else if (strcmp (d,"pt") == 0) { j = (int)(v                *65536); }
  else if (strcmp (d,"sp") == 0) { j = (int)(v); }
  else {
	lua_pushstring(L, "unknown dimension specifier");
	lua_error(L);
	j = 0;
  }
  return j;
}

int setdimen (lua_State *L) {
  int i,j,k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  j = 0;
  // find the value
  if (!lua_isnumber(L,i))
    if (lua_isstring(L,i)) {
	j = dimen_to_number(L,(char *)lua_tostring(L,i));
    } else {
      lua_pushstring(L, "unsupported value type");
      lua_error(L);
    }
  else
    j = (int)lua_tonumber(L,i);
  // find the index
  if (lua_type(L,i-1)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i-1));
    cur_cs = string_lookup(texstr);
    flush_str(texstr);
    k = zget_equiv(cur_cs)-get_scaled_base();
  } else {
    k = (int)luaL_checkinteger(L,i-1);
  }
  check_index_range(k);
  if(set_tex_dimen_register(k,j)) {
    lua_pushstring(L, "incorrect value");
    lua_error(L);
  }
  return 0;
}

int getdimen (lua_State *L) {
  int i,j,  k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  if (lua_type(L,i)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i));
    cur_cs = string_lookup(texstr);
    flush_str(texstr);
    if (is_undefined_cs(cur_cs)) {
      lua_pushnil(L);
      return 1;
    }
    k = zget_equiv(cur_cs)-get_scaled_base();
  } else {
    k = (int)luaL_checkinteger(L,i);
  }
  check_index_range(k);
  j = get_tex_dimen_register(k);
  lua_pushnumber(L, j);
  return 1;
}

int setcount (lua_State *L) {
  int i,j,k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  j = (int)luaL_checkinteger(L,i);
  if (lua_type(L,i-1)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i-1));
    cur_cs = string_lookup(texstr);
    flush_str(texstr);
    k = zget_equiv(cur_cs)-get_count_base();
  } else {
    k = (int)luaL_checkinteger(L,i-1);
  }
  check_index_range(k);
  if (set_tex_count_register(k,j)) {
	lua_pushstring(L, "incorrect value");
	lua_error(L);
  }
  return 0;
}

int getcount (lua_State *L) {
  int i, j, k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  if (lua_type(L,i)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i));
    cur_cs = string_lookup(texstr);
    flush_str(texstr);
    if (is_undefined_cs(cur_cs)) {
      lua_pushnil(L);
      return 1;
    }
    k = zget_equiv(cur_cs)-get_count_base();
  } else {
    k = (int)luaL_checkinteger(L,i);
  }
  check_index_range(k);
  j = get_tex_count_register(k);
  lua_pushnumber(L, j);
  return 1;
}


int setattribute (lua_State *L) {
  int i,j,k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  j = (int)luaL_checkinteger(L,i);
  if (lua_type(L,i-1)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i-1));
    cur_cs = string_lookup(texstr);
    flush_str(texstr);
    k = zget_equiv(cur_cs)-get_attribute_base();
  } else {
    k = (int)luaL_checkinteger(L,i-1);
  }
  check_index_range(k);
  if (set_tex_attribute_register(k,j)) {
	lua_pushstring(L, "incorrect value");
	lua_error(L);
  }
  return 0;
}

int getattribute (lua_State *L) {
  int i, j, k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  if (lua_type(L,i)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i));
    cur_cs = string_lookup(texstr);
    flush_str(texstr);
    if (is_undefined_cs(cur_cs)) {
      lua_pushnil(L);
      return 1;
    }
    k = zget_equiv(cur_cs)-get_attribute_base();
  } else {
    k = (int)luaL_checkinteger(L,i);
  }
  check_index_range(k);
  j = get_tex_attribute_register(k);
  lua_pushnumber(L, j);
  return 1;
}

int settoks (lua_State *L) {
  int i,j,k,len;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  char *st;
  if (!lua_isstring(L,i)) {
    lua_pushstring(L, "unsupported value type");
    lua_error(L);
  }
  st = (char *)lua_tostring(L,i);
  len = lua_strlen(L, i);

  if (lua_type(L,i-1)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i-1));
    cur_cs = string_lookup(texstr);
    flush_str(texstr);
    k = zget_equiv(cur_cs)-get_toks_base();
  } else {
    k = (int)luaL_checkinteger(L,i-1);
  }
  check_index_range(k);
  j = maketexstring(st);
  if(zset_tex_toks_register(k,j)) {
    flush_str(j);
    lua_pushstring(L, "incorrect value");
    lua_error(L);
  }
  return 0;
}

int gettoks (lua_State *L) {
  int i,k;
  strnumber t;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  if (lua_type(L,i)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i));
    cur_cs = string_lookup(texstr);
    flush_str(texstr);
    if (is_undefined_cs(cur_cs)) {
      lua_pushnil(L);
      return 1;
    }
    k = zget_equiv(cur_cs)-get_toks_base();
  } else {
    k = (int)luaL_checkinteger(L,i);
  }

  check_index_range(k);
  t = get_tex_toks_register(k);
  lua_pushstring(L, makecstring(t));
  flush_str(t);
  return 1;
}

int getbox (lua_State *L) {
  int k, t;
  k = (int)luaL_checkinteger(L,-1);
  check_index_range(k);
  t = get_tex_box_register(k);
  nodelist_to_lua(L,t);
  return 1;
}

int setbox (lua_State *L) {
  int i,j,k;
  int cur_cs;
  char *s;
  int ok = 0;
  k = (int)luaL_checkinteger(L,-2);
  check_index_range(k);
  i = get_tex_box_register(k);
  if (lua_isboolean(L,-1)) {
    j = lua_toboolean(L,-1);
    if (j==0)
      j == null;
    else 
      return 0;
  } else {
    j = nodelist_from_lua(L);
  }
  if(set_tex_box_register(k,j)) {
    lua_pushstring(L, "incorrect value");
    lua_error(L);
  }
  return 0;
}


static int getboxdim (lua_State *L, int whichdim) {
  int i, j;
  i = lua_gettop(L);
  j = (int)lua_tonumber(L,(i));
  lua_settop(L,(i-2)); /* table at -1 */
  if (j<0 || j > 65535) {
	lua_pushstring(L, "incorrect index");
	lua_error(L);
  }
  switch (whichdim) {
  case width_offset: 
	lua_pushnumber(L, get_tex_box_width(j));
	break;
  case height_offset: 
	lua_pushnumber(L, get_tex_box_height(j));
	break;
  case depth_offset: 
	lua_pushnumber(L, get_tex_box_depth(j));
  }
  return 1;
}

int getboxwd (lua_State *L) {
  return getboxdim (L, width_offset);
}

int getboxht (lua_State *L) {
  return getboxdim (L, height_offset);
}

int getboxdp (lua_State *L) {
  return getboxdim (L, depth_offset);
}

static int setboxdim (lua_State *L, int whichdim) {
  int i,j,k,err;
  i = lua_gettop(L);
  if (!lua_isnumber(L,i)) {
	j = dimen_to_number(L,(char *)lua_tostring(L,i));
  } else {
    j = (int)lua_tonumber(L,i);
  }
  k = (int)lua_tonumber(L,(i-1));
  lua_settop(L,(i-3)); /* table at -2 */
  if (k<0 || k > 65535) {
	lua_pushstring(L, "incorrect index");
	lua_error(L);
  }
  err = 0;
  switch (whichdim) {
  case width_offset: 
	err = set_tex_box_width(k,j);
	break;
  case height_offset: 
	err = set_tex_box_height(k,j);
	break;
  case depth_offset: 
	err = set_tex_box_depth(k,j);
  }
  if (err) {
	lua_pushstring(L, "not a box");
	lua_error(L);
  }
  return 0;
}

int setboxwd (lua_State *L) {
  return setboxdim(L,width_offset);
}

int setboxht (lua_State *L) {
  return setboxdim(L,height_offset);
}

int setboxdp (lua_State *L) {
  return setboxdim(L,depth_offset);
}

int settex (lua_State *L) {
  char *st;
  int i,j,texstr;
  int cur_cs, cur_cmd;
  j = 0;
  i = lua_gettop(L);
  if (lua_isstring(L,(i-1))) {
    st = (char *)lua_tostring(L,(i-1));
    texstr = maketexstring(st);
    if (zis_primitive(texstr)) {
      cur_cs = string_lookup(texstr);
      flush_str(texstr);
      cur_cmd = zget_eq_type(cur_cs);
      if (is_int_assign(cur_cmd)) {
	if (lua_isnumber(L,i)) {
	  assign_internal_int(zget_equiv(cur_cs),lua_tonumber(L,i));
	} else {
	  lua_pushstring(L, "unsupported value type");
	  lua_error(L);
	}
      } else if (is_dim_assign(cur_cmd)) {
	if (!lua_isnumber(L,i))
	  if (lua_isstring(L,i)) {
	    j = dimen_to_number(L,(char *)lua_tostring(L,i));
	  } else {
	    lua_pushstring(L, "unsupported value type");
	    lua_error(L);
	  }
	else
	  j = (int)lua_tonumber(L,i);
	assign_internal_dim(zget_equiv(cur_cs),j);
      } else {
	lua_pushstring(L, "unsupported tex internal assignment");
	lua_error(L);
      }
    } else {
      lua_rawset(L,(i-2));
    }
  } else {
    lua_rawset(L,(i-2));
  }
  return 0;
}

char *
get_something_internal (int cur_cmd, int cur_code) {
  int texstr;
  char *str;
  int save_cur_val,save_cur_val_level;
  save_cur_val = cur_val;
  save_cur_val_level = cur_val_level;
  zscan_something_simple(cur_cmd,cur_code);
  texstr = the_scanned_result();
  cur_val = save_cur_val;
  cur_val_level = save_cur_val_level;  
  str = makecstring(texstr);
  flush_str(texstr);
  return str;
}

char *
get_convert (int cur_code) {
  int texstr;
  char *str = NULL;
  texstr = the_convert_string(cur_code);
  if (texstr) {
    str = makecstring(texstr);
    flush_str(texstr);
  }
  return str;
}


int
gettex (lua_State *L) {
  char *st;
  int i,texstr;
  char *str;
  int cur_cs, cur_cmd, cur_code;
  i = lua_gettop(L);
  if (lua_isstring(L,i)) {
    st = (char *)lua_tostring(L,i);
    texstr = maketexstring(st);
    cur_cs = zprim_lookup(texstr);
    flush_str(texstr);
    if (cur_cs) {
      cur_cmd = zget_prim_eq_type(cur_cs);
      cur_code = zget_prim_equiv(cur_cs);
      if (is_convert(cur_cmd))
	str = get_convert(cur_code);
      else 
	str = get_something_internal(cur_cmd,cur_code);
      if (str)
	lua_pushstring(L,str);
      else 
	lua_pushnil(L);
      return 1;
    } else {
      lua_rawget(L,(i-1));
      return 1;
    }    
  } else {
    lua_rawget(L,(i-1));
    return 1;
  }
  return 0; // not reached
}


int
getlist (lua_State *L) {
  char *str;
  if (lua_isstring(L,1)) {
    str = (char *)lua_tostring(L,1);
    if (strcmp(str,"page_ins_head")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"contrib_head")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"page_head")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"temp_head")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"hold_head")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"adjust_head")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"pre_adjust_head")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"active")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"align_head")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else if (strcmp(str,"end_span")==0) {
      lua_pushnumber(L,page_ins_head);  lua_nodelib_push(L);
    } else {
      lua_pushnil(L);
    }
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int
setlist (lua_State *L) {
  return 0;
}


#define STORE_WORD()				\
if (w>0) {					\
  word[w] = 0;					\
  *s = 0;					\
  if (!(limit>0 && w<limit)) {			\
    lua_pushlstring(L,(char *)word,w);	\
    lua_pushlstring(L,value,(s-value));		\
    lua_rawset(L,-3);				\
  }						\
  w=0;						\
}

static int 
language_load_dict (lua_State *L) {
  char *filename;
  FILE *dict;
  char *s, *key, *value;
  int w, limit;
  integer len;
  unsigned char word [256];
  unsigned char *buffer;

  if (lua_isstring(L,1)) {
    filename = (char *)lua_tostring(L,1);
  }
  limit = luaL_optinteger (L,2,0);
  if((dict = fopen(filename,"rb"))) {
    if(readbinfile(dict,&buffer,&len)) {
      s = (char *)buffer;
      value = (char *)buffer;
      w = 0;
      lua_newtable(L);
      while (*s) {
		if (*s == ' ' || *s == '\r' || *s == '\t' || *s == '\n') {
		  STORE_WORD();
		  value = s+1;
		} else if (*s == '-' || *s == '=') {
		  /* skip */
		} else {
		  word[w] = *s;  
		  if (w<255) 
			w++;
		}
		s++;
      }
      /* fix a trailing word */
      STORE_WORD();
    } else {
      lua_pushnil(L);
    }
    fclose(dict);
    free(buffer);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

#define infinity 2147483647

#define test_integer(m,n)						\
  if (m>(double)infinity || m<-(double)infinity ) {			\
    if (m>0.0) n = infinity; else n = -infinity;				\
    char *help[] = {"I can only go up to 2147483647='17777777777=""7FFFFFFF,", \
		    "so I'm using that number instead of yours.",   NULL } ; \
    tex_error("Number too big",help);  } 


static int
tex_roundnumber (lua_State *L) {
  double m = lua_tonumber(L, 1)+0.5;
  double n = floor(m);
  test_integer(m,n);
  lua_pushnumber(L, (integer)n);
  return 1;
}

static int
tex_scaletable (lua_State *L) {
  double n, m;
  double delta = luaL_checknumber(L, 2);
  
  if (lua_istable(L,1)) {
    lua_newtable(L); /* the new table is at index 3 */
    lua_pushnil(L);
    while (lua_next(L,1)!= 0 ) {
      /* numeric value */
      if (lua_isnumber(L,-1)) {
        m = (double)(luaL_checknumber(L,-1)*delta) + 0.5;
        n = floor(m);
	test_integer(m,n);
	lua_pushvalue(L,-2);
	lua_pushnumber(L,(integer)n);
      } else {
	lua_pushvalue(L,-2);
	lua_pushvalue(L,-2);
      }
      lua_rawset(L,3);

      lua_pop(L, 1);
    }
  } else if (lua_isnumber(L,1)) {
    m = (double)(luaL_checknumber(L,1)*delta) + 0.5;
    n = floor(m);
    test_integer(m,n);
    lua_pushnumber(L,(integer)n);
  } else {
    lua_pushnil(L);
  }
  return 1;
}



static const struct luaL_reg texlib [] = {
  {"write",    luacwrite},
  {"print",    luacprint},
  {"sprint",   luacsprint},
  {"setdimen", setdimen},
  {"getdimen", getdimen},
  {"setattribute", setattribute},
  {"getattribute", getattribute},
  {"setcount",  setcount},
  {"getcount",  getcount},
  {"settoks",   settoks},
  {"gettoks",   gettoks},
  {"setbox",    setbox},
  {"getbox",    getbox},
  {"setlist",   setlist},
  {"getlist",   getlist},
  {"setboxwd",  setboxwd},
  {"getboxwd",  getboxwd},
  {"setboxht",  setboxht},
  {"getboxht",  getboxht},
  {"setboxdp",  setboxdp},
  {"getboxdp",  getboxdp},
  {"load_dict", language_load_dict},
  {"round",     tex_roundnumber},
  {"scale",     tex_scaletable},
  {NULL, NULL}  /* sentinel */
};

int luaopen_tex (lua_State *L) 
{
  luaL_register(L, "tex", texlib);
  make_table(L,"attribute","getattribute","setattribute");
  make_table(L,"dimen","getdimen","setdimen");
  make_table(L,"count","getcount","setcount");
  make_table(L,"toks","gettoks","settoks");
  make_table(L,"box","getbox","setbox");
  make_table(L,"lists","getlist","setlist");
  make_table(L,"wd","getboxwd","setboxwd");
  make_table(L,"ht","getboxht","setboxht");
  make_table(L,"dp","getboxdp","setboxdp");
  /* make the meta entries */
  /* fetch it back */
  luaL_newmetatable(L,"tex_meta"); 
  lua_pushstring(L, "__index");
  lua_pushcfunction(L, gettex); 
  lua_settable(L, -3);
  lua_pushstring(L, "__newindex");
  lua_pushcfunction(L, settex); 
  lua_settable(L, -3);
  lua_setmetatable(L,-2); /* meta to itself */
  /* initialize the I/O stack: */
  spindles = xmalloc(sizeof(spindle));
  spindle_index = 0;
  spindles[0].head = NULL;
  spindles[0].tail = NULL;
  spindle_size = 1;
  return 1;
}

