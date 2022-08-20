#!/bin/bash

# DON'T RUN THIS SCRIPT AS SUDO/ROOT

sudo iptables -t nat -A POSTROUTING -j MASQUERADE -s 192.0.2.1
sudo sysctl -w net.ipv4.ip_forward=1

../net-tools/loop-socat.sh &
sudo ../net-tools/loop-slip-tap.sh