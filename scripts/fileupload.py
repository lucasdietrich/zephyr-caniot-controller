#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from pprint import pprint
from caniot.controller import Controller

ip = "192.0.2.1"
ip = "192.168.10.240"

t = Controller(ip, False)

N = 80

size = (N*1024 + 77)

with open("/tmp/test.txt", "wb") as f:
    f.write(b"a"*size)

res = t.upload("/tmp/test.txt",
               chunks_size=1024,
               filepath="tmp/test2.big",
               lfn=True)

print(res, res.status_code)
pprint(res.text)