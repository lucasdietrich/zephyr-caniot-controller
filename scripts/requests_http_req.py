#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

import requests
from requests.api import request
from requests.auth import HTTPBasicAuth
import time

ip = "192.168.10.240"
ip = "192.0.2.1"
proto = "https"

req = {
        "method": "GET",
        "url": f"{proto}://{ip}/info",
        "headers": {
                "Connection": "close"
        },
        "json": {
                "user": "L"*2000
        },
        "auth": HTTPBasicAuth("lucas", "password!"),
        "verify": False
}
a = time.time()
with requests.session() as sess:
    for i in range(10):
        resp = sess.request(**req)
        print(resp.status_code, resp.text[:50])

b = time.time()

print(f"{b - a: .3f} s")
