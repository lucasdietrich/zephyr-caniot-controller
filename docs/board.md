# CANIoT Board

Choosen:
- Main microcontroller: STM32F429BIT6
- BLE module: ACN52840

Architecture, interfaces, chips, wiring, etc.

## SD Card

![pics/sd-card-pinout.png](./pics/sd-card-pinout.png)

## Miscelaneous

| Label | stm32f429bit6 | Note                |
| ----- | ------------- | ------------------- |
| MCO1  | PA8           |                     |
| MCO2  | PC9           | Shared with SDIO_D1 |


## SD Card (SPI)

| Label     | stm32f429bit6 | sdcard SPI | Note |
| --------- | ------------- | ---------- | ---- |
| PC13      | PC13          | 1 CS       |      |
| SPI3_MOSI | PC12          | 2 DI       |      |
| -         | -             | 3 VSS1     |      |
| -         | -             | 4 VDD      |      |
| SPI3_SCK  |               | 5 SCLK     |      |
| -         | -             | 6 VSS2     |      |
| SPI3_MISO |               | 7 D0       |      |
| -         | -             | 8 -        |      |
| -         | -             | 9 -        |      |

### SD Card (MMC)

| Label MMC | stm32f429bit6 | sdcard MMC | Note             |
| --------- | ------------- | ---------- | ---------------- |
| SDIO_D3   | PC11          | 1 CD/DAT3  |                  |
| SDIO_CMD  | PD2           | 2 CMD      |                  |
| -         | -             | 3 VSS1     |                  |
| -         | -             | 4 VDD      |                  |
| SDIO_CK   | PC12          | 5 SCLK     |                  |
| -         | -             | 6 VSS2     |                  |
| SDIO_D0   | PC8           | 7 DAT1     |                  |
| SDIO_D1   | PC9           | 8 DAT2     | Shared with MC02 |
| SDIO_D2   | PC10          | 9 DAT3     |                  |

## Ethernet

| ~~Label MII~~ | Label RMII  | stm32f429bit6 | ~~MII DP83848C~~ | **RMII DP83848C** | Note |
| ------------- | ----------- | ------------- | ---------------- | ----------------- | ---- |
| ETH_CRS       |             | PA0 /WKUP     | ?                | -                 |      |
| ETH_RX_CLK    | ETH_REF_CLK | PA1           | ?                | X1                |      |
| ETH_MDIO      | ETH_MDIO    | PA2           | ?                | MDIO              |      |
| ETH_COL       |             | PA3           | ?                | -                 |      |
| ETH_RX_DV     | ETH_CRS_DV  | PA7           | ?                | DX_DV             |      |
| ETH_RXD2      |             | PB0           | ?                | -                 |      |
| ETH_RXD3      |             | PB1           | ?                | -                 |      |
| ETH_TX_EN     | ETH_TX_EN   | PB11          | ?                | TX_EN             |      |
| ETH_MDC       | ETH_MDC     | PC1           | ?                | MDC               |      |
| ETH_TXD2      |             | PC2           | ?                | -                 |      |
| ETH_TX_CLK    |             | PC3           | ?                | -                 |      |
| ETH_RXD0      | ETH_RXD0    | PC4           | ?                | RXD_0             |      |
| ETH_RXD1      | ETH_RXD1    | PC5           | ?                | RXD_1             |      |
| ETH_TXD3      |             | PE2           | ?                | -                 |      |
| ETH_TXD0      | ETH_TXD0    | PG13          | ?                | TXD_0             |      |
| ETH_TXD1      | ETH_TXD1    | PG14          | ?                | TXD_1             |      |
|               |             |               |                  | **RX_ERR**        |      |


## SDRAM

