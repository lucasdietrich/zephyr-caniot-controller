import socket
import random
import struct

def get_random_port() -> int:
        return random.randint(10000, 60000)

def parse_discovery_response(data: bytes):
        ipraw = struct.unpack('L', data[:4])
        ipstr = data[4:]

        return ipraw, ipstr

data = b"Search caniot-controller"

BROADCAST_ADDR = "255.255.255.255"
IP = "192.168.10.240"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

sock.bind(('', get_random_port()))

sock.sendto(data, (IP, 5000))

buf, addr = sock.recvfrom(32)

print(f"Received {len(buf)} bytes from {addr} : parse = {parse_discovery_response(buf)[1]}")
