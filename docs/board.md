# CANIoT Board

TODO:
- diode USB > LD3985..
- MCP9804-E/MS x 4 / MCP9803 x 10 (footprint ??)
- LD1117S50TR alim externe (>5V) + diode 
- capa eth TC1206KKX7RDBB102 (1nF, 2000V)
- diff pair resistance
- res + diode buttons
- unecessary 4.7uF capa with chips (other than I2C line, etc...)


Choosen:
- Main microcontroller: STM32F429BIT6
- BLE module: ACN52840

Architecture, interfaces, chips, wiring, etc.

## Clocks

- HSE: Enabled
  - Frency (can be 4 to 24Mhz)
- ~~LSE~~: Disable because no need of RTC

### Pin configuration

| Label             | stm32f429bit6           | Note                  |
| ----------------- | ----------------------- | --------------------- |
| STM_RCC_OSC_IN    | PH0-OSC_IN (32)         | HSE Clock             |
| STM_RCC_OSC_OUT   | PH1-OSC_OUT (33)        | HSE Clock             |
| STM_RCC_OSC32_IN  | PC14-RCC_OSC32_IN (9)   | RCC Clock (optionnal) |
| STM_RCC_OSC32_OUT | PC15-RCC_OSC32_OUT (10) | RCC Clock (optionnal) |
| MCO1              | PA8 (142)               |                       |
| MCO2              | PC9 (141)               | Shared with SDIO_D1   |

## Power supply

- MP2359

## SWD

| Label     | stm32f429bit6           | Note |
| --------- | ----------------------- | ---- |
| STM_SWDIO | PA13(JTMS-SWDIO) (147)  |      |
| STM_SWCLK | PA14(JTCK-SWCLK) (159)  |      |
| STM_SWO   | PB3(JTDOTRACESWO) (192) |      |


## Boot0/1

| Label | stm32f429bit6  | Note          |
| ----- | -------------- | ------------- |
| BOOT0 | BOOT0 (197)    | Set to VCC    |
| -     | PB2-BOOT1 (63) | Left floating |


## Misc

### VCAPs

- VCAP1
- VCAP2

Note `low esr`:

Note DocID024030 Rev 10 page 97/239 :
> When bypassing the voltage regulator, the two 2.2 μF VCAP capacitors are not required and should be replaced by two 100 nF decoupling capacitors.

### PDR_ON

- Set to VDD

## LEDs

| Label      | stm32f429bit6      | Note         |
| ---------- | ------------------ | ------------ |
| STM32_LED1 | PI5 TIM8_CH1 (156) | PWM required |
| STM32_LED2 | PI6 TIM8_CH2 (206) | PWM required |
| STM32_LED3 | PI7 TIM8_CH3 (207) | PWM required |
| STM32_LED4 | PI2 TIM8_CH4 (208) | PWM required |

## Buttons

| Label      | stm32f429bit6 | Note |
| ---------- | ------------- | ---- |
| STM32_BTN1 | PI8 (7)       |      |
| STM32_BTN2 | PI10 (12)     |      |

## I2C Bus

| Label    | stm32f429bit6 | Note |
| -------- | ------------- | ---- |
| I2C3_SCL | PH7 (97)      |      |
| I2C3_SDA | PH8 (98)      |      |


### I2C Temperature sensor

- Chip: MCP9804-E/MS

| Label     | I2C Bus | stm32f429bit6 | MCP9804-E/MS | Note |
| --------- | ------- | ------------- | ------------ | ---- |
| MCP_SDA   | SDA     | -             | SDA          |      |
| MCP_SCL   | SCL     | -             | SCL          |      |
| MCP_ALERT | -       | PI12 (19)     | ALERT        | irq  |

## USB

| Label                        | stm32f429bit6  | Note                                                     |
| ---------------------------- | -------------- | -------------------------------------------------------- |
| ~~USB_SOF~~                  | ~~PA8 (142)~~  | not used                                                 |
| USB_VBUS (~~USB_OTG_HS_ID~~) | PA9 (143)      | If USB device is self-powered, VBUS sensing is mandatory |
| ~~USB_OTG_FS_ID~~            | ~~PA10 (144)~~ | no OTG                                                   |
| USB_D-                       | PA11 (145)     | -                                                        |
| USB_D+                       | PA12 (146)     | -                                                        |

## SD Card

![pics/sd-card-pinout.png](./pics/sd-card-pinout.png)

### SD Card (SPI)

| Label     | stm32f429bit6 | sdcard SPI | Note |
| --------- | ------------- | ---------- | ---- |
| SPI3_CS   | PC13 (8)      | 1 CS       |      |
| SPI3_MOSI | PC12 (163)    | 2 DI       |      |
| -         | -             | 3 VSS1     |      |
| -         | -             | 4 VDD      |      |
| SPI3_SCK  | PC10 (161)    | 5 SCLK     |      |
| -         | -             | 6 VSS2     |      |
| SPI3_MISO | PC11 (162)    | 7 D0       |      |
| -         | -             | 8 -        |      |
| -         | -             | 9 -        |      |

