# Configurations

Use script [scripts/zephyr_conf_parser.py](./scripts/zephyr_conf_parser.py) to
generate the list of all configurations options for a given board.

Export the generated csv to markdown to generate the following table.

## Different configurations

Following table describes configuration differences between different boards
in explicit configuration files.

Following configuration has been used for this table:

```python
prj = "prj.conf"
boards = {
    "nucleo_f429zi": [
        prj,
        "boards/nucleo_f429zi.conf",
        "overlays/nucleo_f429zi_spi_disk.conf",
        "overlays/nucleo_f429zi_mcuboot.conf",
    ],
    "mps2_an385": [
        prj,
        "boards/mps2_an385.conf",
        "overlays/mps2_an385_slip.conf",
    ],
    "qemu_x86": [
        prj,
        "boards/qemu_x86.conf",
    ],
}
```

|option                                 |nucleo_f429zi|mps2_an385|qemu_x86|
|---------------------------------------|-------------|----------|--------|
|CONFIG_ADC                             |True         |          |        |
|CONFIG_ADC_LOG_LEVEL_DBG               |False        |          |        |
|CONFIG_APP_BLE_CONTROLLER              |True         |False     |False   |
|CONFIG_APP_CREDS_FLASH                 |True         |False     |False   |
|CONFIG_APP_CREDS_HARDCODED             |False        |True      |True    |
|CONFIG_APP_DFU                         |True         |          |        |
|CONFIG_APP_HA                          |True         |          |        |
|CONFIG_APP_HA_EMULATED_DEVICES         |False        |True      |True    |
|CONFIG_APP_HTTP_TEST                   |False        |True      |True    |
|CONFIG_APP_HTTP_TEST_SERVER            |False        |True      |True    |
|CONFIG_APP_PRINTF_1SEC_COUNTER         |             |False     |        |
|CONFIG_ASSERT                          |False        |True      |True    |
|CONFIG_BOOTLOADER_MCUBOOT              |True         |          |        |
|CONFIG_CAN                             |True         |          |        |
|CONFIG_CANIOT_MAX_PENDING_QUERIES      |6            |12        |12      |
|CONFIG_CAN_INIT_PRIORITY               |80           |          |        |
|CONFIG_CAN_LOG_LEVEL_DBG               |False        |          |        |
|CONFIG_CAN_LOOPBACK                    |False        |          |        |
|CONFIG_DISK_DRIVER_SDMMC               |True         |          |        |
|CONFIG_DMA_LOG_LEVEL_DBG               |False        |          |        |
|CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR |True         |False     |False   |
|CONFIG_ETH_STM32_HAL_MAC3              |119          |          |        |
|CONFIG_ETH_STM32_HAL_MAC4              |119          |          |        |
|CONFIG_ETH_STM32_HAL_MAC5              |119          |          |        |
|CONFIG_ETH_STM32_HAL_RANDOM_MAC        |False        |          |        |
|CONFIG_FS_FATFS_MOUNT_MKFS             |False        |          |        |
|CONFIG_GPIO                            |True         |          |        |
|CONFIG_HEAP_MEM_POOL_SIZE              |2048         |4096      |2048    |
|CONFIG_IMG_BLOCK_BUF_SIZE              |512          |          |        |
|CONFIG_IMG_ENABLE_IMAGE_CHECK          |True         |          |        |
|CONFIG_IMG_ERASE_PROGRESSIVELY         |True         |          |        |
|CONFIG_IMG_MANAGER                     |True         |          |        |
|CONFIG_LOG_BACKEND_FS                  |True         |          |        |
|CONFIG_LOG_BACKEND_FS_DIR              |/SD:/logs    |          |        |
|CONFIG_LOG_BACKEND_FS_FILES_LIMIT      |100          |          |        |
|CONFIG_LOG_BACKEND_FS_FILE_PREFIX      |zephyr_      |          |        |
|CONFIG_LOG_BACKEND_FS_FILE_SIZE        |1048576      |          |        |
|CONFIG_LOG_BACKEND_FS_OUTPUT_TEXT      |True         |          |        |
|CONFIG_LOG_BACKEND_FS_OVERWRITE        |True         |          |        |
|CONFIG_LOG_MODE_IMMEDIATE              |False        |False     |True    |
|CONFIG_MAIN_STACK_SIZE                 |2048         |4096      |4096    |
|CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS      |--pad --ve...|          |        |
|CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE|False        |          |        |
|CONFIG_MCUBOOT_IMG_MANAGER             |True         |          |        |
|CONFIG_MCUBOOT_SIGNATURE_KEY_FILE      |zephyr-can...|          |        |
|CONFIG_MY_STM32_HAL                    |True         |          |        |
|CONFIG_NET_BUF_DATA_SIZE               |128          |236       |236     |
|CONFIG_NET_BUF_RX_COUNT                |64           |256       |128     |
|CONFIG_NET_BUF_TX_COUNT                |64           |256       |128     |
|CONFIG_NET_CONFIG_AUTO_INIT            |False        |True      |True    |
|CONFIG_NET_CONFIG_NEED_IPV4            |False        |True      |True    |
|CONFIG_NET_CONFIG_NEED_IPV6            |False        |True      |True    |
|CONFIG_NET_CONFIG_SETTINGS             |False        |True      |True    |
|CONFIG_NET_DHCPV4                      |True         |False     |False   |
|CONFIG_NET_DHCPV4_INITIAL_DELAY_MAX    |2            |          |        |
|CONFIG_NET_DHCPV4_LOG_LEVEL_DBG        |False        |          |        |
|CONFIG_NET_PKT_RX_COUNT                |64           |256       |128     |
|CONFIG_NET_PKT_TX_COUNT                |64           |256       |128     |
|CONFIG_NET_QEMU_ETHERNET               |             |False     |        |
|CONFIG_PWM                             |True         |          |        |
|CONFIG_PWM_LOG_LEVEL_DBG               |True         |          |        |
|CONFIG_SDHC_LOG_LEVEL_DBG              |False        |          |        |
|CONFIG_SDMMC_LOG_LEVEL_INF             |True         |          |        |
|CONFIG_SDMMC_VOLUME_NAME               |SD           |          |        |
|CONFIG_SENSOR                          |True         |          |        |
|CONFIG_SERIAL                          |True         |          |        |
|CONFIG_SPI                             |True         |          |        |
|CONFIG_SPI_LOG_LEVEL_INF               |True         |          |        |
|CONFIG_STM32_TEMP                      |True         |          |        |
|CONFIG_TEST_RANDOM_GENERATOR           |False        |True      |True    |
|CONFIG_UART_ASYNC_API                  |True         |          |        |
|CONFIG_UART_IPC                        |True         |          |        |
|CONFIG_UART_IPC_DEBUG_GPIO_STM32       |False        |          |        |
|CONFIG_UART_IPC_EVENT_API              |True         |          |        |
|CONFIG_UART_IPC_FULL                   |True         |          |        |
|CONFIG_UART_IPC_PING                   |True         |          |        |
|CONFIG_UART_IPC_PING_PERIOD            |1000         |          |        |
|CONFIG_UART_IPC_STATS                  |True         |          |        |
|CONFIG_UART_IPC_THREAD_START_DELAY     |5000         |          |        |
|CONFIG_USE_DT_CODE_PARTITION           |True         |          |        |

