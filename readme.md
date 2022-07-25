# Home Automation Controller for CAN/BLE devices - with ZephyrRTOS and stm32f4

## Introduction

## Features

- developped, in progress, incoming

- HTTP
  - Concurrent connections (up to 3)
  - Keep-alive support
  - HTTP/HTTPS (TLS 1.2)
  - Chunked encoding for requests (stream)
  - **TODO** Chunked encoding for responses (stream)
  - REST server base
  - WEB server base
  - **TODO** Multipart parser
  - Prometheus metrics client
  - **TODO** File download
  - WebServer
- BLE controller
  - Interface: IPC with nrf52840dk
  - Data collector
- CAN controller
  - Interface
  - CANIOT controller
- LUA
  - **TODO** orchestrator
  - **TODO** OS module
  - **TODO** Application programming interface (API)
- Cloud
  - **TODO** AWS IoT 
    - Request/response
    - Device update
    - Device sync with Device Shadow
- Filesystem
  - **PoC** Settings (NVS)
  - RAM FAT FS
  - **PoC** SD FAT FS
  - **TODO** SD FAT FS formatting
- **PoC** HA abstraction layer 
- UDP discovery server
- Userio
  - **PoC** LEDS
  - **PoC** Button
- Stats
  - **TODO** HEAP usage (newlibc)
  - MbedTLS memory usage
  - Stacks usage
- Sensors:
  - I2C Temperature sensor

Other ideas:
- Thread/Matter
- 

---

# Debug network remotely using Wireshark and tcpdump

- On windows: `ssh lucas@fedora sudo tcpdump -U -s0 'not port 22' -i tap0 -w - | "C:\Program Files\Wireshark\Wireshark.exe" -k -i -`
- On linux: `ssh lucas@fedora sudo tcpdump -U -s0 'not port 22' -i tap0 -w - | wireshark -k -i -`