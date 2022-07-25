/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#include <string.h>


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