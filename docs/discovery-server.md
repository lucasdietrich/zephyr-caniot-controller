# UDP Discovery Server

The UDP Discovery Server is a simple UDP server that can be used to discover
the controller.

## Configuration options

| Option | Description |
| ------ | ----------- |
| **`DISCOVERY_SERVER`** | Globally enable UDP Discovery Server |

## Usage

The UDP Discovery Server listens for UDP packets on port 5000.

It responds to unicast or broadcast packets with the following payload:

        Search caniot-controller

And it responds with the following binary payload:

```c
struct discovery_response {
	uint32_t ip; /* Networking Byte Order */
	char str_ip[NET_IPV4_ADDR_LEN]; /* Null terminated IP address*/
} __attribute__((__packed__));
```

## Example

Using python script `python3 scripts/discover.py` to discover the controller:

Expected output:
```
[lucas@fedora zephyr-caniot-controller]$ python3 scripts/discover.py
Address ('192.168.10.240', 5000) responded with 20 bytes : parse = 192.168.10.240
```