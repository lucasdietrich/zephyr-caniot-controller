import socket

req = b"""
GET /path/2 HTTP/1.1
Host: 192.168.10.240
User-Agent: python-requests/2.26.0
Accept-Encoding: gzip, deflate
Accept: */*
Connection: close
Content-Length: 17
Content-Type: application/json
Authorization: Basic bHVjYXM6cGFzc3dvcmQh


{"user": "Lucas"}
"""

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

sock.settimeout(5.0)

sock.connect(("192.168.10.240", 80))

sock.send(req)

data = sock.recv(1024)

print(f"[{len(data)}] {data}")

sock.close()