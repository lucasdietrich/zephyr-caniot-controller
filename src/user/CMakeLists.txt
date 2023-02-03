#
# Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

FILE(GLOB_RECURSE app_sources *.c*)
target_sources(app PRIVATE ${app_sources})