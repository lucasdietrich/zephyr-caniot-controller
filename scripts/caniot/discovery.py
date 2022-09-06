#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

import socket
import random
import struct

DISCOVERY_PORT  = 5000
SEARCH_REQ_DATA = b"Search caniot-controller"
BROADCAST_IP    = "255.255.255.255"

class DiscoveryClient:
    def __init__(self):
        self.src_min_port = 10000
        self.src_max_port = 65000
        self.timeout = 5.0

    def get_src_rdm_port(self) -> int:
        return random.randint(49152, 65535)

    def parse_discovery_response(self, data: bytes):
        # ipraw, = struct.unpack('I', data[:4])
        # ipconverted = socket.inet_ntoa(data[:4])

        ipstr_raw = data[4:20]

        # return string until first null byte
        return ipstr_raw.split(b'\0', 1)[0].decode('utf-8')

    def lookup(self, ip: str = BROADCAST_IP):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        sock.settimeout(self.timeout)

        sock.bind(('', self.get_src_rdm_port()))

        sock.sendto(SEARCH_REQ_DATA, (ip, DISCOVERY_PORT))

        buf, addr = sock.recvfrom(32)

        return addr, buf, self.parse_discovery_response(buf)


if __name__ == "__main__":
    client = DiscoveryClient()

    addr, raw, parsed = client.lookup("192.0.2.1")

    print(f"Received {len(raw)} bytes from {addr} : parse = {parsed}")
    print(f"\t REST server http://{addr[0]}")