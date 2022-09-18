#!/bin/bash

# DON'T RUN THIS SCRIPT AS SUDO/ROOT

# Please refer to official documentation
# https://docs.zephyrproject.org/3.0.0/guides/networking/qemu_setup.html
sudo iptables -t nat -A POSTROUTING -j MASQUERADE -s 192.0.2.1
sudo sysctl -w net.ipv4.ip_forward=1

../net-tools/loop-socat.sh &

sudo ../net-tools/loop-slip-tap.sh

# Keep the terminal open ...
