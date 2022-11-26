#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

# add_subdirectory(cantcp)
# add_subdirectory(cantest)

add_subdirectory_ifdef(CONFIG_APP_DISCOVERY_SERVER discovery_server)
add_subdirectory_ifdef(CONFIG_APP_HA ha)
add_subdirectory(utils)
add_subdirectory(libs)
add_subdirectory(crypto)
add_subdirectory_ifdef(CONFIG_APP_CREDENTIALS_MANAGER creds)
add_subdirectory_ifdef(CONFIG_APP_CLOUD cloud)
add_subdirectory_ifdef(CONFIG_APP_BLE_INTERFACE ble)
add_subdirectory_ifdef(CONFIG_APP_CAN_INTERFACE can)
add_subdirectory_ifdef(CONFIG_APP_HTTP_SERVER http_server)
add_subdirectory_ifdef(CONFIG_LUA lua)
add_subdirectory_ifndef(CONFIG_QEMU_TARGET userio)
add_subdirectory_ifndef(CONFIG_QEMU_TARGET drivers)
add_subdirectory_ifdef(CONFIG_APP_DFU dfu)

FILE(GLOB app_sources *.c*)
target_sources(app PRIVATE ${app_sources})