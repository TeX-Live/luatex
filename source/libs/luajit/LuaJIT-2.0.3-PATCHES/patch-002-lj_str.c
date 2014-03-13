diff -u LuaJIT-2.0.3.orig/src/lj_str.c LuaJIT-2.0.3/src/lj_str.c
--- LuaJIT-2.0.3.orig/src/lj_str.c	2014-03-13 12:38:39.403900527 +0100
+++ LuaJIT-2.0.3/src/lj_str.c	2014-03-13 13:19:53.956003116 +0100
@@ -90,6 +90,8 @@
   g->strhash = newhash;
 }
 
+#define cast(t, exp)	((t)(exp))
+LUA_API int luajittex_choose_hash_function = 0 ; 
 /* Intern a string and return string object. */
 GCstr *lj_str_new(lua_State *L, const char *str, size_t lenx)
 {
@@ -98,27 +100,44 @@
   GCobj *o;
   MSize len = (MSize)lenx;
   MSize a, b, h = len;
+  size_t step ;
+  size_t l1 ;
   if (lenx >= LJ_MAX_STR)
     lj_err_msg(L, LJ_ERR_STROV);
   g = G(L);
+
+  if (len==0)
+    return &g->strempty; 
+  if (luajittex_choose_hash_function==0) { 
+    /* Lua 5.1.5 hash function */
+    /* for 5.2 max methods we also need to patch the vm eq */ 
+    step = (len>>6)+1;  /* if string is too long, don't hash all its chars  Was 5, we try 6*/
+    for (l1=len; l1>=step; l1-=step)  /* compute hash */
+      h = h ^ ((h<<5)+(h>>2)+cast(unsigned char, str[l1-1])); 
+   } else { 
+  /* LuaJIT 2.0.2 hash function */
   /* Compute string hash. Constants taken from lookup3 hash by Bob Jenkins. */
-  if (len >= 4) {  /* Caveat: unaligned access! */
-    a = lj_getu32(str);
-    h ^= lj_getu32(str+len-4);
-    b = lj_getu32(str+(len>>1)-2);
-    h ^= b; h -= lj_rol(b, 14);
-    b += lj_getu32(str+(len>>2)-1);
-  } else if (len > 0) {
-    a = *(const uint8_t *)str;
-    h ^= *(const uint8_t *)(str+len-1);
-    b = *(const uint8_t *)(str+(len>>1));
-    h ^= b; h -= lj_rol(b, 14);
-  } else {
-    return &g->strempty;
-  }
-  a ^= h; a -= lj_rol(h, 11);
-  b ^= a; b -= lj_rol(a, 25);
-  h ^= b; h -= lj_rol(b, 16);
+    if (len >= 4) {  /* Caveat: unaligned access! */
+      a = lj_getu32(str);
+      h ^= lj_getu32(str+len-4);
+      b = lj_getu32(str+(len>>1)-2);
+      h ^= b; h -= lj_rol(b, 14);
+      b += lj_getu32(str+(len>>2)-1);
+    } else if (len > 0) {
+      a = *(const uint8_t *)str;
+      h ^= *(const uint8_t *)(str+len-1);
+      b = *(const uint8_t *)(str+(len>>1));
+      h ^= b; h -= lj_rol(b, 14);
+    } else {
+       /* Already done, kept for reference */ 
+       return &g->strempty;
+    }
+    a ^= h; a -= lj_rol(h, 11);
+    b ^= a; b -= lj_rol(a, 25);
+    h ^= b; h -= lj_rol(b, 16);
+  } 
+
+
   /* Check if the string has already been interned. */
   o = gcref(g->strhash[h & g->strmask]);
   if (LJ_LIKELY((((uintptr_t)str+len-1) & (LJ_PAGESIZE-1)) <= LJ_PAGESIZE-4)) {
