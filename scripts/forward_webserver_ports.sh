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
