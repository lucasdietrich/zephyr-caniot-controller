# CAN 

The solution was to properly define following devicetree properties in the `.overlay file` by selecting the desired [stm32f429zi pins](https://github.com/zephyrproject-rtos/hal_stm32/blob/5c8275071ec1cf160bfe8c18bbd9330a7d714dc8/dts/st/f4/stm32f429zitx-pinctrl.dtsi#L207-L254) see [ST Nucleo F429ZI - Connections and IOs](https://github.com/zephyrproject-rtos/zephyr/blob/main/boards/arm/nucleo_f429zi/doc/index.rst#available-pins).

```
&can1 {
        status = "okay";
        pinctrl-0 = <&can1_rx_pd0 &can1_tx_pd1>;
        bus-speed = < 500000 >;
        ...
```

## Output console

Expected output : 

```
*** Booting Zephyr OS build zephyr-v20600  ***
[00:00:00.000,000] <dbg> can_driver.can_calc_timing_int: SP error: 18 1/1000 
[00:00:00.000,000] <dbg> can_driver.can_stm32_init: Presc: 6, TS1: 11, TS2: 2
[00:00:00.000,000] <dbg> can_driver.can_stm32_init: Sample-point err : 18
[00:00:00.000,000] <dbg> can_driver.can_stm32_set_mode: Set mode 0       
[00:00:00.000,000] <dbg> can_driver.config_can_1_irq: Enable CAN1 IRQ    
[00:00:00.000,000] <inf> can_driver: Init of CAN_1 done
[00:00:00.004,000] <dbg> can_driver.can_stm32_send: Sending 8 bytes on CAN_1. Id: 0x123, ID type: standard, Remote Frame: no
[00:00:00.004,000] <dbg> can_driver.can_stm32_send: Using mailbox 0
[00:00:05.004,000] <dbg> can_driver.can_stm32_send: Sending 8 bytes on CAN_1. Id: 0x123, ID type: standard, Remote Frame: no
[00:00:05.004,000] <dbg> can_driver.can_stm32_send: Using mailbox 0
```

## Note :

Current stm32 can driver only support a single CAN interface for now (2 interfaces are available in the stm32f429zi).

A solution would be to use another mcp2515 via SPI

## Branches

- `can/poc` : PoC
- `can/socket` : socket can API 
- `can/drivers_level` : drivers API 

## Sources :

- [devicetree - st,stm32-can](https://docs.zephyrproject.org/latest/reference/devicetree/bindings/can/st%2Cstm32-can.html)
- GitHub Issue : *zephyrproject-rtos / zephyrdrivers*: [can: stm32fd: can2 does not work](https://github.com/zephyrproject-rtos/zephyr/issues/36075)