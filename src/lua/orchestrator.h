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

/* TODO add run context */
/**
 * @brief Execute a LUA script
 * 
 * @param path 
 * @param lua_ret 
 * @return int 
 */
int lua_orch_run_script(const char *path, int *lua_ret);

int lua_orch_status(int handle);

int lua_orch_kill(int handle);


#endif /* _LUA_ORCHESTRATOR_H_ */