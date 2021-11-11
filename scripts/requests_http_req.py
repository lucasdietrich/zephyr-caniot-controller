import requests
from requests.api import request
from requests.auth import HTTPBasicAuth
import time

ip = "192.168.10.240"

req = {
        "method": "GET",
        "url": f"http://{ip}/path/2",
        "headers": {
                "Connection": "close"
        },
        "json": {
                "user": "Lucas"
        },
        "auth": HTTPBasicAuth("lucas", "password!"),
        "verify": False
}

a = time.time()
resp = requests.request(**req)
b = time.time()

print(resp, f"{b - a: .3f} s")