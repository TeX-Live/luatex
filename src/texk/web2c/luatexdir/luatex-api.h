/* $Id */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <../lua51/lua.h>
#include <../lua51/lauxlib.h>
#include <../lua51/lualib.h>


typedef struct LoadS {
    const char *s;
    size_t size;
} LoadS;

extern lua_State *Luas[];

extern void make_table (lua_State *L, char *tab, char *getfunc, char*setfunc);

extern int luac_main (int argc, char* argv[]);

extern int luaopen_tex (lua_State *L);

extern int luaopen_pdf (lua_State *L);

#define LUA_TEXFILEHANDLE		"TEXFILE*"

extern int luaopen_texio (lua_State *L);

extern int luaopen_lang (lua_State *L);

extern void luatex_load_init (int s, LoadS *ls);

extern lua_State *luatex_error (lua_State *L, int fatal);

extern int luaopen_unicode (lua_State *L);
extern int luaopen_zip (lua_State *L);
extern int luaopen_lfs (lua_State *L);
extern int luaopen_lpeg (lua_State *L);
extern int luaopen_md5 (lua_State *L);
extern int luaopen_zlib (lua_State *L);
extern int luaopen_gzip (lua_State *L);
extern int luaopen_ff  (lua_State *L);

extern int luaopen_img  (lua_State *L);
extern int luaopen_mp (lua_State *L);

extern void open_oslibext (lua_State *L, int safer_option);

extern void initfilecallbackids    (int max);
extern void setinputfilecallbackid (int n, int i) ;
extern void setreadfilecallbackid  (int n, int i) ;
extern int  getinputfilecallbackid (int n);
extern int  getreadfilecallbackid  (int n);

extern void lua_initialize (int ac, char **av);

extern int luaopen_kpse (lua_State *L);

extern int luaopen_callback (lua_State *L);

extern int luaopen_lua (lua_State *L, int n, char *fname);

extern int luaopen_stats (lua_State *L);

extern int luaopen_font (lua_State *L);
extern int font_to_lua   (lua_State *L, int f) ;
extern int font_from_lua (lua_State *L, int f) ; /* return is boolean */

extern int   luaopen_token (lua_State *L);
extern char *tokenlist_to_cstring (int p , int inhibit_par, int *siz);
extern void  tokenlist_to_lua(lua_State *L, int p);
extern void  tokenlist_to_luastring(lua_State *L, int p) ;
extern int   tokenlist_from_lua(lua_State *L);

extern void  lua_nodelib_push (lua_State *L) ;
extern int   luaopen_node (lua_State *L);
extern void  nodelist_to_lua (lua_State *L, int n) ;
extern int   nodelist_from_lua (lua_State *L); 


extern void dump_luac_registers (void);

extern void undump_luac_registers (void);


extern void unhide_lua_table(lua_State *lua, char *name, int r);
extern int  hide_lua_table  (lua_State *lua, char *name);

extern void unhide_lua_value(lua_State *lua, char *name, char *item, int r);
extern int  hide_lua_value  (lua_State *lua, char *name, char *item);

typedef struct command_item_ {
  char *cmd_name;
  int command_offset;
  char **commands;
} command_item;
 
extern command_item command_names[];
extern int callback_callbacks_id;

void  luainterpreter (int n) ;

