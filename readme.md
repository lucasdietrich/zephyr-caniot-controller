# caniot-controller (stm32F429zi w/ Zephyr RTOS)

## MCU

[STM32F429ZI](https://www.st.com/en/microcontrollers-microprocessors/stm32f429zi.html)
- 2MB Flash
- 180MHz
- 192kB SRAM
- 64kB CCM

![](https://www.st.com/content/ccc/fragment/product_related/rpn_information/product_circuit_diagram/group0/8d/9e/28/ab/4a/5b/45/ca/bd_stm32f429xi_2m/files/bd_stm32f429xi_2m.jpg/jcr:content/translations/en.bd_stm32f429xi_2m.jpg)

## Zephyr RTOS 2.6 (current)

- [All Kconfig configuration options](https://docs.zephyrproject.org/2.6.0/reference/kconfig/index-all.html)


## Zephyr RTOS 2.7

[Zephyr RTOS release 2.7](https://github.com/zephyrproject-rtos/zephyr/releases/tag/zephyr-v2.7.0)

- [All Kconfig configuration options](https://docs.zephyrproject.org/2.7.0/reference/kconfig/index-all.html)

## PlatformIO and Zephyr RTOS

- https://docs.platformio.org/en/latest/frameworks/zephyr.html
- Tutoriel [Enabling PlatformIO and Zephyr on custom hardware](https://piolabs.com/blog/engineering/platformio-zephyr-custom-hardware.html)

## `stm32f429zi` (`nucleo_f429zi`) :

- [hal_stm32/dts/st/f4/stm32f429zitx-pinctrl.dtsi](https://github.com/zephyrproject-rtos/hal_stm32/blob/main/dts/st/f4/stm32f429zitx-pinctrl.dtsi)
- [ST Nucleo F429ZI](https://github.com/zephyrproject-rtos/zephyr/blob/main/boards/arm/nucleo_f429zi/doc/index.rst#st-nucleo-f429zi)

## Modules

Will be required when using zephyr 2.7 without PlatformIO

- zephyrproject-rtos / [hal_stm32](https://github.com/zephyrproject-rtos/hal_stm32)

---

## Build



## Flash

- Install [STM32 ST-LINK utility](https://www.st.com/en/development-tools/stsw-link004.html)

---

## Misc

- *st* :
  - [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
  - [x-cube-azrtos-f4](https://github.com/STMicroelectronics/x-cube-azrtos-f4)