### SD Card (MMC)

| Label MMC | stm32f429bit6 | sdcard MMC | Note             |
| --------- | ------------- | ---------- | ---------------- |
| SDIO_D3   | PC11 (162)    | 1 CD/DAT3  |                  |
| SDIO_CMD  | PD2 (166)     | 2 CMD      |                  |
| -         | -             | 3 VSS1     |                  |
| -         | -             | 4 VDD      |                  |
| SDIO_CK   | PC12 (163)    | 5 SCLK     |                  |
| -         | -             | 6 VSS2     |                  |
| SDIO_D0   | PC8 (140)     | 7 DAT1     |                  |
| SDIO_D1   | PC9 (141)     | 8 DAT2     | Shared with MC02 |
| SDIO_D2   | PC10 (161)    | 9 DAT3     |                  |

### SD Detect

| Label SD_DETECT | stm32f429bit6 | Note |
| --------------- | ------------- | ---- |
| SDIO_D3         | PC11 (162)    |      |

## Ethernet

| ~~Label MII~~ | Label RMII   | stm32f429bit6 | ~~MII DP83848C~~ | **RMII DP83848C** | Note |
| ------------- | ------------ | ------------- | ---------------- | ----------------- | ---- |
| MII_CRS       |              | PA0 /WKUP     | ?                | -                 |      |
| MII_RX_CLK    | RMII_REF_CLK | PA1 (44)      | ?                | X1 (34)           |      |
| MII_MDIO      | RMII_MDIO    | PA2 (45)      | ?                | MDIO (30)         |      |
| MII_COL       |              | PA3           | ?                | -                 |      |
| MII_RX_DV     | RMII_CRS_DV  | PA7 (56)      | ?                | CRS_DV (40)       |      |
| MII_RXD2      |              | PB0           | ?                | -                 |      |
| MII_RXD3      |              | PB1           | ?                | -                 |      |
| MII_TX_EN     | RMII_TX_EN   | PB11 (91)     | ?                | TX_EN (2)         |      |
| MII_MDC       | RMII_MDC     | PC1 (36)      | ?                | MDC (31)          |      |
| MII_TXD2      |              | PC2           | ?                | -                 |      |
| MII_TX_CLK    |              | PC3           | ?                | -                 |      |
| MII_RXD0      | RMII_RXD0    | PC4 (57)      | ?                | RXD_0 (43)        |      |
| MII_RXD1      | RMII_RXD1    | PC5 (58)      | ?                | RXD_1 (44)        |      |
| MII_TXD3      |              | PE2           | ?                | -                 |      |
| MII_TXD0      | RMII_TXD0    | PG13 (182)    | ?                | TXD_0 (3)         |      |
| MII_TXD1      | RMII_TXD1    | PG14 (183)    | ?                | TXD_1 (4)         |      |
|               |              |               |                  | **RX_ERR**        |      |


## SDRAM

| Label      | stm32f429bit6 | AS4C16M16SA-6TIN | Note |
| ---------- | ------------- | ---------------- | ---- |
| FMC_SDNWE  | PC0 (35)      | WE# (16)         |      |
| FMC_SDNE0  | PC2 (37)      | CS#              |      |
| FMC_SDCKE0 | PC3 (38)      | CKE              |      |
| FMC_D2     | PD0 (164)     | DQ2              |      |
| FMC_D3     | PD1 (165)     | DQ3              |      |
| FMC_D13    | PD8 (108)     | DQ13             |      |
| FMC_D14    | PD9 (109)     | DQ14             |      |
| FMC_D15    | PD10 (110)    | DQ15             |      |
| FMC_D0     | PD14 (116)    | DQ0              |      |
| FMC_D1     | PD15 (117)    | DQ1              |      |
| FMC_D4     | PE7 (79)      | DQ4              |      |
| FMC_D5     | PE8 (80)      | DQ5              |      |
| FMC_D6     | PE9  (81)     | DQ6              |      |
| FMC_D7     | PE10 (84)     | DQ7              |      |
| FMC_D8     | PE11 (85)     | DQ8              |      |
| FMC_D9     | PE12 (86)     | DQ9              |      |
| FMC_D10    | PE13 (87)     | DQ10             |      |
| FMC_D11    | PE14 (88)     | DQ11             |      |
| FMC_D12    | PE15 (89)     | DQ12             |      |
| FMC_A0     | PF0 (16)      | A0               |      |
| FMC_A1     | PF1 (17)      | A1               |      |
| FMC_A2     | PF2 (18)      | A2               |      |
| FMC_A3     | PF3 (19)      | A3               |      |
| FMC_A4     | PF4 (23)      | A4               |      |
| FMC_A5     | PF5 (24)      | A5               |      |
| FMC_SDNRAS | PF11 (70)     | RAS#             |      |
| FMC_A6     | PF12 (71)     | A6               |      |
| FMC_A7     | PF13 (74)     | A7               |      |
| FMC_A8     | PF14 (75)     | A8               |      |
| FMC_A9     | PF15 (76)     | A9               |      |
| FMC_A10    | PG0 (77)      | A10              |      |
| FMC_A11    | PG1 (78)      | A11              |      |
| FMC_A12    | PG2 (129)     | A12              |      |
| FMC_BA0    | PG4 (131)     | BA0              |      |
| FMC_BA1    | PG5 (132)     | BA1              |      |
| FMC_SDCLK  | PG8 (135)     | CLK              |      |
| FMC_SDNCAS | PG15 (191)    | CAS#             |      |
| FMC_NBL0   | PE0 (200)     | LDQM             |      |
| FMC_NBL1   | PE1 (201)     | UDQM             |      |

