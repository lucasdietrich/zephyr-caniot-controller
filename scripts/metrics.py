from pprint import pprint
import requests

ip = "192.168.10.240"
ip = "192.0.2.1"

N = 10
with requests.Session() as s:
    for i in range(N):
        resp = s.get(f"http://{ip}/metrics", headers={
            "Connection": "Keep-alive",
        })
        print(resp, resp.status_code, len(resp.text))