| Label      | stm32f429bit6 | AS4C16M16SA-6TIN | Note |
| ---------- | ------------- | ---------------- | ---- |
| FMC_SDNWE  | PC0 (35)      | WE# (16)         |      |
| FMC_SDNE0  | PC2           | CS#              |      |
| FMC_SDCKE0 | PC3           | CKE              |      |
| FMC_D2     | PD0           | DQ2              |      |
| FMC_D3     | PD1           | DQ3              |      |
| FMC_D13    | PD8           | DQ13             |      |
| FMC_D14    | PD9           | DQ14             |      |
| FMC_D15    | PD10          | DQ15             |      |
| FMC_D0     | PD14          | DQ0              |      |
| FMC_D1     | PD15          | DQ1              |      |
| FMC_D4     | PE7           | DQ4              |      |
| FMC_D5     | PE8           | DQ5              |      |
| FMC_D6     | PE9           | DQ6              |      |
| FMC_D7     | PE10          | DQ7              |      |
| FMC_D8     | PE11          | DQ8              |      |
| FMC_D9     | PE12          | DQ9              |      |
| FMC_D10    | PE13          | DQ10             |      |
| FMC_D11    | PE14          | DQ11             |      |
| FMC_D12    | PE15          | DQ12             |      |
| FMC_A0     | PF0           | A0               |      |
| FMC_A1     | PF1           | A1               |      |
| FMC_A2     | PF2           | A2               |      |
| FMC_A3     | PF3           | A3               |      |
| FMC_A4     | PF4           | A4               |      |
| FMC_A5     | PF5           | A5               |      |
| FMC_SDNRAS | PF11          | RAS#             |      |
| FMC_A6     | PF12          | A6               |      |
| FMC_A7     | PF13          | A7               |      |
| FMC_A8     | PF14          | A8               |      |
| FMC_A9     | PF15          | A9               |      |
| FMC_A10    | PG0           | A10              |      |
| FMC_A11    | PG1           | A11              |      |
| FMC_A12    | PG2           | A12              |      |
| FMC_BA0    | PG4           | BA0              |      |
| FMC_BA1    | PG5           | BA1              |      |
| FMC_SDCLK  | PG8           | CLK              |      |
| FMC_SDNCAS | PG15          | CAS#             |      |
| FMC_NBL0   | PE0           | LDQM             |      |
| FMC_NBL1   | PE1           | UDQM             |      |

## CAN

- 3v3 CAN transceiver: TCAN337DCNT
- (MCP2551 not recommanded for new designs, prefer MCP2561) -> 5v only

| Label   | stm32f429bit6 | TCAN337DCNT | Note |
| ------- | ------------- | ----------- | ---- |
| CAN1_RX | PA11          | RXD         |      |
| CAN1_TX | PA12          | TXD         |      |
| CAN2_RX | PB5           | RXD         |      |
| CAN2_TX | PB6           | TXD         |      |

## BLE - HCI interface (with ACN52840)

Dma 1: 

| Label    | stm32f429bit6   | TCAN337DCNT   | Note  |
| -------- | --------------- | ------------- | ----- |
|          |                 | 2 P1.10       |       |
|          |                 | 3 P1.13       |       |
|          |                 | 4 P1.15       |       |
|          |                 | 5 P0.02/AIN0  |       |
|          |                 | 6 P0.03/AIN1  |       |
|          |                 | 7 P0.28/AIN4  |       |
|          |                 | 8 P0.29/AIN5  |       |
|          |                 | 9 P0.31/AIN7  |       |
| HCI_IRQ  | PE3             | 10 P0.30/AIN6 |       |
| HCI_MOSI | PE6 (SPI4_MOSI) | 11 P0.05/AIN3 |       |
| HCI_CS   | PE4 (SPI4_NSS)  | 14 P0.04/AIN2 |       |
| HCI_SCK  | PE2 (SPI4_SCK)  | 15 P0.26      |       |
| HCI_MISO | PE5 (SPI4_MISO) | 16 P0.27      |       |
|          |                 | 17 P0.06      |       |
|          |                 | 18 P0.10/NFC2 |       |
|          |                 | 19 P0.09/NFC1 |       |
|          |                 | 20 P0.08      |       |
|          |                 | 21 P1.09      |       |
|          |                 | 22 P0.12      |       |
|          |                 | 25 USB D-     |       |
|          |                 | 26 USB D+     |       |
|          |                 | 27 P0.13      |       |
|          |                 | 28 P0.15      |       |
|          |                 | 29 P0.17      |       |
|          |                 | 30 P0.24      |       |
| HCI_RST  | PG3             | 31 P0.18/RST  | Reset |
|          |                 | 33 SWDIO      |       |
|          |                 | 34 SWDCLK     |       |

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