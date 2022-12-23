#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

FILE(GLOB_RECURSE sources *.c*)
target_sources(app PRIVATE ${sources})