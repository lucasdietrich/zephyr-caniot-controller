#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from pprint import pprint
from caniot.controller import Controller
import time

ip = "192.168.10.240"
ip = "192.0.2.1"

c = Controller(ip, False)

N = 1

lua_script = "ha.lua"

for i in range(N):
    res = c.upload(f"./scripts/lua/{lua_script}", chunks_size=1024)
    res = c.run_script(lua_script)

    if i < N - 1:
        time.sleep(1)

# print(res.json())

print(res, res.status_code, res.text)