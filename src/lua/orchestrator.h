/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LUA_ORCHESTRATOR_H_
#define _LUA_ORCHESTRATOR_H_

#include <zephyr.h>
#include <assert.h>

int lua_orch_init(void);

int lua_orch_run_script(const char *path);

int lua_orch_status(int handle);

int lua_orch_kill(int handle);


#endif /* _LUA_ORCHESTRATOR_H_ */