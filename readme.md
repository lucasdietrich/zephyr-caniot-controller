# Home Automation Controller for CAN/BLE devices - with ZephyrRTOS and stm32f4

## Introduction
This project is an attempt to create a home automation controller for CAN/BLE devices.
It serves my personnal needs but I tried to make it somehow generic with reusable components.
This project is far from being finished, it's also a mess but it's a start.
I have a bit more than 1 year of professionnal and personnal experience with Zephyr RTOS. 
And this project can also be seen of a demonstration of an IoT application handling several IoT protocols using Zephyr RTOS.

Comments, questions, suggestions and contributions are welcome.

Currently supporting **ZephyrRTOS v3.4.0** with following targets:
- `nucleo_f429zi`
- `mps2_an385`
- `qemu_x86`

I also aim to build a custom board dedicated to this project.

## Synoptic

Goal is to have a controller able to handle several IoT protocols like CAN, 
BLE (or Thread), Ethernet protocols, etc.. Goal is to gather all the data from
the different devices and send them to a cloud platform (AWS IoT Core for now) or
to a local server (REST, prometheus ..) for monitoring and/or control 
(like heating, lights, alarm etc..).

![docs/pics/caniot-controller-synoptic-components.png](./docs/pics/caniot-controller-synoptic-components.png)

## AWS - Grafana Dashboard

![docs/pics/aws_grafana_3.png](./docs/pics/aws_grafana_3.png)

## Documentation

Documentation is available in the `docs` folder for following topics:

- [Bluetooth (HCI host/controller)](./docs/bluetooth-hci.md)
- [HTTP server (REST, Prometheus, file server, webserver)](./docs/http-server.md)
  - [Swagger API for the REST server](./docs/swagger-local-api.yaml)
- [Credentials (AWS IoT, HTTPS server, etc.)](./docs/credentials.md)
- [UDP Discovery Server](./docs/discovery-server.md)
- [User IO (LEDs, buttons, etc.)](./docs/userio.md)
- [Cloud AWS](./docs/cloud-aws.md)
- [Home Automation (HA) - Devices and events model](./docs/devices.md)
- [USB CDC ACM / ECM (networking)](./docs/usb.md)
- [Controller Schematic/PCB wiring draft](./docs/board.md)
- [Configuration](./docs/configuration.md)
- [CANIOT](./docs/caniot.md)
- Enabling/disabling features
  - CAN configuration (CANIOT)
  - BLE configuration
  - AWS client
  - LUA
  - ...

## Getting started

### Prerequisites

