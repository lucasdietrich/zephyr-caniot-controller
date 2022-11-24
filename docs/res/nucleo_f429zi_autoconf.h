#define CONFIG_APP_HTTP_SERVER_NONSECURE 1
#define CONFIG_APP_HTTP_MAX_SESSIONS 3
#define CONFIG_APP_HTTP_REQUEST_HEADERS_BUFFER_SIZE 352
#define CONFIG_APP_HTTP_TEST 1
#define CONFIG_APP_HTTP_TEST_SERVER 1
#define CONFIG_FILE_UPLOAD_MOUNT_POINT "/SD:"
#define CONFIG_APP_LUA_FS_SCRIPTS_DIR "/SD:/lua"
#define CONFIG_APP_DISCOVERY_SERVER 1
#define CONFIG_APP_DISCOVERY_SERVER_LOG_LEVEL 2
#define CONFIG_APP_BLE_CONTROLLER 1
#define CONFIG_APP_SYSTEM_MONITORING 1
#define CONFIG_APP_CANIOT_CONTROLLER 1
#define CONFIG_MY_STM32_HAL 1
#define CONFIG_APP_MBEDTLS_CUSTOM_HEAP_SIZE 65536
#define CONFIG_APP_MBEDTLS_CUSTOM_HEAP_CCM 1
#define CONFIG_GPIO 1
#define CONFIG_SPI 1
#define CONFIG_DISK_DRIVER_SDMMC 1
#define CONFIG_CAN_INIT_PRIORITY 80
#define CONFIG_NET_L2_ETHERNET 1
#define CONFIG_ADC_INIT_PRIORITY 50
#define CONFIG_GPIO_INIT_PRIORITY 40
#define CONFIG_NET_IPV6 1
#define CONFIG_BOARD "nucleo_f429zi"
#define CONFIG_SOC "stm32f429xx"
#define CONFIG_SOC_SERIES "stm32f4"
#define CONFIG_NUM_IRQS 91
#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 168000000
#define CONFIG_CLOCK_CONTROL_INIT_PRIORITY 1
#define CONFIG_HEAP_MEM_POOL_SIZE 0
#define CONFIG_ROM_START_OFFSET 0x0
#define CONFIG_PINCTRL 1
#define CONFIG_CORTEX_M_SYSTICK 1
#define CONFIG_CLOCK_CONTROL 1
#define CONFIG_SYS_CLOCK_TICKS_PER_SEC 10000
#define CONFIG_BUILD_OUTPUT_HEX 1
#define CONFIG_SERIAL_INIT_PRIORITY 55
#define CONFIG_FLASH_SIZE 2048
#define CONFIG_FLASH_BASE_ADDRESS 0x8000000
#define CONFIG_MBEDTLS 1
#define CONFIG_TEST_EXTRA_STACKSIZE 0
#define CONFIG_SERIAL 1
#define CONFIG_CLOCK_CONTROL_STM32_CUBE 1
#define CONFIG_UART_STM32 1
#define CONFIG_GPIO_STM32 1
#define CONFIG_PWM_STM32 1
#define CONFIG_SPI_STM32 1
#define CONFIG_ADC_STM32 1
#define CONFIG_DMA_STM32 1
#define CONFIG_ENTROPY_STM32_RNG 1
#define CONFIG_MP_NUM_CPUS 1
#define CONFIG_ZEPHYR_CANOPENNODE_MODULE 1
#define CONFIG_ZEPHYR_HAL_GIGADEVICE_MODULE 1
#define CONFIG_ZEPHYR_HAL_NORDIC_MODULE 1
#define CONFIG_ZEPHYR_HAL_NXP_MODULE 1
#define CONFIG_ZEPHYR_HAL_RPI_PICO_MODULE 1
#define CONFIG_ZEPHYR_LORAMAC_NODE_MODULE 1
#define CONFIG_ZEPHYR_LZ4_MODULE 1
#define CONFIG_ZEPHYR_MBEDTLS_MODULE 1
#define CONFIG_MBEDTLS_BUILTIN 1
#define CONFIG_MBEDTLS_CFG_FILE "config-tls-generic.h"
#define CONFIG_MBEDTLS_TLS_VERSION_1_2 1
#define CONFIG_MBEDTLS_KEY_EXCHANGE_RSA_ENABLED 1
#define CONFIG_MBEDTLS_CIPHER_AES_ENABLED 1
#define CONFIG_MBEDTLS_AES_ROM_TABLES 1
#define CONFIG_MBEDTLS_CIPHER_DES_ENABLED 1
#define CONFIG_MBEDTLS_CIPHER_MODE_CBC_ENABLED 1
#define CONFIG_MBEDTLS_MAC_MD5_ENABLED 1
#define CONFIG_MBEDTLS_MAC_SHA1_ENABLED 1
#define CONFIG_MBEDTLS_MAC_SHA256_ENABLED 1
#define CONFIG_MBEDTLS_SHA256_SMALLER 1
#define CONFIG_MBEDTLS_MAC_SHA512_ENABLED 1
#define CONFIG_MBEDTLS_CTR_DRBG_ENABLED 1
#define CONFIG_MBEDTLS_CIPHER 1
#define CONFIG_MBEDTLS_MD 1
#define CONFIG_MBEDTLS_PEM_CERTIFICATE_FORMAT 1
#define CONFIG_MBEDTLS_USER_CONFIG_ENABLE 1
#define CONFIG_MBEDTLS_USER_CONFIG_FILE "mbedtls-cfg.h"
#define CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN 4096
#define CONFIG_MBEDTLS_DEBUG 1
#define CONFIG_MBEDTLS_DEBUG_LEVEL 1
#define CONFIG_MBEDTLS_MEMORY_DEBUG 1
#define CONFIG_APP_LINK_WITH_MBEDTLS 1
#define CONFIG_ZEPHYR_NANOPB_MODULE 1
#define CONFIG_ZEPHYR_SOF_MODULE 1
#define CONFIG_ZEPHYR_TFLITE_MICRO_MODULE 1
#define CONFIG_ZEPHYR_TRACERECORDER_MODULE 1
#define CONFIG_ZEPHYR_TRUSTED_FIRMWARE_M_MODULE 1
#define CONFIG_ZEPHYR_ZSCILIB_MODULE 1
#define CONFIG_LUA 1
#define CONFIG_ZEPHYR_LUA_MODULE 1
#define CONFIG_UART_IPC 1
#define CONFIG_UART_IPC_STACK_SIZE 512
#define CONFIG_UART_IPC_FRAME_BUFFER_COUNT 2
#define CONFIG_UART_IPC_DMA_BUF_SIZE 68
#define CONFIG_UART_IPC_FULL 1
#define CONFIG_UART_IPC_PING 1
#define CONFIG_UART_IPC_PING_PERIOD 1000
#define CONFIG_UART_IPC_STATS 1
#define CONFIG_UART_IPC_EVENT_API 1
#define CONFIG_UART_IPC_THREAD_PRIORITY 1
#define CONFIG_UART_IPC_THREAD_START_DELAY 0
#define CONFIG_ZEPHYR_UART_IPC_MODULE 1
#define CONFIG_CANIOT_LIB 1
#define CONFIG_CANIOT_LOG_LEVEL 2
#define CONFIG_CANIOT_MAX_PENDING_QUERIES 6
#define CONFIG_CANIOT_CTRL_DRIVERS_API 1
#define CONFIG_ZEPHYR_CANIOT_LIB_MODULE 1
#define CONFIG_APP_HAS_CMSIS_CORE 1
#define CONFIG_APP_HAS_CMSIS_CORE_M 1
#define CONFIG_APP_HAS_STM32CUBE 1
#define CONFIG_USE_STM32_HAL_CRC 1
#define CONFIG_USE_STM32_HAL_ETH 1
#define CONFIG_USE_STM32_HAL_GPIO 1
#define CONFIG_USE_STM32_LL_DMA 1
#define CONFIG_USE_STM32_LL_RCC 1
#define CONFIG_USE_STM32_LL_RNG 1
#define CONFIG_USE_STM32_LL_SPI 1
#define CONFIG_USE_STM32_LL_TIM 1
#define CONFIG_USE_STM32_LL_UTILS 1
#define CONFIG_BOARD_NUCLEO_F429ZI 1
#define CONFIG_SOC_SERIES_STM32F4X 1
#define CONFIG_CPU_HAS_ARM_MPU 1
#define CONFIG_APP_HAS_SWO 1
#define CONFIG_SOC_FAMILY "st_stm32"
#define CONFIG_SOC_FAMILY_STM32 1
#define CONFIG_STM32_CCM 1
#define CONFIG_SOC_STM32F429XX 1
#define CONFIG_SOC_LOG_LEVEL_INF 1
#define CONFIG_SOC_LOG_LEVEL 3
#define CONFIG_ARCH "arm"
#define CONFIG_CPU_CORTEX 1
#define CONFIG_CPU_CORTEX_M 1
#define CONFIG_ISA_THUMB2 1
#define CONFIG_ASSEMBLER_ISA_THUMB2 1
#define CONFIG_COMPILER_ISA_THUMB2 1
#define CONFIG_STACK_ALIGN_DOUBLE_WORD 1
#define CONFIG_FAULT_DUMP 2
#define CONFIG_ARM_STACK_PROTECTION 1
#define CONFIG_FP16 1
#define CONFIG_FP16_IEEE 1
#define CONFIG_CPU_CORTEX_M4 1
#define CONFIG_CPU_CORTEX_M_HAS_SYSTICK 1
#define CONFIG_CPU_CORTEX_M_HAS_DWT 1
#define CONFIG_CPU_CORTEX_M_HAS_BASEPRI 1
#define CONFIG_CPU_CORTEX_M_HAS_VTOR 1
#define CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS 1
#define CONFIG_ARMV7_M_ARMV8_M_MAINLINE 1
#define CONFIG_ARMV7_M_ARMV8_M_FP 1
#define CONFIG_GEN_ISR_TABLES 1
#define CONFIG_NULL_POINTER_EXCEPTION_DETECTION_NONE 1
#define CONFIG_GEN_IRQ_VECTOR_TABLE 1
#define CONFIG_ARM_MPU 1
#define CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE 32
#define CONFIG_MPU_STACK_GUARD 1
#define CONFIG_MPU_ALLOW_FLASH_WRITE 1
#define CONFIG_CUSTOM_SECTION_MIN_ALIGN_SIZE 32
#define CONFIG_ARM 1
#define CONFIG_ARCH_IS_SET 1
#define CONFIG_ARCH_LOG_LEVEL_INF 1
#define CONFIG_ARCH_LOG_LEVEL 3
#define CONFIG_MPU_LOG_LEVEL_INF 1
#define CONFIG_MPU_LOG_LEVEL 3
#define CONFIG_SRAM_SIZE 192
#define CONFIG_SRAM_BASE_ADDRESS 0x20000000
#define CONFIG_HW_STACK_PROTECTION 1
#define CONFIG_PRIVILEGED_STACK_SIZE 1024
#define CONFIG_KOBJECT_TEXT_AREA 256
#define CONFIG_KOBJECT_DATA_AREA_RESERVE_EXTRA_PERCENT 100
#define CONFIG_KOBJECT_RODATA_AREA_EXTRA_BYTES 16
#define CONFIG_GEN_PRIV_STACKS 1
#define CONFIG_GEN_SW_ISR_TABLE 1
#define CONFIG_ARCH_SW_ISR_TABLE_ALIGN 0
#define CONFIG_GEN_IRQ_START_VECTOR 0
#define CONFIG_ARCH_HAS_SINGLE_THREAD_SUPPORT 1
#define CONFIG_ARCH_HAS_TIMING_FUNCTIONS 1
#define CONFIG_ARCH_HAS_STACK_PROTECTION 1
#define CONFIG_ARCH_HAS_USERSPACE 1
#define CONFIG_ARCH_HAS_EXECUTABLE_PAGE_BIT 1
#define CONFIG_ARCH_HAS_RAMFUNC_SUPPORT 1
#define CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION 1
#define CONFIG_ARCH_SUPPORTS_COREDUMP 1
#define CONFIG_ARCH_SUPPORTS_ARCH_HW_INIT 1
#define CONFIG_ARCH_HAS_EXTRA_EXCEPTION_INFO 1
#define CONFIG_ARCH_HAS_THREAD_LOCAL_STORAGE 1
#define CONFIG_ARCH_HAS_THREAD_ABORT 1
#define CONFIG_CPU_HAS_FPU 1
#define CONFIG_CPU_HAS_MPU 1
#define CONFIG_MPU 1
#define CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT 1
#define CONFIG_SRAM_REGION_PERMISSIONS 1
#define CONFIG_TOOLCHAIN_HAS_BUILTIN_FFS 1
#define CONFIG_KERNEL_LOG_LEVEL_INF 1
#define CONFIG_KERNEL_LOG_LEVEL 3
#define CONFIG_MULTITHREADING 1
#define CONFIG_NUM_COOP_PRIORITIES 16
#define CONFIG_NUM_PREEMPT_PRIORITIES 15
#define CONFIG_MAIN_THREAD_PRIORITY 0
#define CONFIG_COOP_ENABLED 1
#define CONFIG_PREEMPT_ENABLED 1
#define CONFIG_PRIORITY_CEILING -127
#define CONFIG_NUM_METAIRQ_PRIORITIES 0
#define CONFIG_MAIN_STACK_SIZE 2048
#define CONFIG_IDLE_STACK_SIZE 320
#define CONFIG_ISR_STACK_SIZE 2048
#define CONFIG_THREAD_STACK_INFO 1
#define CONFIG_ERRNO 1
#define CONFIG_SCHED_DUMB 1
#define CONFIG_WAITQ_DUMB 1
#define CONFIG_INIT_STACKS 1
#define CONFIG_BOOT_BANNER 1
#define CONFIG_BOOT_DELAY 0
#define CONFIG_THREAD_NAME 1
#define CONFIG_THREAD_MAX_NAME_LEN 32
#define CONFIG_INSTRUMENT_THREAD_SWITCHING 1
#define CONFIG_THREAD_RUNTIME_STATS 1
#define CONFIG_SCHED_THREAD_USAGE 1
#define CONFIG_SCHED_THREAD_USAGE_ALL 1
#define CONFIG_SCHED_THREAD_USAGE_AUTO_ENABLE 1
#define CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE 2024
#define CONFIG_SYSTEM_WORKQUEUE_PRIORITY -1
#define CONFIG_ATOMIC_OPERATIONS_BUILTIN 1
#define CONFIG_TIMESLICING 1
#define CONFIG_TIMESLICE_SIZE 0
#define CONFIG_TIMESLICE_PRIORITY 0
#define CONFIG_POLL 1
#define CONFIG_NUM_MBOX_ASYNC_MSGS 10
#define CONFIG_KERNEL_MEM_POOL 1
#define CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN 1
#define CONFIG_SWAP_NONATOMIC 1
#define CONFIG_SYS_CLOCK_EXISTS 1
#define CONFIG_TIMEOUT_64BIT 1
#define CONFIG_SYS_CLOCK_MAX_TIMEOUT_DAYS 365
#define CONFIG_XIP 1
#define CONFIG_KERNEL_INIT_PRIORITY_OBJECTS 30
#define CONFIG_KERNEL_INIT_PRIORITY_DEFAULT 40
#define CONFIG_KERNEL_INIT_PRIORITY_DEVICE 50
#define CONFIG_APPLICATION_INIT_PRIORITY 90
#define CONFIG_STACK_POINTER_RANDOM 0
#define CONFIG_TICKLESS_KERNEL 1
#define CONFIG_APP_HAS_DTS 1
#define CONFIG_APP_HAS_DTS_GPIO 1
#define CONFIG_CONSOLE 1
#define CONFIG_CONSOLE_INPUT_MAX_LINE_LEN 128
#define CONFIG_CONSOLE_HAS_DRIVER 1
#define CONFIG_CONSOLE_INIT_PRIORITY 60
#define CONFIG_UART_CONSOLE 1
#define CONFIG_UART_CONSOLE_LOG_LEVEL_INF 1
#define CONFIG_UART_CONSOLE_LOG_LEVEL 3
#define CONFIG_APP_HAS_SEGGER_RTT 1
#define CONFIG_ETHERNET_LOG_LEVEL_DEFAULT 1
#define CONFIG_ETHERNET_LOG_LEVEL 3
#define CONFIG_ETH_INIT_PRIORITY 80
#define CONFIG_ETH_STM32_HAL 1
#define CONFIG_ETH_STM32_HAL_RX_THREAD_STACK_SIZE 1500
#define CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO 2
#define CONFIG_ETH_STM32_HAL_PHY_ADDRESS 0
#define CONFIG_ETH_STM32_HAL_MAC3 0x77
#define CONFIG_ETH_STM32_HAL_MAC4 0x77
#define CONFIG_ETH_STM32_HAL_MAC5 0x77
#define CONFIG_ETH_STM32_CARRIER_CHECK_RX_IDLE_TIMEOUT_MS 500
#define CONFIG_ETH_STM32_AUTO_NEGOTIATION_ENABLE 1
#define CONFIG_PHY_LOG_LEVEL_DEFAULT 1
#define CONFIG_PHY_LOG_LEVEL 3
#define CONFIG_PHY_INIT_PRIORITY 70
#define CONFIG_PHY_AUTONEG_TIMEOUT_MS 4000
#define CONFIG_PHY_MONITOR_PERIOD 500
#define CONFIG_SERIAL_HAS_DRIVER 1
#define CONFIG_SERIAL_SUPPORT_ASYNC 1
#define CONFIG_SERIAL_SUPPORT_INTERRUPT 1
#define CONFIG_UART_USE_RUNTIME_CONFIGURE 1
#define CONFIG_UART_ASYNC_API 1
#define CONFIG_EXTI_STM32 1
#define CONFIG_EXTI_STM32_EXTI0_IRQ_PRI 0
#define CONFIG_EXTI_STM32_EXTI1_IRQ_PRI 0
#define CONFIG_EXTI_STM32_EXTI2_IRQ_PRI 0
#define CONFIG_EXTI_STM32_EXTI3_IRQ_PRI 0
#define CONFIG_EXTI_STM32_EXTI4_IRQ_PRI 0
#define CONFIG_EXTI_STM32_EXTI9_5_IRQ_PRI 0
#define CONFIG_EXTI_STM32_EXTI15_10_IRQ_PRI 0
#define CONFIG_EXTI_STM32_PVD_IRQ_PRI 0
#define CONFIG_EXTI_STM32_OTG_FS_WKUP_IRQ_PRI 0
#define CONFIG_EXTI_STM32_TAMP_STAMP_IRQ_PRI 0
#define CONFIG_EXTI_STM32_RTC_WKUP_IRQ_PRI 0
#define CONFIG_SYSTEM_CLOCK_INIT_PRIORITY 0
#define CONFIG_TICKLESS_CAPABLE 1
#define CONFIG_SYSTEM_TIMER_HAS_DISABLE_SUPPORT 1
#define CONFIG_CORTEX_M_SYSTICK_INSTALL_ISR 1
#define CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER 0
#define CONFIG_ENTROPY_GENERATOR 1
#define CONFIG_ENTROPY_LOG_LEVEL_INF 1
#define CONFIG_ENTROPY_LOG_LEVEL 3
#define CONFIG_ENTROPY_INIT_PRIORITY 50
#define CONFIG_ENTROPY_STM32_THR_POOL_SIZE 8
#define CONFIG_ENTROPY_STM32_THR_THRESHOLD 4
#define CONFIG_ENTROPY_STM32_ISR_POOL_SIZE 16
#define CONFIG_ENTROPY_STM32_ISR_THRESHOLD 12
#define CONFIG_ENTROPY_HAS_DRIVER 1
#define CONFIG_GPIO_LOG_LEVEL_INF 1
#define CONFIG_GPIO_LOG_LEVEL 3
#define CONFIG_FXL6408_LOG_LEVEL_INF 1
#define CONFIG_FXL6408_LOG_LEVEL 3
#define CONFIG_SPI_INIT_PRIORITY 70
#define CONFIG_SPI_COMPLETION_TIMEOUT_TOLERANCE 200
#define CONFIG_SPI_LOG_LEVEL_INF 1
#define CONFIG_SPI_LOG_LEVEL 3
#define CONFIG_SPI_STM32_USE_HW_SS 1
#define CONFIG_PWM 1
#define CONFIG_PWM_LOG_LEVEL_DBG 1
#define CONFIG_PWM_LOG_LEVEL 4
#define CONFIG_ADC 1
#define CONFIG_ADC_LOG_LEVEL_INF 1
#define CONFIG_ADC_LOG_LEVEL 3
#define CONFIG_ADC_STM32_SHARED_IRQS 1
#define CONFIG_CLOCK_CONTROL_LOG_LEVEL_INF 1
#define CONFIG_CLOCK_CONTROL_LOG_LEVEL 3
#define CONFIG_CLOCK_STM32_HSE_CLOCK 8000000
#define CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK 1
#define CONFIG_CLOCK_STM32_MCO2_SRC_NOCLOCK 1
#define CONFIG_FLASH_HAS_DRIVER_ENABLED 1
#define CONFIG_FLASH_HAS_PAGE_LAYOUT 1
#define CONFIG_FLASH 1
#define CONFIG_FLASH_LOG_LEVEL_INF 1
#define CONFIG_FLASH_LOG_LEVEL 3
#define CONFIG_FLASH_PAGE_LAYOUT 1
#define CONFIG_FLASH_INIT_PRIORITY 50
#define CONFIG_SOC_FLASH_STM32 1
#define CONFIG_SENSOR 1
#define CONFIG_SENSOR_LOG_LEVEL_INF 1
#define CONFIG_SENSOR_LOG_LEVEL 3
#define CONFIG_SENSOR_INIT_PRIORITY 90
#define CONFIG_STM32_TEMP 1
#define CONFIG_TMP112_FULL_SCALE_RUNTIME 1
#define CONFIG_TMP112_SAMPLING_FREQUENCY_RUNTIME 1
#define CONFIG_DMA 1
#define CONFIG_DMA_INIT_PRIORITY 40
#define CONFIG_DMA_LOG_LEVEL_DBG 1
#define CONFIG_DMA_LOG_LEVEL 4
#define CONFIG_DMA_STM32_V1 1
#define CONFIG_CAN 1
#define CONFIG_CAN_LOG_LEVEL_INF 1
#define CONFIG_CAN_LOG_LEVEL 3
#define CONFIG_CAN_HAS_RX_TIMESTAMP 1
#define CONFIG_CAN_WORKQ_FRAMES_BUF_CNT 4
#define CONFIG_CAN_AUTO_BUS_OFF_RECOVERY 1
#define CONFIG_CAN_STM32 1
#define CONFIG_CAN_MAX_FILTER 5
#define CONFIG_DISK_DRIVERS 1
#define CONFIG_SDMMC_INIT_PRIORITY 90
#define CONFIG_SDMMC_VOLUME_NAME "SD"
#define CONFIG_SDMMC_OVER_SPI 1
#define CONFIG_SDMMC_LOG_LEVEL_INF 1
#define CONFIG_SDMMC_LOG_LEVEL 3
#define CONFIG_PINCTRL_STM32 1
#define CONFIG_PINCTRL_STM32_REMAP_INIT_PRIORITY 2
#define CONFIG_SUPPORT_MINIMAL_LIBC 1
#define CONFIG_NEWLIB_LIBC 1
#define CONFIG_APP_HAS_NEWLIB_LIBC_NANO 1
#define CONFIG_NEWLIB_LIBC_NANO 1
#define CONFIG_NEWLIB_LIBC_MIN_REQUIRED_HEAP_SIZE 2048
#define CONFIG_NEWLIB_LIBC_FLOAT_PRINTF 1
#define CONFIG_NEWLIB_LIBC_FLOAT_SCANF 1
#define CONFIG_STDOUT_CONSOLE 1
#define CONFIG_JSON_LIBRARY 1
#define CONFIG_MPSC_PBUF 1
#define CONFIG_CBPRINTF_COMPLETE 1
#define CONFIG_CBPRINTF_FULL_INTEGRAL 1
#define CONFIG_CBPRINTF_FP_SUPPORT 1
#define CONFIG_CBPRINTF_N_SPECIFIER 1
#define CONFIG_SYS_HEAP_ALLOC_LOOPS 3
#define CONFIG_SYS_HEAP_SMALL_ONLY 1
#define CONFIG_SYS_MEM_BLOCKS 1
#define CONFIG_POSIX_MAX_FDS 16
#define CONFIG_POSIX_API 1
#define CONFIG_PTHREAD_IPC 1
#define CONFIG_MAX_PTHREAD_COUNT 5
#define CONFIG_SEM_VALUE_MAX 32767
#define CONFIG_POSIX_CLOCK 1
#define CONFIG_MAX_TIMER_COUNT 5
#define CONFIG_POSIX_MQUEUE 1
#define CONFIG_MSG_COUNT_MAX 16
#define CONFIG_MSG_SIZE_MAX 16
#define CONFIG_MQUEUE_NAMELEN_MAX 16
#define CONFIG_POSIX_FS 1
#define CONFIG_POSIX_MAX_OPEN_FILES 16
#define CONFIG_APP_LINK_WITH_POSIX_SUBSYS 1
#define CONFIG_EVENTFD 1
#define CONFIG_EVENTFD_MAX 1
#define CONFIG_PRINTK 1
#define CONFIG_EARLY_CONSOLE 1
#define CONFIG_ASSERT_VERBOSE 1
#define CONFIG_DISK_ACCESS 1
#define CONFIG_DISK_LOG_LEVEL_DBG 1
#define CONFIG_DISK_LOG_LEVEL 4
#define CONFIG_FILE_SYSTEM 1
#define CONFIG_FS_LOG_LEVEL_INF 1
#define CONFIG_FS_LOG_LEVEL 3
#define CONFIG_APP_LINK_WITH_FS 1
#define CONFIG_FILE_SYSTEM_MAX_TYPES 1
#define CONFIG_FILE_SYSTEM_MAX_FILE_NAME -1
#define CONFIG_FAT_FILESYSTEM_ELM 1
#define CONFIG_FS_FATFS_EXFAT 1
#define CONFIG_FS_FATFS_NUM_FILES 4
#define CONFIG_FS_FATFS_NUM_DIRS 4
#define CONFIG_FS_FATFS_LFN 1
#define CONFIG_FS_FATFS_LFN_MODE_BSS 1
#define CONFIG_FS_FATFS_MAX_LFN 255
#define CONFIG_FS_FATFS_CODEPAGE 437
#define CONFIG_FS_FATFS_MAX_SS 512
#define CONFIG_NVS 1
#define CONFIG_NVS_LOG_LEVEL_DBG 1
#define CONFIG_NVS_LOG_LEVEL 4
#define CONFIG_LOG 1
#define CONFIG_LOG_MODE_DEFERRED 1
#define CONFIG_LOG2 1
#define CONFIG_LOG2_DEFERRED 1
#define CONFIG_LOG_DEFAULT_LEVEL 3
#define CONFIG_LOG_OVERRIDE_LEVEL 0
#define CONFIG_LOG_MAX_LEVEL 4
#define CONFIG_LOG_FUNC_NAME_PREFIX_DBG 1
#define CONFIG_LOG_BACKEND_SHOW_COLOR 1
#define CONFIG_LOG_TAG_MAX_LEN 0
#define CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP 1
#define CONFIG_LOG_MODE_OVERFLOW 1
#define CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD 10
#define CONFIG_LOG_PROCESS_THREAD 1
#define CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS 0
#define CONFIG_LOG_PROCESS_THREAD_SLEEP_MS 1000
#define CONFIG_LOG_PROCESS_THREAD_STACK_SIZE 768
#define CONFIG_LOG_BUFFER_SIZE 1024
#define CONFIG_LOG_TRACE_SHORT_TIMESTAMP 1
#define CONFIG_LOG_BACKEND_UART 1
#define CONFIG_LOG_BACKEND_UART_OUTPUT_TEXT 1
#define CONFIG_LOG_DOMAIN_ID 0
#define CONFIG_LOG2_USE_VLA 1
#define CONFIG_NET_BUF 1
#define CONFIG_NET_BUF_USER_DATA_SIZE 0
#define CONFIG_NET_BUF_LOG_LEVEL_INF 1
#define CONFIG_NET_BUF_LOG_LEVEL 3
#define CONFIG_NETWORKING 1
#define CONFIG_NET_L2_ETHERNET_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_L2_ETHERNET_LOG_LEVEL 3
#define CONFIG_NET_ARP 1
#define CONFIG_NET_ARP_TABLE_SIZE 2
#define CONFIG_NET_ARP_GRATUITOUS 1
#define CONFIG_NET_ARP_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_ARP_LOG_LEVEL 3
#define CONFIG_NET_NATIVE 1
#define CONFIG_NET_NATIVE_IPV6 1
#define CONFIG_NET_NATIVE_IPV4 1
#define CONFIG_NET_NATIVE_TCP 1
#define CONFIG_NET_NATIVE_UDP 1
#define CONFIG_NET_INIT_PRIO 90
#define CONFIG_NET_IF_MAX_IPV6_COUNT 1
#define CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT 2
#define CONFIG_NET_IF_MCAST_IPV6_ADDR_COUNT 3
#define CONFIG_NET_IF_IPV6_PREFIX_COUNT 2
#define CONFIG_NET_INITIAL_HOP_LIMIT 64
#define CONFIG_NET_IPV6_MAX_NEIGHBORS 8
#define CONFIG_NET_IPV6_MLD 1
#define CONFIG_NET_IPV6_NBR_CACHE 1
#define CONFIG_NET_IPV6_ND 1
#define CONFIG_NET_IPV6_DAD 1
#define CONFIG_NET_IPV6_RA_RDNSS 1
#define CONFIG_NET_IPV6_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_IPV6_LOG_LEVEL 3
#define CONFIG_NET_ICMPV6_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_ICMPV6_LOG_LEVEL 3
#define CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL 3
#define CONFIG_NET_IPV4 1
#define CONFIG_NET_INITIAL_TTL 64
#define CONFIG_NET_IF_MAX_IPV4_COUNT 1
#define CONFIG_NET_IF_UNICAST_IPV4_ADDR_COUNT 1
#define CONFIG_NET_IF_MCAST_IPV4_ADDR_COUNT 1
#define CONFIG_NET_DHCPV4 1
#define CONFIG_NET_DHCPV4_INITIAL_DELAY_MAX 2
#define CONFIG_NET_IPV4_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_IPV4_LOG_LEVEL 3
#define CONFIG_NET_ICMPV4_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_ICMPV4_LOG_LEVEL 3
#define CONFIG_NET_DHCPV4_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_DHCPV4_LOG_LEVEL 3
#define CONFIG_NET_TC_TX_COUNT 0
#define CONFIG_NET_TC_RX_COUNT 1
#define CONFIG_NET_TC_THREAD_COOPERATIVE 1
#define CONFIG_NET_TC_NUM_PRIORITIES 16
#define CONFIG_NET_TC_MAPPING_STRICT 1
#define CONFIG_NET_TX_DEFAULT_PRIORITY 1
#define CONFIG_NET_RX_DEFAULT_PRIORITY 0
#define CONFIG_NET_IP_ADDR_CHECK 1
#define CONFIG_NET_MAX_ROUTERS 2
#define CONFIG_NET_ROUTE 1
#define CONFIG_NET_MAX_ROUTES 8
#define CONFIG_NET_MAX_NEXTHOPS 8
#define CONFIG_NET_TCP 1
#define CONFIG_NET_TCP_CHECKSUM 1
#define CONFIG_NET_TCP_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_TCP_LOG_LEVEL 3
#define CONFIG_NET_TCP_BACKLOG_SIZE 1
#define CONFIG_NET_TCP_TIME_WAIT_DELAY 0
#define CONFIG_NET_TCP_ACK_TIMEOUT 1000
#define CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT 200
#define CONFIG_NET_TCP_RETRY_COUNT 9
#define CONFIG_NET_TCP_MAX_SEND_WINDOW_SIZE 0
#define CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT 100
#define CONFIG_NET_TCP_WORKQ_STACK_SIZE 1024
#define CONFIG_NET_TCP_ISN_RFC6528 1
#define CONFIG_NET_UDP 1
#define CONFIG_NET_UDP_CHECKSUM 1
#define CONFIG_NET_UDP_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_UDP_LOG_LEVEL 3
#define CONFIG_NET_MAX_CONN 12
#define CONFIG_NET_MAX_CONTEXTS 12
#define CONFIG_NET_CONTEXT_SYNC_RECV 1
#define CONFIG_NET_CONTEXT_CHECK 1
#define CONFIG_NET_PKT_RX_COUNT 64
#define CONFIG_NET_PKT_TX_COUNT 64
#define CONFIG_NET_BUF_RX_COUNT 64
#define CONFIG_NET_BUF_TX_COUNT 64
#define CONFIG_NET_BUF_FIXED_DATA_SIZE 1
#define CONFIG_NET_BUF_DATA_SIZE 128
#define CONFIG_NET_DEFAULT_IF_FIRST 1
#define CONFIG_NET_TX_STACK_SIZE 1200
#define CONFIG_NET_RX_STACK_SIZE 1500
#define CONFIG_NET_MGMT 1
#define CONFIG_NET_MGMT_EVENT 1
#define CONFIG_NET_MGMT_EVENT_STACK_SIZE 768
#define CONFIG_NET_MGMT_EVENT_QUEUE_SIZE 2
#define CONFIG_NET_MGMT_EVENT_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_MGMT_EVENT_LOG_LEVEL 3
#define CONFIG_NET_STATISTICS 1
#define CONFIG_NET_STATISTICS_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_STATISTICS_LOG_LEVEL 3
#define CONFIG_NET_STATISTICS_PER_INTERFACE 1
#define CONFIG_NET_STATISTICS_USER_API 1
#define CONFIG_NET_STATISTICS_IPV4 1
#define CONFIG_NET_STATISTICS_IPV6 1
#define CONFIG_NET_STATISTICS_IPV6_ND 1
#define CONFIG_NET_STATISTICS_ICMP 1
#define CONFIG_NET_STATISTICS_UDP 1
#define CONFIG_NET_STATISTICS_TCP 1
#define CONFIG_NET_STATISTICS_MLD 1
#define CONFIG_NET_STATISTICS_ETHERNET 1
#define CONFIG_NET_LOG 1
#define CONFIG_NET_PKT_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_PKT_LOG_LEVEL 3
#define CONFIG_NET_DEBUG_NET_PKT_EXTERNALS 0
#define CONFIG_NET_CORE_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_CORE_LOG_LEVEL 3
#define CONFIG_NET_IF_LOG_LEVEL_INF 1
#define CONFIG_NET_IF_LOG_LEVEL 3
#define CONFIG_NET_TC_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_TC_LOG_LEVEL 3
#define CONFIG_NET_UTILS_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_UTILS_LOG_LEVEL 3
#define CONFIG_NET_CONTEXT_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_CONTEXT_LOG_LEVEL 3
#define CONFIG_NET_CONN_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_CONN_LOG_LEVEL 3
#define CONFIG_NET_ROUTE_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_ROUTE_LOG_LEVEL 3
#define CONFIG_DNS_RESOLVER 1
#define CONFIG_DNS_RESOLVER_ADDITIONAL_BUF_CTR 1
#define CONFIG_DNS_RESOLVER_ADDITIONAL_QUERIES 1
#define CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES 1
#define CONFIG_DNS_RESOLVER_MAX_SERVERS 1
#define CONFIG_DNS_NUM_CONCUR_QUERIES 1
#define CONFIG_DNS_RESOLVER_LOG_LEVEL_DBG 1
#define CONFIG_DNS_RESOLVER_LOG_LEVEL 4
#define CONFIG_HTTP_PARSER 1
#define CONFIG_HTTP_PARSER_URL 1
#define CONFIG_HTTP_CLIENT 1
#define CONFIG_NET_HTTP_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_HTTP_LOG_LEVEL 3
#define CONFIG_SNTP 1
#define CONFIG_SNTP_LOG_LEVEL_DEFAULT 1
#define CONFIG_SNTP_LOG_LEVEL 3
#define CONFIG_NET_CONFIG_INIT_TIMEOUT 30
#define CONFIG_NET_CONFIG_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_CONFIG_LOG_LEVEL 3
#define CONFIG_NET_SOCKETS 1
#define CONFIG_NET_SOCKETS_PRIORITY_DEFAULT 50
#define CONFIG_NET_SOCKETS_POLL_MAX 7
#define CONFIG_NET_SOCKETS_CONNECT_TIMEOUT 3000
#define CONFIG_NET_SOCKETS_DNS_TIMEOUT 2000
#define CONFIG_NET_SOCKETS_SOCKOPT_TLS 1
#define CONFIG_NET_SOCKETS_TLS_PRIORITY 45
#define CONFIG_NET_SOCKETS_TLS_SET_MAX_FRAGMENT_LENGTH 1
#define CONFIG_NET_SOCKETS_TLS_MAX_CONTEXTS 4
#define CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS 4
#define CONFIG_NET_SOCKETS_TLS_MAX_CIPHERSUITES 4
#define CONFIG_NET_SOCKETS_LOG_LEVEL_DEFAULT 1
#define CONFIG_NET_SOCKETS_LOG_LEVEL 3
#define CONFIG_TLS_CREDENTIALS 1
#define CONFIG_TLS_MAX_CREDENTIALS_NUMBER 4
#define CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR 1
#define CONFIG_CSPRING_ENABLED 1
#define CONFIG_APP_HARDWARE_DEVICE_CS_GENERATOR 1
#define CONFIG_FLASH_MAP 1
#define CONFIG_TEST_LOGGING_FLUSH_AFTER_TEST 1
#define CONFIG_TOOLCHAIN_ZEPHYR_0_14 1
#define CONFIG_LINKER_ORPHAN_SECTION_WARN 1
#define CONFIG_APP_HAS_FLASH_LOAD_OFFSET 1
#define CONFIG_USE_DT_CODE_PARTITION 1
#define CONFIG_FLASH_LOAD_OFFSET 0x0
#define CONFIG_FLASH_LOAD_SIZE 0x100000
#define CONFIG_LD_LINKER_SCRIPT_SUPPORTED 1
#define CONFIG_LD_LINKER_TEMPLATE 1
#define CONFIG_KERNEL_ENTRY "__start"
#define CONFIG_LINKER_SORT_BY_ALIGNMENT 1
#define CONFIG_SRAM_OFFSET 0x0
#define CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT 1
#define CONFIG_SIZE_OPTIMIZATIONS 1
#define CONFIG_COMPILER_COLOR_DIAGNOSTICS 1
#define CONFIG_COMPILER_OPT ""
#define CONFIG_RUNTIME_ERROR_CHECKS 1
#define CONFIG_KERNEL_BIN_NAME "zephyr"
#define CONFIG_OUTPUT_STAT 1
#define CONFIG_OUTPUT_DISASSEMBLY 1
#define CONFIG_OUTPUT_PRINT_MEMORY_USAGE 1
#define CONFIG_BUILD_OUTPUT_BIN 1
#define CONFIG_EXPERIMENTAL 1
#define CONFIG_COMPAT_INCLUDES 1
