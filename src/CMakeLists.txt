#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

# add_subdirectory(canif)
# add_subdirectory(cantcp)
# add_subdirectory(cantest)

add_subdirectory_ifdef(CONFIG_DISCOVERY_SERVER discovery_server)
add_subdirectory(ha)
add_subdirectory(utils)
add_subdirectory(libs)
add_subdirectory_ifdef(CONFIG_HTTP_SERVER http_server)
add_subdirectory_ifdef(CONFIG_LUA lua)
add_subdirectory_ifndef(CONFIG_QEMU_TARGET userio)
add_subdirectory_ifndef(CONFIG_QEMU_TARGET drivers)
add_subdirectory_ifndef(CONFIG_QEMU_TARGET settings)

FILE(GLOB app_sources *.c*)
target_sources(app PRIVATE ${app_sources})