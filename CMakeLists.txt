cmake_minimum_required(VERSION 3.13.1)

# set following variables before calling the cmake command
if (NOT DEFINED BOARD)
    set(BOARD nucleo_f429zi)
endif()

if (BOARD STREQUAL "nucleo_f429zi")
    set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/lua ${CMAKE_CURRENT_SOURCE_DIR}/modules/uart_ipc ${CMAKE_CURRENT_SOURCE_DIR}/modules/caniot-lib)
    set(DTC_OVERLAY_FILE "boards/nucleo_f429zi.overlay")
    set(CONF_FILE "prj.conf")
    set(OVERLAY_CONFIG "boards/nucleo_f429zi.conf boards/f429zi_spi_disk.conf ${OVERLAY_CONFIG}")
elseif(BOARD STREQUAL "qemu_x86")
    set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/lua ${CMAKE_CURRENT_SOURCE_DIR}/modules/caniot-lib)
    set(DTC_OVERLAY_FILE "boards/qemu_x86.overlay")
    set(CONF_FILE "prj.conf")
    set(OVERLAY_CONFIG "boards/qemu_x86.conf ${OVERLAY_CONFIG}")
endif()

find_package(Zephyr)

project(caniot-controller)

# ======= SOURCE FILES =======
add_subdirectory(src)

# ======= INCLUDE FILES =======
zephyr_include_directories(src/mbedtls_config)
zephyr_include_directories(src)
