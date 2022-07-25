#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from ast import For
import requests
from requests.api import request
from requests.auth import HTTPBasicAuth
import time
from colorama import Fore, Style, Back

ip = "192.168.10.240"
proto = "http"

req = {
    "method": "GET",
    "url": f"{proto}://{ip}/devices/caniot/24/ll/3/query_telemetry",
    "headers": {
        "Connection": "close"
    },
    "json": {

    },
    "auth": HTTPBasicAuth("lucas", "password!"),
    "verify": False
}

with requests.session() as sess:

    timeouts = 0
    total = 0

    while True:
        a = time.perf_counter()
        resp = sess.request(**req)
        b = time.perf_counter()
        delta = b - a

        if delta > 1.0:
            timeouts += 1

        total += 1

        msg = f"{timeouts}/{total} Time {delta:.3f} "
        if delta > 1.0:
            print(Fore.RED + msg + Style.RESET_ALL, resp)
        elif delta > 0.2:
            print(Fore.YELLOW + msg + Style.RESET_ALL, resp)
        else:
            print(msg, resp)

        # time.sleep(0.1)