## All application configuration options only

|option                                     |nucleo_f429zi|mps2_an385|qemu_x86|
|-------------------------------------------|-------------|----------|--------|
|CONFIG_APP_BLE_CONTROLLER                  |True         |False     |False   |
|CONFIG_APP_CANIOT_CONTROLLER               |True         |True      |True    |
|CONFIG_APP_CANTCP_SERVER                   |False        |False     |False   |
|CONFIG_APP_CAN_INTERFACE                   |             |False     |False   |
|CONFIG_APP_CLOUD                           |False        |False     |False   |
|CONFIG_APP_CREDS_FLASH                     |True         |False     |False   |
|CONFIG_APP_CREDS_FS                        |False        |False     |False   |
|CONFIG_APP_CREDS_HARDCODED                 |False        |True      |True    |
|CONFIG_APP_DFU                             |True         |          |        |
|CONFIG_APP_DISCOVERY_SERVER                |True         |True      |True    |
|CONFIG_APP_FILE_ACCESS_HISTORY             |             |False     |False   |
|CONFIG_APP_HA                              |True         |          |        |
|CONFIG_APP_HA_EMULATED_DEVICES             |False        |True      |True    |
|CONFIG_APP_HTTP_MAX_SESSIONS               |8            |8         |8       |
|CONFIG_APP_HTTP_REQUEST_HEADERS_BUFFER_SIZE|352          |352       |352     |
|CONFIG_APP_HTTP_SERVER                     |True         |True      |True    |
|CONFIG_APP_HTTP_SERVER_NONSECURE           |True         |True      |True    |
|CONFIG_APP_HTTP_SERVER_SECURE              |True         |True      |True    |
|CONFIG_APP_HTTP_SERVER_VERIFY_CLIENT       |False        |False     |False   |
|CONFIG_APP_HTTP_TEST                       |False        |True      |True    |
|CONFIG_APP_HTTP_TEST_SERVER                |False        |True      |True    |
|CONFIG_APP_LUA_AUTORUN_SCRIPTS             |             |False     |False   |
|CONFIG_APP_LUA_FS_DEFAULT_SCRIPTS          |             |True      |True    |
|CONFIG_APP_MBEDTLS_CUSTOM_HEAP_CCM         |             |False     |False   |
|CONFIG_APP_MBEDTLS_CUSTOM_HEAP_SIZE        |             |65536     |65536   |
|CONFIG_APP_PRINTF_1SEC_COUNTER             |             |False     |        |
|CONFIG_APP_SYSTEM_MONITORING               |True         |True      |True    |