In order to build this project you need the Zephyr RTOS toolchain. Please refer 
to the [Zephyr RTOS Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

I advice to use a Linux machine for this project, Zephyr SDK should now be
available for Windows but I didn't try it yet. I personally use a Fedora 36
virtual machine.

This project is entirely compliant with the Zephyr RTOS tooling like `west`, 
the `Zephyr SDK`, `Zephyr net-tools` (`twister` in the future).

### Get the project

Let's suppose you are in your home directory : `cd ~`

The project contains a [west.yml](./west.yml) file, you can clone the project using `west`:

    west init -m git@github.com:lucasdietrich/zephyr-caniot-controller.git zephyr-caniot-workspace
    cd zephyr-caniot-workspace
    west update

Finally initialize application submodules (should be enhanced in the future):

    cd zephyr-caniot-controller
    git submodule update --init --recursive

Once you have created the workspace you should have the following file structure:

```
[lucas@fedora zephyr-caniot-workspace]$ tree -L 1
.
├── bootloader
├── modules
├── zephyr-caniot-controller
├── tools
└── zephyr
```

I also advice to create a python virtual environment in your workspace:

    cd ~/zephyr-caniot-workspace
    python3 -m venv .venv
    source .venv/bin/activate

Then install Zephyr RTOS and application requirements:
    pip install -r zephyr/scripts/requirements.txt
    pip install -r zephyr-caniot-controller/requirements.txt


## Configure the project

Once you have cloned the project, go to the application directory `cd zephyr-caniot-controller` :

Configuration is the result of several files:
- Root [prj.conf](./prj.conf) file contains the configuration common to all boards:
Following files contain the configuration specific to each board.
  - [boards/nucleo_f429zi.conf](./boards/nucleo_f429zi.conf)
  - [boards/mps2_an385.conf](./boards/mps2_an385.conf)
  - [boards/qemu_x86.conf](./boards/qemu_x86.conf)
- Finally [overlays/](./overlays/) contains specific configuration to enable features.

If you want to enable a feature, you need to add the corresponding overlay file 
to the `OVERLAY_CONFIG` cmake variable, as follows, for further details please refer 
to [this Makefile](./Makefile).

    west build -b nucleo_f429zi -- -DOVERLAY_CONFIG="overlays/nucleo_f429zi_shell.conf"

You may want to enable a specific feature, to do so, set `-DOVERLAY_CONFIG` 

If you're using features that requires credentials (like HTTPS server of AWS IoT),
please refer to the [docs/credentials.md](./docs/credentials.md) documentation.

### Configure for QEMU targets

If you want to build for QEMU targets, take a look to `CONFIG_QEMU_ICOUNT` and
related configuration options.

### Flash the bootloader

You will need to flash the MCUBoot bootloader to be able to run the application 
on a real board. You can find the bootloader in the `bins` directory:
- [bins/bootloader_debug.bin](./bins/bootloader_debug.bin)

You can flash it using `make flash_bootloader`, from the project root directory.

Caution: The command erases the whole flash memory.

Note: In case you want to sign and encrypt your own applications, you will need
to build and flash a custom bootloader.

More details are available here: [docs/mcuboot.md](./docs/mcuboot.md).

### Build the project

To build the project, make sure you have activated the Python virtual environment 
with `source ../.venv/bin/activate`, then run the following command (for `nucleo_f429zi`):

    west build -b nucleo_f429zi

For `mps2_an385`:

    west build -b mps2_an385

For `qemu_x86`:

    west build -b qemu_x86

If compilation is successful, you should get following output, for `nucleo_f429zi`:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      502356 B         1 MB     47.91%
            SRAM:      125878 B       192 KB     64.02%
             CCM:         64 KB        64 KB    100.00%
        IDT_LIST:          0 GB         2 KB      0.00%
```    

In case of error, please refer to the troubleshooting section below.

### Flash and run (Nucleo F429ZI)

With west simply flash with `west flash`

### QEMU Networking

If you want to run the application on QEMU with networking, more configuration is required.
- Clone `git clone zephyrproject-rtos/net-tools` in your zephyr workspace (`zedpyr-caniot-workspace`).
- Build the tools with `cd net-tools && make`

The tools are now available to the scripts located in [scripts/](./scripts/)

## `qemu_x86` target

`qemu_x86` uses SLIP TAP networking.

The script [scripts/prepareqemunet.sh](./scripts/prepareqemunet.sh) prepare
the networking configuration for QEMU. But please refer to the official documentation
for further details : [Networking with QEMU](https://docs.zephyrproject.org/3.0.0/guides/networking/qemu_setup.html)

So open a terminal and launch the script (not as sudo):

    ./scripts/prepareqemunet.sh

You should see the created `tap0` interface
```
[lucas@fedora zephyr-caniot-controller]$ ifconfig
tap0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.0.2.2  netmask 255.255.255.0  broadcast 0.0.0.0
        inet6 2001:db8::2  prefixlen 64  scopeid 0x0<global>
        inet6 fe80::c894:12ff:fefa:2cdf  prefixlen 64  scopeid 0x20<link>
        ether ca:94:12:fa:2c:df  txqueuelen 1000  (Ethernet)
        RX packets 159  bytes 15931 (15.5 KiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 143  bytes 29149 (28.4 KiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

In this guide we are using the TAP interface but there are other options available
as described here [Networking with the host system](https://docs.zephyrproject.org/3.0.0/guides/networking/networking_with_host.html).

Then you can run the application (built for `qemu_x86`) :

    west build -t run

Finally if you want to expose your QEMU HTTP server, you may
configure and run the script [scripts/forward_webserver_ports.sh](./scripts/forward_webserver_ports.sh) :

    ./scripts/forward_webserver_ports.sh

## `mps2_an385` target

`mps2_an385` supports both `slip` and real `lan9220` hardware.
- If using `lan9220` hardware, run the script `sudo ../net-tools/net-setup.sh` (`sudo`) to start the interface.
- If using `slip` interface, as for `qemu_x86`, run `./scripts/prepareqemunet.sh`

Then run your application with `west build -t run`
    west build -t run



## Debug (VS Code)

Debug can be launch on QEMU targets using `ninja debugserver -C build` command

### Nucleo F429ZI

Debug of the nucleo_f429zi board is done using the ST-Link V2 debugger with OpenOCD.

Use one of the the configurations `Launch cortex-debug nucleo_429zi` and `Attach cortex-debug nucleo_429zi` 
available in the [launch.json](./.vscode/launch.json) file.

If OpenOCD configuration doesn't work try the configuration `BACKUP cortex-debug STutil`, but 
I discourage using it and you will need to install ST-Link utilities.

- Select the configuration
- Then press `F5`

![./docspics/caniot-controller-nucleo-f429zi-debug.png](./docs/pics/caniot-controller-nucleo-f429zi-debug.png)

### mps2_an385
Debug of the mps2_an385 board is done using the gdbserver provided by QEMU.

- Run `ninja debugserver -C build` 
- Then use configuration `(gdb) mps2_an385`.
- Then press `F5`

### QEMU x86
Debug of the qemu_x86 board is done using the gdbserver provided by QEMU.

- Run `ninja debugserver -C build` 
- Then use configuration `(gdb) qemu_x86`.
- Then press `F5`
![./docs/pics/caniot-controller-qemux86-debug.png](./docs/pics/caniot-controller-qemux86-debug.png)

### QEMU ARM (TODO)

With `EXPERIMENTAL (gdb) QEMU ARM` and `LEGACY (gdb) QEMU ARM`

**TODO**

### Agnostic (deprecated)

You can launch debug on any target using the `ninja debugserver -C build` command
with `BACKUP (gdb) Agnostic` configuration. But I advice to use the configuration
specific to the target.

### Wireshark (QEMU)

To see network traffic generated by the application in QEMU, use one of the following commands:

- If you are on a windows host, with your QEMU instance running on a Linux guest, 
run the following command in a git bash terminal:

Interface to monitor is `zeth` (with `lan9220` for example):

    ssh lucas@fedora sudo tcpdump -U -s0 'not port 22' -i zeth -w - | "C:\Program Files\Wireshark\Wireshark.exe" -k -i -

Interface to monitor is `tap0` :

    ssh lucas@fedora sudo tcpdump -U -s0 'not port 22' -i tap0 -w - | "C:\Program Files\Wireshark\Wireshark.exe" -k -i -

If you are on a Linux host, with your QEMU instance running on a Linux guest, use the following command :

    ssh lucas@fedora sudo tcpdump -U -s0 'not port 22' -i tap0 -w - | wireshark -k -i -

If your wireshark and QEMU are on the same machine (`zeth` instead of `tap0` if not using slip):

    sudo tcpdump -U -s0 'not port 22' -i tap0 -w - | wireshark -k -i -

## Features

Progress if not percentage :
- `P` for planned
- `E` for evaluated (soon to be implemented)
- `T` for testing
- `D` for done
- `A` for abandoned

Priority: grade from 1 to 10.

| Module           | Feature                                 | Progress | Priority | Issues |
| ---------------- | --------------------------------------- | -------- | -------- | ------ |
| HTTP Server      |                                         |          |          |        |
|                  | Concurrent connections                  | D        |          |        |
|                  | Keep-alive support                      | D        |          |        |
|                  | Static files                            | 15%      |          |        |
|                  | HTTPS                                   | D        |          |        |
|                  | HTTPS Client Authentication             | 0%       |          |        |
|                  | Chunked encoding (request)              | D        |          |        |
|                  | Chunked encoding (response)             | D        |          |        |
|                  | Webserver                               | 0%       | 5        |        |
|                  | REST server                             | D        |          |        |
|                  | Prometheus metrics client               | D        |          |        |
|                  | File download                           | 50%      |          |        |
|                  | Multipart parser (for files)            | 0%       | 5        |        |
| BLE              |                                         |          |          |        |
|                  | HCI Interface                           | E        |          |        |
|                  | GATT Server (phone application)         | P        | 10       |        |
|                  | GATT Client                             | P        |          |        |
|                  | Xiaomi data collector                   | E        |          |        |
| CAN              |                                         |          |          |        |
|                  | CAN Interface                           | D        |          |        |
|                  | CANIOT (class0)                         | T        |          |        |
|                  | CANIOT (class1)                         | 25%      | 10       |        |
|                  | CANIOT (telemetry)                      | T        | 10       |        |
|                  | CANIOT (command)                        | 0%       | 10       |        |
| LUA              |                                         | A        |          |        |
|                  | Lua interpreter                         | DA       |          |        |
|                  | Lua orchestrator                        | 10% A    |          |        |
|                  | OS Module for LUA                       | 0% A     |          |        |
|                  | Devices module for LUA                  | 0% A     |          |        |
|                  | REST API                                | 25% A    |          |        |
| HA               |                                         | P        |          |        |
| CLOUD            |                                         |          |          |        |
|                  | AWS IoT (publish)                       | T        |          |        |
|                  | AWS IoT (remote control)                | 0% P     |          |        |
|                  | AWS Iot (remote config - device shadow) | 0%       |          |        |
| FS               |                                         |          |          |
|                  | SD SPI FAT32                            | D        |          |        |
|                  | SD MCC FAT32                            | 0% P     |          |        |
|                  | SD card FAT32 formatting                | 0% P     |          |        |
|                  | NVS FLASH                               | D        |          |        |
| Discovery server |                                         |          |          |        |
|                  | UDP Discovery server                    | 80% P    |          |        |
| User IO          |                                         |          |          |        |
|                  | LEDs                                    | E        |          |        |
|                  | Buttons                                 | E        |          |        |
| Stats            |                                         |          |          |        |
|                  | Heap memory usage                       | P        |          |        |
|                  | Mbedtls memory usage                    | P        |          |        |
|                  | HA stats                                | P        |          |        |
|                  | CAN stats                               | P        |          |        |
|                  | NET stats                               | P        |          |        |
|                  | BLE stats                               | P        |          |        |
| Thread/matter    |                                         | P        |          |        |


## Footprint

Current footprint `nucleo_f429zi` build (debug):
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

## clang-format

- Install clang-format, and run `make format` to format all files
- Or install `xaver.clang-format` extension for VS Code, and format on save or
format using `Ctrl + K, Ctrl + F`


## Known issues

- MAJOR: SD FAT FS internal state gets corrupted when doing many requests in a short time.
- minor: Fix `-Waddress-of-packed-member` during `xiaomi_dataframe_t` handling
- CANIOT send command with small timeout (100ms) always fails
  - ISR seems to be called very late
  - Set `CONFIG_CAN_LOG_LEVEL_DBG=y` 
- FS Access  (upload/download) -> do a stress test

## TODO

### Immediate
- Reduce MBEDTLS memory usage by 1/3
- Remove dependencies on CANIOT_LIB, CANIOT_CONTROLLER, HA, so that the code
can be compiled without them.
- Add general diagnostic messages
- Look at zephyr Settings Subsystem and this sample: samples/subsys/settings/src/main.c
- **Fix use of lan9220 ethernet driver with mps2_an385**
- Investigate crash for emulated devices in QEMU:
```
ASSERTION FAIL [atomic_get(&ev->ref_count) == (atomic_val_t)0] @ WEST_TOPDIR/zephyr-caniot-controller/src/ha/devices.c:867
[00:07:59.400,000] <err> ha_dev: k_fifo_alloc_put() error -12
[00:07:59.400,000] <err> ha_dev: Failed to notify event 0x20093868 to 0x20093988, err=-12
```

### Long term

- Move several buffers to CCM to keep space in SRAM for Newlib heap (for LUA)
- ~~Allow to embed LUA script when building for `nucleo_f429zi`~~
- Cannot uses 2 sockets chrome with keepalive
- Use [docs/ram_report.txt](./docs/ram_report.txt) to optimize memory usage
  - DNS buffers
- Bit of C++ ?
- HTTP:
  - Make requests processing completely assynchronous (with concurrent requests)
    - Using workqueue to process requests ?
  - Allow to stream response -> helps to decrease http buffer size
- ~~Draft LUA scripts orchestrator~~
- REST: list files in filesystem
- HTTP: allow to download files from the filesystem
- Allow to store few scripts in the SoC ROM
- Test keepalive feature
- Allow authentication using "username:password" and x509 certificates
- Find a way to be able to print numbers from LUA with CONFIG_NEWLIB_LIBC_NANO enabled (for ARM)
- Find a way to redirect stdout and stderr to a file or to logging system
  - Check `zephyr/lib/libc/newlib/libc-hooks.c`
- `CONFIG_NEWLIB_LIBC_MIN_REQUIRED_HEAP_SIZE` seems to have no effect
- Create a totally agnostic API for devices, allowing to add many kind of MAC 
addr abstraction layers.

## Troubleshooting
- For an unkown reason yet, make sure to have docker disabled when using QEMU with networking. To disable it run:

    sudo systemctl disable --now docker.service
    sudo reboot

## Ressources:
- [Zephyr Documentation - West Manifests](https://docs.zephyrproject.org/3.0.0/guides/west/manifest.html)
- TODO Add all the ressources !