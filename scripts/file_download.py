#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from pprint import pprint
from caniot.controller import Controller

ip = "192.168.10.240"
ip = "192.0.2.1"

ctrl = Controller(ip, False)

resp = ctrl.download("/RAM:/lua/math.lua")

print(resp, resp.status_code, len(resp.text))
print()
print()
print(resp.text)