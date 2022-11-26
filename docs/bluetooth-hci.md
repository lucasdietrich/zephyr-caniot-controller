# Bluetooth HCI

Controller have a nrf52840 chip dedicated to bluetooth. Formerly, the bluetooth
application was hosted on the BLE SoC. A custom UART protocol was used between
the two chips to communicate.

With the new architecture, the bluetooth application runs on the main MCU, the
BLE chip only run a HCI controller, and the two chips communicate over SPI.
With this design, the bluetooth application is embedded with the main application.

This architecture is well documented in the description of the 
**Dual-chip configuration** in the
[Zephyr Bluetooth Stack Architecture documentation](https://docs.zephyrproject.org/latest/connectivity/bluetooth/bluetooth-arch.html)

## PoC

In order to validate this architecture, following PoC was carried out:
- Sample `bluetooth/observer` as HCI host with `nucleo_f429zi` board
- Sample `bluetooth/hci_spi` as HCI controller with `nrf52840dk_nrf52840` board

### Wiring

| Pin  | `nrf52840dk_nrf52840` | `nucleo_f429zi` | Logic analyzer |
| ---- | --------------------- | --------------- | -------------- |
| CS   | P0.03                 | PA_4            | 1              |
| MISO | P0.04                 | PA_6            | 2              |
| MOSI | P0.05                 | PA_7            | 4              |
| SCK  | P0.06                 | PA_5            | 3              |
| IRQ  | P1.02                 | PF_12           |                |
| RST  | P0.18 (RESET)         | PD_15           |                |

### HCI Host - `nucleo_f429zi`

Following configuration options were added to `prj.conf`:
```
CONFIG_SPI=y

CONFIG_BT_HCI=y
CONFIG_BT_SPI=y
CONFIG_BT_CTLR=n

CONFIG_BT_DEBUG_LOG=n
CONFIG_BT_DEBUG_HCI_DRIVER=n

CONFIG_LOG=y
CONFIG_PRINTK=y
```

Devicetree overlay `boards/nucleo_f429zi.overlay`:
```devicetree
&spi1 {
	cs-gpios = <&gpioa 4 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
	status = "okay";

	hci-nrf52@0 {
		compatible = "zephyr,bt-hci-spi";
		reg = <0>;
		irq-gpios = <&gpiof 12 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
		reset-gpios = <&gpiod 15 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
		spi-max-frequency = <2000000>; /* 2000000 */
	};
};

&gpiof {
	status = "okay";
};
```

So that the host receive BLE advertising frames continuously, scan parameters were changed to :
```c
struct bt_le_scan_param scan_param = {
	.type       = BT_LE_SCAN_TYPE_PASSIVE,
	.options    = BT_LE_SCAN_OPT_NONE, /* No filtering on duplicates */
	.interval   = BT_GAP_SCAN_FAST_INTERVAL,
	.window     = BT_GAP_SCAN_FAST_WINDOW,
};
```


### HCI Controller - `nrf52840dk_nrf52840`

Configuration file was kept as default.

Devicetree overlay `boards/nrf52840dk_nrf52840.overlay`: 
```
&pinctrl {
	spi1_default_alt: spi1_default_alt {
		group1 {
			psels = <NRF_PSEL(SPIS_SCK, 0, 6)>,
				<NRF_PSEL(SPIS_MOSI, 0, 5)>,
				<NRF_PSEL(SPIS_MISO, 0, 4)>,
				<NRF_PSEL(SPIS_CSN, 0, 3)>;
		};
	};
};

&spi1 {
	compatible = "nordic,nrf-spis";
	status = "okay";
	def-char = <0x00>;
	pinctrl-0 = <&spi1_default_alt>;
	/delete-property/ pinctrl-1;
	pinctrl-names = "default";

	bt-hci@0 {
		compatible = "zephyr,bt-hci-spi-slave";
		reg = <0>;
		irq-gpios = <&gpio1 2 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
	};
};

&uart1 {
	status = "disabled";
};
```

Following instruction was add at the very beginning of `main` function:
```c
    /* Set GPIO voltage level to 3.3v, instead of default 1.8v*/
	NRF_UICR->REGOUT0 = UICR_REGOUT0_VOUT_3V3;
```

### Expected result

Nucleo console expected output is:
```
*** Booting Zephyr OS build zephyr-v3.2.0-1-g817a0726429f  ***
Starting Observer Demo
Started scanning...
Exiting main thread.
Device found: A4:C1:38:8D:BA:B4 (public) (RSSI -50), type 0, AD data len 19
Device found: A4:C1:38:0A:1E:38 (public) (RSSI -59), type 0, AD data len 19
Device found: A4:C1:38:28:17:E3 (public) (RSSI -78), type 0, AD data len 19
Device found: A4:C1:38:EC:1C:6D (public) (RSSI -56), type 0, AD data len 19
Device found: A4:C1:38:68:05:63 (public) (RSSI -55), type 0, AD data len 19
Device found: A4:C1:38:A7:30:C4 (public) (RSSI -84), type 0, AD data len 19
Device found: A4:C1:38:68:05:63 (public) (RSSI -56), type 0, AD data len 19
Device found: A4:C1:38:E0:18:ED (public) (RSSI -72), type 0, AD data len 19
Device found: A4:C1:38:8D:BA:B4 (public) (RSSI -52), type 0, AD data len 19
Device found: A4:C1:38:0A:1E:38 (public) (RSSI -57), type 0, AD data len 19
Device found: A4:C1:38:28:17:E3 (public) (RSSI -78), type 0, AD data len 19
```

By monitoring the SPI bus with a logic analyzer, we can see the HCI frames exchanged between the two boards.