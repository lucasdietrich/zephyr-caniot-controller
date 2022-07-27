#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#


target_sources(app PRIVATE devices.c data.c room.c utils.c ha_controller.c)
target_sources_ifdef(CONFIG_BLE_CONTROLLER app PRIVATE ble_controller.c)
target_sources_ifdef(CONFIG_CANIOT_CONTROLLER app PRIVATE caniot_controller.c devices/garage.c)