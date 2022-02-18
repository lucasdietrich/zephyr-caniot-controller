import requests
from requests.api import request
from requests.auth import HTTPBasicAuth
import time

ip = "192.168.10.240"
proto = "http"

req = {
        "method": "GET",
        "url": f"{proto}://{ip}/info",
        "headers": {
                # "Connection": "close"
        },
        "json": {
                "user": "L"*2000
        },
        "auth": HTTPBasicAuth("lucas", "password!"),
        "verify": False
}
a = time.time()
with requests.session() as sess:
        resp = sess.request(**req)
        print(resp)
        # resp = sess.request(**req)
        # print(resp)

b = time.time()

print(f"{b - a: .3f} s")
