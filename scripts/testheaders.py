from pprint import pprint
from caniot.test import  TestClient, data_gen_zeros, ChunksGeneratorType
from caniot.utils import genrdmhex
import requests

ip = "192.168.10.240"
ip = "192.0.2.1"

t = TestClient(ip, False)

res = t.test_headers({
    "App-Upload-Filename": "HELLOWORLD.LUA",
    "Authorization": genrdmhex(46),
    "App-Test-Header1": genrdmhex(20),
    "App-Test-Header2": genrdmhex(20),
    "App-Test-Header3": genrdmhex(40),
    "App-Test-Header4": genrdmhex(20),
})

print(res)

try:
    pprint(res.json())
except requests.exceptions.JSONDecodeError:
    pprint(res.text)