cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(caniot-controller)

# library caniotlib
set(CANIOTLIB_PATH "${PROJECT_SOURCE_DIR}/lib/caniot-lib")
add_subdirectory(${CANIOTLIB_PATH})
zephyr_include_directories(lib/caniot-lib/include)
# TODO debug why different printf functions are used by caniotlib and application
target_compile_definitions(caniotlib PRIVATE CONFIG_CANIOT_LOG_LEVEL=0) 
target_compile_definitions(caniotlib PRIVATE CONFIG_CANIOT_ARCH_AGNOSTIC=0)
# target_compile_definitions(caniotlib PRIVATE __ZEPHYR__)

FILE(GLOB_RECURSE app_sources src/*.c*)
target_sources(app PRIVATE ${app_sources})

zephyr_include_directories(src/mbedtls_config)
zephyr_include_directories(src)

target_link_libraries(app PUBLIC caniotlib)

# target_compile_definitions(app PRIVATE __FIRMWARE_VERSION__ 1)