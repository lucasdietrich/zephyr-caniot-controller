/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "orchestrator.h"

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#include "app_sections.h"
#include "modules.h"
#include "utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(lua, LOG_LEVEL_DBG);

#define CONFIG_LUA_ORCHESTRATOR_WORK_Q_STACK_SIZE 	4096u
#define CONFIG_LUA_ORCHESTRATOR_CONTEXTS_COUNT 		2u
#define CONFIG_LUA_ORCHESTRATOR_WORK_Q_PRIORITY		K_PRIO_COOP(5)

// __buf_noinit_section
K_THREAD_STACK_DEFINE(work_q_stack, CONFIG_LUA_ORCHESTRATOR_WORK_Q_STACK_SIZE);

static struct k_work_q lua_work_q;

int lua_orch_init(void)
{
	k_work_queue_init(&lua_work_q);
	
	/* Workqueue should yield between scripts execution (yield by default) */
	k_work_queue_start(&lua_work_q, work_q_stack,
			   K_THREAD_STACK_SIZEOF(work_q_stack),
			   CONFIG_LUA_ORCHESTRATOR_WORK_Q_PRIORITY, NULL);

	return 0;
}

typedef enum {
	LUA_ORCH_RET_OK = 0u,
} lua_orch_script_status_t;

struct script_context {
	struct k_work _work;
	struct k_sem _sem;
	lua_State *L;

	int lua_ret;

	/**
	 * @brief Contain exit value of the script.
	 */
	lua_orch_script_status_t res;
};

K_MEM_SLAB_DEFINE_STATIC(works_pool, sizeof(struct script_context),
			 CONFIG_LUA_ORCHESTRATOR_CONTEXTS_COUNT, 4u);


void lua_orch_script_handler(struct k_work *work)
{
	struct script_context *const sx =
		CONTAINER_OF(work, struct script_context, _work);

	LOG_DBG("(%p) Executing script ...", sx);

	/* Init LUA context */
	sx->lua_ret = lua_pcall(sx->L, 0, LUA_MULTRET, 0);

	/* TODO Handle script returned values */
	lua_close(sx->L);

	LOG_DBG("(%p) Script returned res=%d...", sx, sx->lua_ret);

	/* Free context */
	k_sem_give(&sx->_sem);
}

int lua_orch_run_script(const char *path, int *lua_ret)
{
	int res;
	struct script_context *sx;

	/* Allocate context */
	res = k_mem_slab_alloc(&works_pool, (void **)&sx, K_NO_WAIT);
	if (res == 0) {
		/* Context init */
		k_work_init(&sx->_work, lua_orch_script_handler);
		k_sem_init(&sx->_sem, 0u, 1u);
		sx->res = LUA_ORCH_RET_OK;
		sx->lua_ret = 0u;

		/* Init LUA context */
		sx->L = luaL_newstate();
		lm_openlibs(sx->L);
		res = luaL_loadfile(sx->L, path);
		if (res != LUA_OK) {
			goto exit;
		}

		/* Schedule script execution */
		k_work_submit_to_queue(&lua_work_q, &sx->_work);

		/* Wait for script end */
		k_sem_take(&sx->_sem, K_FOREVER);

		/* Forward lua return value */
		if (lua_ret != NULL) {
			*lua_ret = sx->lua_ret;
		}

		/* Check for script execution error */
		if (sx->lua_ret >= LUA_ERRRUN) {
			LOG_WRN("(%p) Script returned error %d (%s)",
				sx, sx->lua_ret, lua_utils_luaret2str(sx->lua_ret));
		} else {
			LOG_INF("(%p) Script returned %d (%s)",
				sx, sx->lua_ret, lua_utils_luaret2str(sx->lua_ret));
		}
	}

exit:
	if (res != 0) {
		lua_close(sx->L);
		LOG_ERR("(%p) Script execution failed", sx);
	}

	k_mem_slab_free(&works_pool, (void **)&sx);

	return res;
}