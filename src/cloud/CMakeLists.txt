#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(core)
add_subdirectory(platforms)

target_sources(app PRIVATE cloud_app.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})