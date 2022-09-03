# Home Automation Controller for CAN/BLE devices - with ZephyrRTOS and stm32f4

## Introduction
This project is an attempt to create a home automation controller for CAN/BLE devices.
It serves my personnal needs but I tried to make it somehow generic with reusable components.
This project is far from being finished, it's also a mess but it's a start.
I have a bit more than 1 year of professionnal and personnal experience with Zephyr RTOS. 
And this project can also be seen of a demonstration of an IoT application handling several IoT protocols under Zephyr RTOS.

Currently two boards are supported: nucleo_f429zi and qemu_x86 but I also aim to build a custom board for it.


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


## Footprint

Default `nucleo_f429zi` build (debug):
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      498428 B         1 MB     47.53%
            SRAM:      119286 B       192 KB     60.67%
             CCM:         64 KB        64 KB    100.00%
        IDT_LIST:          0 GB         2 KB      0.00%
```

## Demo

 HTTP upload of a LUA script to SD card then execute.
```
somecalc1.lua
1661031779
0.505708
[00:01:32.512,000] <inf> http_server: (4) Connection accepted from 192.168.10.216:54476, cli sock = 7
[00:01:32.514,000] <inf> files_server: Filepath: /SD:/lua/somecalc1.lua
[00:01:32.527,000] <inf> files_server: File /SD:/lua/somecalc1.lua upload succeeded [size = 389]
[00:01:32.528,000] <inf> http_server: (7) Req /files [payload 389 B] returned B resp status 200 [payload 16 B] (keep-alive=0)
[00:01:32.544,000] <inf> http_server: (7) Connection closed by peer
[00:01:32.544,000] <inf> http_server: (7) Closing sock conn 0x20003e54
[00:01:32.545,000] <inf> http_server: (4) Connection accepted from 192.168.10.216:54486, cli sock = 7
[00:01:32.557,000] <dbg> lua: lua_orch_script_handler: (0x20010bb4) Executing script ...
[00:01:32.595,000] <dbg> lua: lua_orch_script_handler: (0x20010bb4) Script returned res=0...
[00:01:32.595,000] <inf> lua: (0x20010bb4) Script returned 0 (LUA_OK)
[00:01:32.596,000] <inf> http_server: (7) Req /lua/execute [payload 0 B] returned B resp status 200 [payload 45 B] (keep-alive=0)
[00:01:32.598,000] <inf> http_server: (7) Connection closed by peer
[00:01:32.598,000] <inf> http_server: (7) Closing sock conn 0x20003e98
```

## Known issues

- MAJOR: SD FAT FS internal state gets corrupted when doing many requests in a short time.
- minor: Fix `-Waddress-of-packed-member` during `xiaomi_dataframe_t` handling



## TODO

- Move several buffers to CCM to keep space in SRAM for Newlib heap (for LUA)
- Allow to embed LUA script when building for `nucleo_f429zi`

---

# Debug network remotely using Wireshark and tcpdump

- On windows: `ssh lucas@fedora sudo tcpdump -U -s0 'not port 22' -i tap0 -w - | "C:\Program Files\Wireshark\Wireshark.exe" -k -i -`
- On linux: `ssh lucas@fedora sudo tcpdump -U -s0 'not port 22' -i tap0 -w - | wireshark -k -i -`

