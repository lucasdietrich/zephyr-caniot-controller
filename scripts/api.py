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

    def write_attribute(self, did: int, attr: int, value: Union[int, bytes]):
        method, path = urls[API.WriteAttribute]

        return requests.request(
            method=method.name,
            url=(self.url + path).project(did=did, attr=attr),
            data=value,
            timeout=self.timeout,
            json={
                "value": BytesToU32(value)
            }
        )

    def read_attribute(self, did: int, attr: int) -> requests.Response:
        method, path = urls[API.ReadAttribute]

        return requests.request(
            method=method.name,
            url=(self.url + path).project(did=did, attr=attr),
            timeout=self.timeout
        )

    def request_telemetry(self, did: int, ep: int) -> requests.Response:
        method, path = urls[API.RequestTelemetry]

        return requests.request(
            method=method.name,
            url=(self.url + path).project(did=did, ep=ep),
            timeout=self.timeout
        )

    def __enter__(self, did: int) -> DeviceContext:
        return DeviceContext(did)


if __name__ == "__main__":
    ctrl = Controller("192.168.10.240", 80, False)

    did = 0x20
    ep = 3

    res = ctrl.request_telemetry(did, ep)

    print(res)
    if res is not None:
        print(res.status_code)

        try:
            pprint.pprint(res.json())
        except:
            print(res.text)