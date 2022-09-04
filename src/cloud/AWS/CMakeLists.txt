#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

file(GLOB app_sources *.c)
target_sources(app PRIVATE ${app_sources})