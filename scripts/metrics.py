from pprint import pprint
import requests

ip = "192.0.2.1"
ip = "192.168.10.240"

N = 10**100
with requests.Session() as s:
    for i in range(N):
        resp = s.get(f"http://{ip}/metrics", headers={
            "Connection": "Keep-alive",
        })
        print(i, resp, resp.status_code, len(resp.text))
