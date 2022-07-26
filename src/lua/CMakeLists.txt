#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

# SET(app_sources "orchestrator.c utils.c libc_stubs.c")

zephyr_sources("orchestrator.c")
zephyr_sources("utils.c")
zephyr_sources("libc_stubs.c")

zephyr_sources_ifdef(CONFIG_APP_FS_DEFAULT_LUA_FILES fsdefault.c)

target_sources(app PRIVATE ${embedded_lua_scripts})