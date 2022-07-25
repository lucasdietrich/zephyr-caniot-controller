/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LUA_UTILS_H_
#define _LUA_UTILS_H_

#include <zephyr.h>
#include <assert.h>

int lua_utils_string_test(void);

int lua_utils_execute_script(const char *name);

#endif /* _LUA_ORCHESTRATOR_H_ */