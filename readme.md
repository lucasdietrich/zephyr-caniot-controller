# caniot-controller (stm32F429zi w/ Zephyr RTOS)

## Links

**Zephyr RTOS 2.6**
- [GitHub](https://github.com/zephyrproject-rtos/zephyr/tree/v2.6-branch)
- [All Configuration Options](https://docs.zephyrproject.org/2.6.0/reference/kconfig/index-all.html)
- [Guides](https://docs.zephyrproject.org/latest/guides/index.html)

**Board**
- [hal_stm32/dts/st/f4/stm32f429zitx-pinctrl.dtsi](https://github.com/zephyrproject-rtos/hal_stm32/blob/main/dts/st/f4/stm32f429zitx-pinctrl.dtsi)

- [ST Nucleo F429ZI](https://github.com/zephyrproject-rtos/zephyr/blob/main/boards/arm/nucleo_f429zi/doc/index.rst#st-nucleo-f429zi)


**Project**
- Configuration
  - [prj.conf](./zephyr/prj.conf)
  - [nucleo_f429zi.overlay](./zephyr/nucleo_f429zi.overlay)
  - [CMakeLists.txt](./zephyr/CMakeLists.txt)
- Build:
  - [zephyr.dts](.pio/build/nucleo_f429zi/zephyr/zephyr.dts)
  - [autoconf.h](.pio/build/nucleo_f429zi/zephyr/include/generated/autoconf.h)

**Zephyr Samples**
- [code_relocation](https://github.com/zephyrproject-rtos/zephyr/tree/v2.6-branch/samples/application_development/code_relocation)
- [application_development/external_lib](https://github.com/zephyrproject-rtos/zephyr/tree/v2.6-branch/samples/application_development/external_lib)
- [out_of_tree_driver](https://github.com/zephyrproject-rtos/zephyr/tree/v2.6-branch/samples/application_development/out_of_tree_driver)


---

## Next steps

- MBEDTLS : 
  - setup simple http server and tcp client
  - MBEDTLS
  - elliptic curves ECDSA better than RSA ?

- HTTP server :
  - Timeout on keepalive connections

- CAN-TCP server/client

- Starting to specify the application

- SD card :
  - Drivers
  - Interrupt
  - Connect/Read/Write/Disconnect model

## Issues
- loopback tcp doesn't work : connecting to localhost doesn't work (client connecting to server, both on the same f429zi)
    - Infinite recursion in tcp_in(), stack overflow
    - Enable `CONFIG_NET_TCP_LOG_LEVEL_DBG` to debug

## Questions/Open Points

- How to show correct Memory usage after linker in PlatformIO with custom partitions ?
- Find a way to link certificates in the dedicated `certificates` partition (from a text file ?)
## Objectives
- Be able to build/debug the project at the same time using PlatformIO and CMake/Zephyr env

---

## Zephyr RTOS 2.6 (current)

- [All Kconfig configuration options](https://docs.zephyrproject.org/2.6.0/reference/kconfig/index-all.html)


## Zephyr RTOS 2.7

[Zephyr RTOS release 2.7](https://github.com/zephyrproject-rtos/zephyr/releases/tag/zephyr-v2.7.0)

- [All Kconfig configuration options](https://docs.zephyrproject.org/2.7.0/reference/kconfig/index-all.html)

## PlatformIO and Zephyr RTOS

- https://docs.platformio.org/en/latest/frameworks/zephyr.html
- Tutoriel [Enabling PlatformIO and Zephyr on custom hardware](https://piolabs.com/blog/engineering/platformio-zephyr-custom-hardware.html)
- https://docs.platformio.org/en/latest/boards/ststm32/nucleo_f429zi.html

## Modules

Will be required when using zephyr 2.7 without PlatformIO

- zephyrproject-rtos / [hal_stm32](https://github.com/zephyrproject-rtos/hal_stm32)

---

## Build

*PlatformIO*

## Flash

- Install [STM32 ST-LINK utility](https://www.st.com/en/development-tools/stsw-link004.html)

---

## Misc

- *st* :
  - [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
  - [x-cube-azrtos-f4](https://github.com/STMicroelectronics/x-cube-azrtos-f4)