#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

# Compile utils.c in "zephyr_interface" target context, so that "sig_atomic_t"
# can be found in included headers.
zephyr_sources("utils.c")
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/modules)

target_sources(app PRIVATE "orchestrator.c" "libc_stubs.c" "modules.c")

target_sources_ifdef(CONFIG_APP_LUA_FS_DEFAULT_SCRIPTS app PRIVATE "emblua.c")