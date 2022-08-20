#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from pprint import pprint
from caniot.controller import Controller
import time

ip = "192.0.2.1"
ip = "192.168.10.240"

c = Controller(ip, False)

for i in range(100):
    res = c.upload("./scripts/lua/somecalc1.lua", chunks_size=1024)
    res = c.run_script("somecalc1.lua")
    time.sleep(1)

# print(res.json())

print(res, res.status_code, res.text)