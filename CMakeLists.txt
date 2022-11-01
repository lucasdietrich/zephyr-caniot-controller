#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

# Read: https://github.com/zephyrproject-rtos/zephyr/blob/zephyr-v3.0.0/cmake/extensions.cmake#L33-L43

cmake_minimum_required(VERSION 3.13.1)

# Set where zephyr should look for custom boards
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.)

# set following variables before calling the cmake command
if (NOT DEFINED BOARD)
    set(BOARD nucleo_f429zi)
endif()

# Following variables can be forced if required
#  set(DTC_OVERLAY_FILE "boards/nucleo_f429zi.overlay")
#  set(CONF_FILE "prj.conf")
# set(OVERLAY_CONFIG "boards/nucleo_f429zi.conf boards/f429zi_spi_disk.conf ${OVERLAY_CONFIG}")
if (BOARD STREQUAL "nucleo_f429zi")
    set(OVERLAY_CONFIG "overlays/f429zi_spi_disk.conf overlays/mcuboot.conf ${OVERLAY_CONFIG}")
endif()

# Debug.conf
# set(OVERLAY_CONFIG "overlays/debug.conf ${OVERLAY_CONFIG}")

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/lua ${CMAKE_CURRENT_SOURCE_DIR}/modules/uart_ipc ${CMAKE_CURRENT_SOURCE_DIR}/modules/caniot-lib)

find_package(Zephyr)

project(caniot-controller)

# This is terrible, try to move this to a dedicated CMakeLists.txt file
# 
set(embedded_lua_scripts "")
if(DEFINED CONFIG_LUA_FS_DEFAULT_SCRIPTS)
    file(GLOB luascripts "${CMAKE_SOURCE_DIR}/scripts/lua/*.lua")
    file(GLOB officialscripts "${CMAKE_SOURCE_DIR}/modules/lua/testes/*.lua")
    list (APPEND luascripts ${officialscripts})
    foreach(file ${luascripts})
        get_filename_component(filename ${file} NAME)
        get_filename_component(filepath ${file} PATH)
        message(STATUS "embedding ${filepath}/${filename} into elf")
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/embluaobj/${filename}.o
            WORKING_DIRECTORY ${filepath}
            COMMAND ${CMAKE_OBJCOPY}
            ARGS -I binary --output-target elf32-i386 --binary-architecture i386 ${filename} ${CMAKE_CURRENT_BINARY_DIR}/embluaobj/${filename}.o
        )
        # Find a way to automatically choose output-target and binary-architecture
        #   e.g. using : $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>
        list(APPEND embedded_lua_scripts ${CMAKE_CURRENT_BINARY_DIR}/embluaobj/${filename}.o)
    endforeach()
endif()

target_sources(app PRIVATE ${embedded_lua_scripts})

zephyr_linker_sources(SECTIONS iterables.ld)


# ======= SOURCE FILES =======
add_subdirectory(src)

# ======= INCLUDE FILES =======
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/mbedtls_config)

target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE modules)