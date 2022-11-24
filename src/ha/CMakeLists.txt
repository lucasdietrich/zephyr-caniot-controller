#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#


target_sources(app PRIVATE devices.c room.c utils.c ha_controller.c datalogger.c json.c data.c)
target_sources(app PRIVATE devices/xiaomi.c devices/f429zi.c devices/garage.c)
target_sources(app PRIVATE config.c)
target_sources_ifdef(CONFIG_CANIOT_LIB app PRIVATE devices/caniot.c)
target_sources_ifdef(CONFIG_APP_HA_EMULATED_DEVICES app PRIVATE emu.c)
target_sources_ifdef(CONFIG_APP_BLE_CONTROLLER app PRIVATE ble_controller.c)
target_sources_ifdef(CONFIG_APP_CANIOT_CONTROLLER app PRIVATE caniot_controller.c)