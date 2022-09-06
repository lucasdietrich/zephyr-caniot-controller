# Flash

## STM32f429zi

- Sector, Bank or Mass **erase**
- Byte/half-word/word/double word **write**

- 2 bancks (1MB each):
  - each bank has 12 sectors :
     - sector 0 - 3 : 16KB
     - sector 4 : 64KB
     - sector 5-11 : 128KB
- 512B OTP

- [Manual page 76 - Embedded Flash memory in STM32F42xxx and STM32F43xxx](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf#page=76)

- [Manual page 77 - Table 6. Flash module - 2 Mbyte dual bank organization (STM32F42xxx and STM32F43xxx)](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf#page=77)

## Partitions

[CONFIG_USE_DT_CODE_PARTITION](https://docs.zephyrproject.org/latest/reference/kconfig/CONFIG_USE_DT_CODE_PARTITION.html)

Idea : 
- Bootloader : 64KB (4*16KB)
- Application : 1MB - 64 KB
- Storage (config) : 4*16KB + 64KB
- Live-values : 128KB*... 
- Certificates : 128KB

or:

- Bootloader : 64KB
- Storage : 64KB
- Application : 2MB - 64 - 64 - 128
- Certificates : 128KB

## Expected output

```
*** Booting Zephyr OS build zephyr-v20600  ***
[00:00:10.004,000] <inf> main: flash_get_page_count(0x200001e8) = 24 sectors (pages)
[00:00:10.012,000] <inf> main: index=0 size=0x4000 start_offset=0x0
[00:00:10.018,000] <inf> main: index=1 size=0x4000 start_offset=0x4000
[00:00:10.025,000] <inf> main: index=2 size=0x4000 start_offset=0x8000
[00:00:10.032,000] <inf> main: index=3 size=0x4000 start_offset=0xc000
[00:00:10.039,000] <inf> main: index=4 size=0x10000 start_offset=0x10000
[00:00:10.046,000] <inf> main: index=5 size=0x20000 start_offset=0x20000
[00:00:10.053,000] <inf> main: index=6 size=0x20000 start_offset=0x40000
[00:00:10.060,000] <inf> main: index=7 size=0x20000 start_offset=0x60000
[00:00:10.068,000] <inf> main: index=8 size=0x20000 start_offset=0x80000
[00:00:10.075,000] <inf> main: index=9 size=0x20000 start_offset=0xa0000
[00:00:10.082,000] <inf> main: index=10 size=0x20000 start_offset=0xc0000
[00:00:10.089,000] <inf> main: index=11 size=0x20000 start_offset=0xe0000
[00:00:10.096,000] <inf> main: index=12 size=0x4000 start_offset=0x100000
[00:00:10.103,000] <inf> main: index=13 size=0x4000 start_offset=0x104000
[00:00:10.110,000] <inf> main: index=14 size=0x4000 start_offset=0x108000
[00:00:10.118,000] <inf> main: index=15 size=0x4000 start_offset=0x10c000
[00:00:10.125,000] <inf> main: index=16 size=0x10000 start_offset=0x110000
[00:00:10.132,000] <inf> main: index=17 size=0x20000 start_offset=0x120000
[00:00:10.139,000] <inf> main: index=18 size=0x20000 start_offset=0x140000
[00:00:10.147,000] <inf> main: index=19 size=0x20000 start_offset=0x160000
[00:00:10.154,000] <inf> main: index=20 size=0x20000 start_offset=0x180000
[00:00:10.161,000] <inf> main: index=21 size=0x20000 start_offset=0x1a0000
[00:00:10.169,000] <inf> main: index=22 size=0x20000 start_offset=0x1c0000
[00:00:10.176,000] <inf> main: index=23 size=0x20000 start_offset=0x1e0000
[00:00:10.183,000] <inf> main: flash_area_open(0x1, *) = 0
[00:00:10.189,000] <inf> main: content
                               49 74 27 73 20 6d 65 2c  20 4c 75 63 61 73 20 21 |It's me,  Lucas !
                               21 21 00 ff ff ff ff ff  ff ff ff ff ff ff ff ff |!!...... ........
                               ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........
...

```

## Configuration options

```
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y

CONFIG_FLASH_PAGE_LAYOUT=y
```

## Sources

- [Flash Map](https://docs.zephyrproject.org/latest/reference/storage/flash_map/flash_map.html)
- [Flash Pages](https://docs.zephyrproject.org/latest/reference/peripherals/flash.html)

## Known Issues

- PlatformIO doesn't show the correct memory usage when use a custom partition for the Zephyr application :

```
RAM:   [          ]   3.4% (used 6644 bytes from 196608 bytes)
Flash: [          ]   1.0% (used 21088 bytes from 2097152 bytes)
```

Which is wrong, real flash % use is 21088 / 1792KB = 1.1%, see [zephyr/nucleo_f429zi.overlay](../zephyr/nucleo_f429zi.overlay)

---

## OBSOLETE

Issues :
- **Interesting** :
  - https://github.com/zephyrproject-rtos/zephyr/issues/13151


- https://github.com/zephyrproject-rtos/zephyr/issues/31073
- https://github.com/zephyrproject-rtos/zephyr/issues/36541
- https://github.com/zephyrproject-rtos/zephyr/issues/34167
- https://github.com/zephyrproject-rtos/zephyr/issues/27471
- https://github.com/zephyrproject-rtos/zephyr/issues/31487
- https://github.com/zephyrproject-rtos/zephyr/pull/27974

- Theorically solved : https://github.com/zephyrproject-rtos/zephyr/issues/24237
