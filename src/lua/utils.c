/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"

#include <string.h>

#include <lua/lua.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <lua/lstate.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lua_utils, LOG_LEVEL_DBG);

#include "modules.h"

int lua_utils_string_test(void)
{
	int res;
	printk("LUA starting ...\n");

	lua_writestring(LUA_COPYRIGHT, strlen(LUA_COPYRIGHT));
	lua_writeline();

	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	luaL_loadstring(L, "print('Hello from LUA running on Zephyr RTOS !')");
	res = lua_pcall(L, 0, LUA_MULTRET, 0);
	lua_close(L);

	printk("LUA done ... : %d\n", res);

	return 0;
}

int lua_utils_execute_fs_script(const char *name)
{
	int res;

	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

	res = luaL_loadfile(L, name);
	lua_pcall(L, 0, LUA_MULTRET, 0);

	lua_close(L);

	return res;
}

int lua_utils_execute_fs_script2(const char *name)
{
	int res;

	lua_State *L = luaL_newstate();

	lm_openlibs(L);
	
	luaL_loadfile(L, name);
	res = lua_pcall(L, 0, LUA_MULTRET, 0);

	switch(res) {
		case LUA_OK:
			LOG_INF("LUA_OK");
			break;
		case LUA_YIELD:
			LOG_WRN("LUA_YIELD");
			break;
		case LUA_ERRRUN:
			LOG_WRN("LUA_ERRRUN");
			break;
		case LUA_ERRSYNTAX:
			LOG_WRN("LUA_ERRSYNTAX");
			break;
		case LUA_ERRMEM:
			LOG_WRN("LUA_ERRMEM");
			break;
		case LUA_ERRERR:
			LOG_WRN("LUA_ERRERR");
			break;
		default:
			LOG_WRN("Unknown error, res=%d", res);
	}

	lua_close(L);

	return res;
}