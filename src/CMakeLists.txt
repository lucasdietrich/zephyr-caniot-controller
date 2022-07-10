# add_subdirectory(canif)
# add_subdirectory(cantcp)
# add_subdirectory(cantest)

add_subdirectory(discovery_server)
add_subdirectory_ifndef(CONFIG_BOARD_QEMU_X86 drivers)
add_subdirectory(ha)
add_subdirectory(http_server)
add_subdirectory_ifndef(CONFIG_BOARD_QEMU_X86 userio)
add_subdirectory(utils)
add_subdirectory_ifndef(CONFIG_BOARD_QEMU_X86 settings)

# zephyr_sources_ifdef(CONFIG_BOARD_QEMU_X86 )

FILE(GLOB app_sources *.c*)
target_sources(app PRIVATE ${app_sources})