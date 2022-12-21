#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

target_sources(app PRIVATE xiaomi.c f429zi.c garage.c)

target_sources_ifdef(CONFIG_CANIOT_LIB app PRIVATE caniot.c)