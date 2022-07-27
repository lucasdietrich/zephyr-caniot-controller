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

res = c.list_lua_scripts(True)

print(res, res.status_code)
if res.status_code == 200:
    pprint(res.json())
