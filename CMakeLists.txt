cmake_minimum_required(VERSION 3.13.1)

# set the variables before calling the cmake command
set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/caniot-lib)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(caniot-controller)

# ======= SOURCE FILES =======
add_subdirectory(src)

# ======= INCLUDE FILES =======
zephyr_include_directories(src/mbedtls_config)
zephyr_include_directories(src)
