# MCU Boot

## Swap partition:

- **https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/mcuboot/readme-zephyr.html**

- https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/mcuboot/design.html#swap-using-scratch

> The scratch area must have a size that is enough to store at least the largest sector that is going to be swapped

> he scratch is only ever used when swapping firmware, which means only when doing an upgrade. Given that, the main reason for using a larger size for the scratch is that flash wear will be more evenly distributed, because a single sector would be written twice the number of times than using two sectors, for example.

## Commands

```
cmake -GNinja -DBOARD=nucleo_f429zi -B build -DDTC_OVERLAY_FILE="boards/nucleo_f429zi.overlay"
```

## Note

`--pad` argument is required for `imgtool` to be valid for mcuboot on slot 1.

Read: https://github.com/mcu-tools/mcuboot/issues/488#issuecomment-496913836