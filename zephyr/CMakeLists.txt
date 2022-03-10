cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(caniot-controller)

FILE(GLOB app_sources ../src/*.c*)
target_sources(app PRIVATE ${app_sources})
zephyr_include_directories(../src/mbedtls_config)


# For custome sections
# DATA_SECTIONS for RAM, SECTIONS for ROM
zephyr_linker_sources(SECTIONS iterables.ld)