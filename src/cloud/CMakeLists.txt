#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(core)

target_sources(app PRIVATE cloud_app.c)
target_sources_ifdef(CONFIG_APP_AWS app PRIVATE aws.c)