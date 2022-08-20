#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

import requests
from requests.api import request
from requests.auth import HTTPBasicAuth
import time

ip = "192.0.2.1"
ip = "192.168.10.240"
proto = "http"

N = 1

req = {
        "method": "GET",
        "url": f"{proto}://{ip}/test/payload",
        "headers": {
                "Connection": "keep-alive",
        },
}
a = time.time()
with requests.session() as sess:
    for i in range(N):
        resp = sess.request(**req, timeout=10000.0)
        print(resp.status_code, len(resp.text), resp.text[:50])
        with open("./tmp_payload.txt", "w") as f:
            f.write(resp.text)

b = time.time()

print(f"{b - a: .3f} s")
