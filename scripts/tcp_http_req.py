#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

import socket
import time
from random import shuffle

# "192.168.10.240"
ip = "192.0.2.1"
parallel_requests = 1
requests_count = 5

req = b"""GET /info HTTP/1.1
Host: 192.168.10.240
User-Agent: python-requests/2.26.0
Accept-Encoding: gzip, deflate
Accept: */*
Connection: keep-alive
Content-Length: 17
Content-Type: application/json
Authorization: Basic bHVjYXM6cGFzc3dvcmQh

{"user": "Lucas"}"""

req = b"""GET /info HTTP/1.1
Host: 192.168.10.240
Connection: keep-alive
Content-Length: 17
Content-Type: application/json

{"user": "Lucas"}"""

req.replace(b"\n", b"\r\n")

a = time.time()

sock = [0] * parallel_requests

for i in range(parallel_requests):
        sock[i] = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock[i].settimeout(5.0)
        sock[i].connect((ip, 80))

b = time.time()

# time.sleep(0.25)

shuffle(sock)

for j in range(requests_count):
        for i in range(parallel_requests):
                print("Sent : ", len(req), req)
                sock[i].send(req)
                data = sock[i].recv(1024)
                # print(f"[{len(data)}] {data}")
                print("Received :", len(data), data[:80])

                # time.sleep(0.25)
        shuffle(sock)

c = time.time()

for i in range(parallel_requests):
        sock[i].close()

d = time.time()

print(f"d - a = {d - a: .3f} s / {b - a: .3f} / {c - b: .3f} / {d - c: .3f}")