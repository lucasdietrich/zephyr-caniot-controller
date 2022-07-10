
zephyr_sources(devices.c data.c room.c utils.c ha_controller.c)
zephyr_sources_ifdef(CONFIG_BLE_CONTROLLER ble_controller.c)
zephyr_sources_ifdef(CONFIG_CANIOT_CONTROLLER caniot_controller.c devices/garage.c)

# FILE(GLOB_RECURSE app_sources *.c*)
# target_sources(app PRIVATE ${app_sources})