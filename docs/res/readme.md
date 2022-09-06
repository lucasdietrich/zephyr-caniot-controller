# Documentation

## Index

- [FLASH](./flash.md)


## Branches

- `can/poc` : PoC
- `can/socket` : socket can API 
- `can/drivers_level` : drivers API 

## Flash

Idea : 
- Bootloader : 64KB (4*16KB)
- Application : 1MB - 64 KB
- Storage (config) : 4*16KB + 64KB
- Live-values : 128KB*... 
- Certificates : 128KB


---

## stm32f429zi

[STM32F429ZI](https://www.st.com/en/microcontrollers-microprocessors/stm32f429zi.html)
- 2MB Flash
- 180MHz
- 192kB SRAM
- 64kB CCM

![](https://www.st.com/content/ccc/fragment/product_related/rpn_information/product_circuit_diagram/group0/8d/9e/28/ab/4a/5b/45/ca/bd_stm32f429xi_2m/files/bd_stm32f429xi_2m.jpg/jcr:content/translations/en.bd_stm32f429xi_2m.jpg)

---

## HTTPS server

- Private key certificate passphrase : `caniot` :