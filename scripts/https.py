from pprint import pprint
from caniot.controller import Controller
import requests

ip = "192.168.10.240"
ip = "192.0.2.1"

# ctrl = Controller(ip, True,
#                   cert="creds/https_server/rsa2048/cert.pem",
#                   key="creds/https_server/rsa2048/key.pem",
#                   verify="creds/https_server/rsa2048/ca.cert.pem")

# ctrl = Controller(ip, True,
#                   cert="creds/https_server/rsa2048/cert.pem",
#                   key="creds/https_server/rsa2048/key.pem")

requests.get(f"https://{ip}/info",
             cert=("creds/https_server/rsa2048/cert.pem",
                   "creds/https_server/rsa2048/key.pem")
             )

print(resp, resp.status_code)
pprint(resp.json())
