# Settings

We are using Zephyr `settings` subsystem to handle settings for our application :
- https://docs.zephyrproject.org/latest/services/settings/index.html

For real board, these `settings` are stored in the NVS.

## General

| Setting name | Type       | Description     | Default             |
| ------------ | ---------- | --------------- | ------------------- |
| `name`       | string[32] | Controller name | `CANIoT Controller` |

### Features

| Setting name     | Type | Description                       | Default |
| ---------------- | ---- | --------------------------------- | ------- |
| `features/can`   | bool | Enable CAN interface and features | `true`  |
| `features/ble`   | bool | Enable BLE interface and features | `true`  |
| `features/cloud` | bool | Enable cloud features             | `true`  |
| `features/lua`   | bool | Enable LUA features               | `true`  |
| `features/web`   | bool | Enable Webserver features         | `true`  |

## Network

| Setting name      | Type       | Description  | Default         |
| ----------------- | ---------- | ------------ | --------------- |
| `network/dhcp`    | bool       | Use DHCP     | `true`          |
| `network/ipv4`    | string[15] | IPv4 address | `192.168.0.80`  |
| `network/mask`    | string[15] | IPv4 mask    | `255.255.255.0` |
| `network/gateway` | string[15] | Gateway      |                 |
| `network/dns`     | string[15] | DNS          | `8.8.8.8`       |

## SNTP

| Setting name  | Type       | Description | Default           |
| ------------- | ---------- | ----------- | ----------------- |
| `sntp/server` | string[64] | SNTP server | `fr.pool.ntp.org` |
| `sntp/port`   | uint       | SNTP port   | `123`             |

## Webserver

| Setting name       | Type | Description | Default |
| ------------------ | ---- | ----------- | ------- |
| `webserver/port`   | uint | Port        | `80`    |
| `webserver/secure` | bool | Use HTTPS   | `true`  |


## CAN

| Setting name   | Type | Description                      | Default |
| -------------- | ---- | -------------------------------- | ------- |
| `can/0/enable` | bool | Enable CAN0 interface            | `true`  |
| `can/1/enable` | bool | Enable CAN1 interface            | `true`  |
| `can/0/speed`  | bool | CAN0 Interface speed (kbits/sec) | 500000  |
| `can/1/speed`  | bool | CAN1 Interface speed (kbits/sec) | 500000  |


## BLE

## Home Automation

### Devices support

| Setting name       | Type | Description                          | Default |
| ------------------ | ---- | ------------------------------------ | ------- |
| `ha/device/caniot` | bool | Add support for CANIoT devices       | `true`  |
| `ha/device/xiaomi` | bool | Add support for Xiaomi Mijia devices | `false`  |
| more ...           | bool |                                      |         |

## Cloud

## Runtime

- Settings related to performances, debug, memory usage, etc.