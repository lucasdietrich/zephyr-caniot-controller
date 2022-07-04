from lib2to3.pgen2.token import OP
import requests
from requests.auth import HTTPBasicAuth
from typing import List, Iterable, Tuple, Dict, Union, Any, Optional, Callable
from dataclasses import dataclass
from enum import IntEnum
import struct
import pprint

from url import URL

class Method(IntEnum):
    GET = 0
    POST = 1
    PUT = 2
    DELETE = 3

class API(IntEnum):
    WriteAttribute = 0
    ReadAttribute = 1
    Command = 2
    RequestTelemetry = 3

urls = {
    API.WriteAttribute: (Method.GET, "devices/caniot/{did}/attributes/{attr:x}"),
    API.ReadAttribute: (Method.PUT, "devices/caniot/{did}/attributes/{attr:x}"),
    API.Command: (Method.POST, "devices/caniot/{did}/endpoints/{ep}/command"),
    API.RequestTelemetry: (Method.GET, "devices/caniot/{did}/endpoints/{ep}/telemetry"),
}

def BytesToU32(b: bytes) -> int:
    return struct.unpack("<L", b)[0]

class DeviceContext:
    def __init__(self, did: int) -> None:
        self.did = did

class Controller:
    def __init__(self, host: str, port: int, secure: bool = False) -> None:
        self.host = host
        self.port = port
        self.secure = secure

        self.timeout = 5.0

        self.url = URL(f"{self.host}:{self.port}", secure=self.secure)

        self.headers = {
            "Timeout-ms": str(int(self.timeout * 1000)),
        }

    def _request(self, api: API, json: Optional[Dict], args: Dict = None, headers: Dict = None):
        method, path = urls[api]

        if args is None:
            args = dict()

        if headers is None:
            headers = dict()

        return requests.request(
            method=method.name,
            url=(self.url + path).project(**args),
            json=json,
            timeout=self.get_req_timeout(),
            headers=self.headers | headers
        )

    def get_req_timeout(self) -> float:
        return self.timeout + 2.0

    def write_attribute(self, did: int, attr: int, value: Union[int, bytes]):

        data = {
            "value": BytesToU32(value)
        }

        args = {
            "did": did,
            "attr": attr
        }

        return self._request(API.WriteAttribute, data, args)

    def read_attribute(self, did: int, attr: int) -> requests.Response:
        return self._request(API.ReadAttribute, None, {
            "did": did,
            "attr": attr
        })

    def request_telemetry(self, did: int, ep: int) -> requests.Response:
        return self._request(API.RequestTelemetry, None, {
            "did": did,
            "ep": ep
        })
    
    def command(self, did: int, ep: int, coc1: int = 0, coc2: int = 0, crl1: int = 0, crl2: int = 0) -> requests.Response:
        XPS = ["none", "set_on", "set_off", "toggle", "reset", "pulse_on", "pulse_off", "pulse_cancel"]
        
        return self._request(API.Command, json={
            "coc1": XPS[coc1],
            "coc2": XPS[coc2],
            "crl1": XPS[crl1],
            "crl2": XPS[crl2]
        }, args={
            "did": did,
            "ep": ep,
        })

    def __enter__(self, did: int) -> DeviceContext:
        return DeviceContext(did)


if __name__ == "__main__":
    ctrl = Controller("192.168.10.240", 80, False)

    did = 0x20
    ep = 3
    n = 1

    res = ctrl.command(did, ep)

    print(res, res.status_code)
    pprint.pprint(res.json())

    # for i in range(n):
    #     res = ctrl.request_telemetry(did, ep)

    #     print(res)
    #     if res is not None:
    #         print(res.status_code)

    #         try:
    #             pprint.pprint(res.json())
    #         except:
    #             print(res.text)