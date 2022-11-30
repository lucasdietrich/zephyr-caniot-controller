## Users LED and buttons

Switch between modes by pressing the button on the board.

Modes:
1. `status` (defaut)
2. `can`
3. `ble`
4. `net`

Because of lack of support of complementary PWM channels on STM32 timers,
blinking mode is not available on the red LED.

## Mode status (default)

| Led                    | Green (NET)       | Red (CAN)           | Blue (BLE)      |
| ---------------------- | ----------------- | ------------------- | --------------- |
| Off                    | (default)         | (default)           | (default)       |
| Blinking quickly (5Hz) | fatal error       | ~~fatal error~~     | fatal error     |
| Blinking slowly (1Hz)  | DHCP/SNTP running | ~~reconfiguration~~ | reconfiguration |
| Flashing               | rx/tx             | rx/tx               | rx/tx           |
| Steady (if long press) | NET up            | CAN up              | BLE up          |

# Mode Interface (CAN/BLE/NET)

| Led                    | Green (events) | Red (status) | Blue (configuration) |
| ---------------------- | -------------- | ------------ | -------------------- |
| Off                    | down           | down         | down                 |
| Blinking quickly (5Hz) | ---            | ---          | ---                  |
| Blinking slowly (1Hz)  | pending        | ---          | ---                  |
| Flashing               | ---            | ---          | rx/tx                |
| Steady                 | up             | fatal error  | ---                  |
