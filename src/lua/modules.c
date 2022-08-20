/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#include <zephyr.h>

#include "modules.h"

#define LUA_GNAME_ENABLED		1
#define LUA_LOADLIBNAME_ENABLED		1
#define LUA_COLIBNAME_ENABLED		0
#define LUA_TABLIBNAME_ENABLED		1
#define LUA_IOLIBNAME_ENABLED		1
#define LUA_OSLIBNAME_ENABLED		1
#define LUA_STRLIBNAME_ENABLED		1
#define LUA_MATHLIBNAME_ENABLED		1
#define LUA_UTF8LIBNAME_ENABLED		0
#define LUA_DBLIBNAME_ENABLED		0

#define LUA_DUMMYLIB_ENABLED		1

#if !LUA_LOADLIBNAME_ENABLED
#	warning "Unable to STDOUT print with LUA_GNAME_ENABLED disabled"
#endif 

#if !LUA_LOADLIBNAME_ENABLED
#	warning "Unable to load modules from LUA \
		script with LUA_LOADLIBNAME_ENABLED disabled"
#endif 




#define LM_LUA_DUMMYLIB "dummy"

static int lm_dum_hello_world(lua_State *L) {
    lua_pushstring(L, "Hello World from module !");
    printk("Called\n");
    return lua_gettop(L);
}

static const struct luaL_Reg lm_dummy_functions[] = {
  {"myhelloworld", lm_dum_hello_world},
  {NULL, NULL}
};

static int lm_luaopen_dummy (lua_State *L) {
  luaL_newlib(L, lm_dummy_functions);
  return 1;
}




static const luaL_Reg lm_lua_modules[] = {
#if LUA_GNAME_ENABLED == 1
  {LUA_GNAME, luaopen_base},
#endif 
#if LUA_LOADLIBNAME_ENABLED == 1
  {LUA_LOADLIBNAME, luaopen_package},
#endif 
#if LUA_COLIBNAME_ENABLED == 1
  {LUA_COLIBNAME, luaopen_coroutine},
#endif 
#if LUA_TABLIBNAME_ENABLED == 1
  {LUA_TABLIBNAME, luaopen_table},
#endif 
#if LUA_IOLIBNAME_ENABLED == 1
  {LUA_IOLIBNAME, luaopen_io},
#endif 
#if LUA_OSLIBNAME_ENABLED == 1
  {LUA_OSLIBNAME, luaopen_os},
#endif 
#if LUA_STRLIBNAME_ENABLED == 1
  {LUA_STRLIBNAME, luaopen_string},
#endif 
#if LUA_MATHLIBNAME_ENABLED == 1
  {LUA_MATHLIBNAME, luaopen_math},
#endif 
#if LUA_UTF8LIBNAME_ENABLED == 1
  {LUA_UTF8LIBNAME, luaopen_utf8},
#endif 
#if LUA_DBLIBNAME_ENABLED == 1
  {LUA_DBLIBNAME, luaopen_debug},
#endif 
#if LUA_DUMMYLIB_ENABLED == 1
  {LM_LUA_DUMMYLIB, lm_luaopen_dummy},
#endif
  {NULL, NULL}
};

/* See "luaL_openlibs" */
void lm_openlibs(lua_State *L) {
	const luaL_Reg *lib;
	for (lib = lm_lua_modules; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}
}