## CAN

- 3v3 CAN transceiver: TCAN337DCNR

| Label   | stm32f429bit6  | TCAN337DCNR | Note                                                    |
| ------- | -------------- | ----------- | ------------------------------------------------------- |
| CAN1_RX | ~~PA11 (145)~~ | RXD_1       | Used by USB_OTG_FS_DM                                   |
| CAN1_TX | ~~PA12 (156)~~ | TXD_1       | User by USB_OTG_FS_DP                                   |
| CAN1_RX | PB8 (198)      | RXD_1       |                                                         |
| CAN1_TX | PB9 (199)      | TXD_1       |                                                         |
| -       | -              | S_1 (8)     | Input: Drive high for silent mode, integrated pull down |
| -       | -              | Fault_1 (5) | Output: Open drain fault output pin.                    |
| CAN2_RX | PB5 (194)      | RXD_2       |                                                         |
| CAN2_TX | PB6 (195)      | TXD_2       |                                                         |
| -       | -              | S_2 (8)     | Input: Drive high for silent mode, integrated pull down |
| -       | -              | Fault_2 (5) | Output: Open drain fault output pin.                    |

## BLE - HCI interface (with ACN52840)

Dma 1: 

| Label    | stm32f429bit6       | TCAN337DCNT   | Note  |
| -------- | ------------------- | ------------- | ----- |
|          |                     | 2 P1.10       |       |
|          |                     | 3 P1.13       |       |
|          |                     | 4 P1.15       |       |
|          |                     | 5 P0.02/AIN0  |       |
|          |                     | 6 P0.03/AIN1  |       |
|          |                     | 7 P0.28/AIN4  |       |
|          |                     | 8 P0.29/AIN5  |       |
|          |                     | 9 P0.31/AIN7  |       |
| HCI_IRQ  | PE3 (2)             | 10 P0.30/AIN6 |       |
| HCI_MOSI | PE6 (SPI4_MOSI) (5) | 11 P0.05/AIN3 |       |
| HCI_CS   | PE4 (SPI4_NSS) (3)  | 14 P0.04/AIN2 |       |
| HCI_SCK  | PE2 (SPI4_SCK) (1)  | 15 P0.26      |       |
| HCI_MISO | PE5 (SPI4_MISO) (4) | 16 P0.27      |       |
|          |                     | 17 P0.06      |       |
|          |                     | 18 P0.10/NFC2 |       |
|          |                     | 19 P0.09/NFC1 |       |
|          |                     | 20 P0.08      |       |
|          |                     | 21 P1.09      |       |
|          |                     | 22 P0.12      |       |
|          |                     | 25 USB D-     |       |
|          |                     | 26 USB D+     |       |
|          |                     | 27 P0.13      |       |
|          |                     | 28 P0.15      |       |
|          |                     | 29 P0.17      |       |
|          |                     | 30 P0.24      |       |
| HCI_RST  | PG3 (130)           | 31 P0.18/RST  | Reset |
|          |                     | 33 SWDIO      |       |
|          |                     | 34 SWDCLK     |       |

## Shared

Pins shared between two potential used functions.

| Label          | stm32f429bit6 |
| -------------- | ------------- |
| MCO2 / SDIO_D1 | PC9           |
|                |               |


---

## Micros shortage

### Ideally
STM32F437VIT6TR
STM32F437VIT6
STM32F439IIT6
**STM32F439VIT6**
STM32F437ZIT7TR
STM32F437ZIT7
STM32F437ZIT6
STM32F437VIT7
STM32F437IIT6
STM32F439BIT6
**STM32F439ZIT6**

### Other solutions:

- STM32F429BIT6: https://lcsc.com/product-detail/Microcontroller-Units-MCUs-MPUs-SOCs_STMicroelectronics-STM32F429BIT6_C93564.html
- STM32F205RGT7: https://lcsc.com/product-detail/Microcontroller-Units-MCUs-MPUs-SOCs_STMicroelectronics-STM32F205RGT7_C967642.html

## SDRAM:

- Interesting discussion:
  - https://community.st.com/s/question/0D53W000006EWJlSAO/external-sdram-with-stm32f767-nucleo-board

## Ressources

- STM32 Nomenclature: https://www.digikey.com/en/maker/blogs/2020/understanding-stm32-naming-conventions
- https://www.altium.com/viewer/fr/