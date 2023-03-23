#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#


target_sources(app PRIVATE ha_controller.c datalogger.c json.c)

target_sources_ifdef(CONFIG_APP_HA_EMULATED_DEVICES app PRIVATE emu.c)
target_sources_ifdef(CONFIG_APP_HA_CANIOT_CONTROLLER app PRIVATE caniot_controller.c)

add_subdirectory(core)
add_subdirectory(devices)