/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LUA_UTILS_H_
#define _LUA_UTILS_H_

#include <assert.h>

#include <zephyr/kernel.h>

int lua_utils_string_test(void);

int lua_utils_execute_fs_script(const char *name);

const char *lua_utils_luaret2str(int lua_ret);

#endif /* _LUA_ORCHESTRATOR_H_ */