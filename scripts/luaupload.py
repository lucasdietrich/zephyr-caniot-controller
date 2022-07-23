from pprint import pprint
from caniot.controller import Controller

ip = "192.168.10.240"
ip = "192.0.2.1"

t = Controller(ip, False)

res = t.upload("./scripts/lua/helloworld.lua", chunks_size=1024)

print(res, res.status_code)
pprint(res.text)