#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from pprint import pprint
from caniot.controller import Controller

ip = "192.168.10.240"
ip = "192.0.2.1"

c = Controller(ip, False)

res = c.upload("./scripts/lua/somecalc1.lua", chunks_size=1024)
res = c.run_script("somecalc1.lua")

print(res.json())