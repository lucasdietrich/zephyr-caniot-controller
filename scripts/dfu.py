#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from pprint import pprint
from caniot.controller import Controller

ip = "192.0.2.1"
ip = "192.168.10.240"

ctrl = Controller(ip, False)

resp = ctrl.dfu_status()

print(resp, resp.status_code, len(resp.text))
print(resp.text)

resp, delta = ctrl.dfu_upload("tmp/zephyr.signed.bin")
print(f"Uploaded in {delta:0.3f} s", resp, resp.status_code, len(resp.text))
print(resp.text)
