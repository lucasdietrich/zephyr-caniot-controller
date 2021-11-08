# SNTP_SIMPLE crash

```
*** Booting Zephyr OS build zephyr-v20700  ***
[00:00:01.569,000] <err> eth_stm32_hal: Failed to enqueue frame into RX queue: -115
[00:00:02.027,000] <inf> ethernet_if: net interface 0x20000ce8 UP !
Local (Europe/Paris) Date and time : 1970/01/01 00:00:02
[00:00:03.037,000] <inf> net_dhcpv4: Received: 192.168.10.240
[00:00:03.043,000] <inf> ethernet_if: Your address: 192.168.10.240
[00:00:03.050,000] <inf> ethernet_if: Lease time: 42824 seconds
[00:00:03.056,000] <inf> ethernet_if: Subnet: 255.255.255.0
[00:00:03.062,000] <inf> ethernet_if: Router: 192.168.10.1
[00:00:05.168,000] <err> net_time: sntp_simple() failed with error = -3
[00:00:05.175,000] <err> os: ***** USAGE FAULT *****
[00:00:05.181,000] <err> os:   Unaligned memory access
[00:00:05.187,000] <err> os: r0/a1:  0x00000002  r1/a2:  0x00000000  r2/a3:  0x20009398
[00:00:05.195,000] <err> os: r3/a4:  0x00000002 r12/ip:  0x00000000 r14/lr:  0x08012dbb
[00:00:05.204,000] <err> os:  xpsr:  0x21000000
[00:00:05.209,000] <err> os: Faulting instruction address (r15/pc): 0x08012da4
[00:00:05.217,000] <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
[00:00:05.225,000] <err> os: Current thread: 0x20001528 (sysworkq)
[00:00:05.232,000] <err> os: Halting system
```