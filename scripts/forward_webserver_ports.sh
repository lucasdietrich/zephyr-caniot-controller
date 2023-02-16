#!/usr/bin/bash

# # Flush
# sudo iptables -F
# sudo iptables -t nat -F

# # Set rule
# sudo iptables -t nat -A PREROUTING -p tcp -d 192.0.2.1 --dport 80 -j DNAT --to 192.168.10.216:9080

# # Check
# sudo iptables -t nat -L -n

# # Apply
# sudo iptables-save
sudo sysctl -w net.ipv4.ip_forward=1

# QEMU net interface forwarded to nat
sudo iptables -t nat -I PREROUTING -p tcp -i ens160 --dport 9080 -j DNAT --to 192.0.2.1:80
sudo iptables -t nat -I PREROUTING -p tcp -i ens160 --dport 9443 -j DNAT --to 192.0.2.1:443

# USB ECM interface forwarded to nat
sudo iptables -t nat -I PREROUTING -p tcp -i ens160 --dport 8080 -j DNAT --to 192.0.3.2:80
sudo iptables -t nat -I PREROUTING -p tcp -i ens160 --dport 8443 -j DNAT --to 192.0.3.2:443