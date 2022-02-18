import socket
import random
import struct
from unicodedata import decimal

def get_random_port() -> int:
        return random.randint(10000, 60000)

def parse_discovery_response(data: bytes):
        ipraw = data[:4]
        ipstr = struct.unpack('I', ipraw)

        return ipstr, data[4:]

data = b"Search caniot-controller"

BROADCAST_ADDR = "255.255.255.255"
IP = "192.168.10.240"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

sock.settimeout(5.0)

sock.bind(('', get_random_port()))

sock.sendto(data, (IP, 5000))

print(f"Sent {data.decode()}, waiting for response : ...")

buf, addr = sock.recvfrom(32)

ip, raw = parse_discovery_response(buf)

print(f"Received {len(buf)} bytes from {addr} : parse = {raw}")
print(f"\t REST server http://{addr[0]}")
