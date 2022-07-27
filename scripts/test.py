#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from pprint import pprint
from caniot.test import TestClient

ip = "192.168.10.240"
ip = "192.0.2.1"

t = TestClient(ip, False)

res = t.test_simultaneous(5, 5)

print(res)