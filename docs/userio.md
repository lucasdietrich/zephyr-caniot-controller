## Users LED and buttons

| Led                    | Green                     | Red                       | Blue                 |
| ---------------------- | ------------------------- | ------------------------- | -------------------- |
| Description            | Ethernet inteface         | Can interface             | BLE/Thread interface |
| Off                    | interface down            | interface up - no traffic | ---                  |
| Blinking quickly (5Hz) | ---                       | interface down            | ---                  |
| Blinking slowly (1Hz)  | DHCP running              | ---                       | ---                  |
| Flashing               | ---                       | packet received or sent   | ---                  |
| Steady                 | interface UP + IP address | error                     | ---                  |
| Toggle                 | ---                       | ---                       | On RX Msg            |
