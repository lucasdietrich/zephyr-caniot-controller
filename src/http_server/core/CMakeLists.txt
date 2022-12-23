#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

FILE(GLOB_RECURSE http_server_core_sources *.c*)
target_sources(app PRIVATE ${http_server_core_sources})