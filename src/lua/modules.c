/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#include <zephyr/kernel.h>

#include "modules.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lua_mod, LOG_LEVEL_DBG);

/* Default modules */
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

/* Custom modules */
#define LUA_DUMMYLIB_ENABLED		1
#define LUA_HA_ENABLED			1
#define LUA_ZEPHYRLIB_ENABLED		0
#define LUA_CLOUDLIB_ENABLED		0

#if LUA_ZEPHYRLIB_ENABLED && LUA_OSLIBNAME_ENABLED
#	warning "LUA_OSLIBNAME_ENABLED and LUA_ZEPHYRLIB_ENABLED are mutually exclusive"
#endif

#if !LUA_LOADLIBNAME_ENABLED
#	warning "Unable to STDOUT print with LUA_GNAME_ENABLED disabled"
#endif 

#if !LUA_LOADLIBNAME_ENABLED
#	warning "Unable to load modules from LUA script with LUA_LOADLIBNAME_ENABLED disabled"
#endif 


#define LM_LUA_DUMMYLIB "dummy"
#define LM_LUA_HALIB "ha"
#define LM_LUA_ZEPHYRLIB "zephyr"
#define LM_LUA_CLOUDLIB "cloud"



static int lm_dum_hello(lua_State *L)
{
	size_t len; 
	const char *name = luaL_checklstring(L, 1, &len);

	LOG_DBG("name=%s [len = %u]", name, len);
	
	char buffer[30u];
	snprintf(buffer, sizeof(buffer), "Hello %s", name);

	lua_pushstring(L, buffer);
	return 1;
}

static int lm_dum_add(lua_State *L)
{
	/* Sum all integers arguments and return the result */
	int i = 1;
	int sum = 0;
	while (lua_isnumber(L, i)) {
		sum += lua_tointeger(L, i);
		i++;
	}
	lua_pushinteger(L, sum);
	return 1;
}

static int lm_dum_misc(lua_State *L)
{
	/* Get the first argument and return it */
	return lua_gettop(L);
}

static const struct luaL_Reg lm_dummy_functions[] = {
  {"hello", lm_dum_hello},
  {"add", lm_dum_add},
  {"misc", lm_dum_misc},
  {NULL, NULL}
};

static int lm_luaopen_dummy(lua_State *L) {
	luaL_newlib(L, lm_dummy_functions);
	return 1;
}

/* TODO https://stackoverflow.com/questions/12096281/how-to-have-a-lua-iterator-return-a-c-struct */
static int lm_ha_list_devices(lua_State *L)
{
	/* Get the first argument and return it */
	return 1;
}

static const struct luaL_Reg lm_ha_functions[] = {
	{"devices", lm_ha_list_devices},
	{"rooms", NULL},
	{"subscribe", NULL},
	{"pend", NULL},
	{"command", NULL},
	{"can", NULL},
	{NULL, NULL}
};

static int lm_luaopen_ha(lua_State *L) {
	luaL_newlib(L, lm_ha_functions);
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
#if LUA_HA_ENABLED == 1
  {LM_LUA_HALIB, lm_luaopen_ha},
#endif
#if LUA_ZEPHYRLIB_ENABLED == 1
  {LM_LUA_ZEPHYRLIB, NULL},
#endif
#if LUA_CLOUDLIB_ENABLED == 1
  {LM_LUA_CLOUDLIB, NULL},
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
