#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

import ssl
from fileinput import filename
import requests
from requests.auth import HTTPBasicAuth
from typing import List, Iterable, Tuple, Dict, Union, Any, Optional, Callable
from dataclasses import dataclass
from enum import IntEnum, auto
import struct
import pprint
import time
import os.path

import logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

from .utils import Method, BytesToU32, MakeChunks,\
    CheckEmbFATFSFilepath, RegexpEmbFATFSFilepath, Filepath2EmbFATFSFilepath
from caniot.url import URL

class API(IntEnum):
    Info = auto()
    WriteAttribute = auto()
    ReadAttribute = auto()
    Command = auto()
    RequestTelemetry = auto()
    Files = auto()
    FileDownload = auto()
    ListLuaScripts = auto()
    ListLuaScriptsDetailled = auto()
    ExecuteLua = auto()
    BLCCommand = auto()
    CAN = auto()
    DFUStatus = auto()
    DFUUpload = auto()


urls = {
    API.Info: (Method.GET, "info"),

    API.WriteAttribute: (Method.PUT, "devices/caniot/{did}/attribute/{attr:x}"),
    API.ReadAttribute: (Method.GET, "devices/caniot/{did}/attribute/{attr:x}"),
    API.BLCCommand: (Method.POST, "devices/caniot/{did}/endpoint/blc/command"),
    API.Command: (Method.POST, "devices/caniot/{did}/endpoint/{ep}/command"),
    API.RequestTelemetry: (Method.GET, "devices/caniot/{did}/endpoint/{ep}/telemetry"),

    API.Files: (Method.POST, "files"),
    API.ListLuaScripts: (Method.GET, "files/lua/simple"),
    API.ListLuaScriptsDetailled: (Method.GET, "files/lua"),
    API.ExecuteLua: (Method.POST, "lua/execute"),
    API.FileDownload: (Method.GET, "files/{filepath}"),
    API.CAN: (Method.POST, "if/can/{id:X}"),
    API.DFUStatus: (Method.GET, "dfu"),
    API.DFUUpload: (Method.POST, "dfu"),
}

RouteType = Tuple[Method, str]

class DeviceContext:
    def __init__(self, did: int) -> None:
        self.did = did

class Controller:

    def __init__(self, host: str = "192.0.2.1", secure: bool = False, 
                 cert: str = None, key: str = None, verify: str = None) -> None:
        self.host = host
        self.port = 443 if secure else 80
        self.secure = secure
        self.cert = cert
        self.key = key
        self.verify = verify

        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS)
        ssl_context.verify_mode = ssl.CERT_REQUIRED
        ssl_context.check_hostname = False
        ssl_context.load_default_certs()

        self.timeout = 5.0

        self.url = URL(f"{self.host}:{self.port}", secure=self.secure)

        self.default_headers = {
            "Timeout-ms": str(int(self.timeout * 1000)),
        }

        self.default_req = {
            "verify": False,
        }

    def todo__(self) -> requests.Response:
        return self._request_json(API.BLCCommand, None, {
            "did": 0,
            "ep": 0,
        })

    def _request_json(self, api: API,
                      json: Optional[Dict] = None,
                      args: Dict = None,
                      headers: Dict = None):
        method, path = urls[api]

        if args is None:
            args = dict()

        if headers is None:
            headers = dict()

        req = self.default_req | {
            "method": method.name,
            "url": (self.url + path).project(**args),
            "json": json,
            "headers": self.default_headers | headers,
            "timeout": self.get_req_timeout(),
            # "cert": (self.cert, self.key),
            "verify": self.verify,
        }

        t0 = time.perf_counter()
        resp = requests.request(**req)
        t1 = time.perf_counter()
        logger.info(" [ {:0.6f} s ] {} {} {}".format(t1 - t0, req["method"], req["url"], resp.status_code))
        return resp

    def get_info(self) -> requests.Response:
        return self._request_json(API.Info)

    def get_req_timeout(self) -> float:
        return self.timeout + 2.0
    
    def config_reset(self, did: int):
        self.can((did << 3) | (3 << 9), [
            0, 0, 0, 0, 0, 0, 0, 1 << 5
        ])

    def blc_command(self, did: int, coc1: int = 0, coc2: int = 0, crl1: int = 0, crl2: int = 0) -> requests.Response:
        XPS = ["none", "set_on", "set_off", "toggle", "reset", "pulse_on", "pulse_off", "pulse_cancel"]
        
        return self._request_json(API.BLCCommand, json={
            "coc1": XPS[coc1],
            "coc2": XPS[coc2],
            "crl1": XPS[crl1],
            "crl2": XPS[crl2]
        }, args={
            "did": did,
        })



    def list_lua_scripts(self, detailled: bool = True) -> requests.Response:
        if detailled:
            return self._request_json(API.ListLuaScriptsDetailled)
        else:
            return self._request_json(API.ListLuaScripts)

    def upload(self, file: str, 
               chunks_size: int = 1024,
               filepath: str = None,
               lfn: bool = True) -> requests.Response:
        """
        LFN length Is only checked against 255 and not actual length limited (if configured) 
        """
        method, path = urls[API.Files]

        # validation and transformation on filename
        if filepath is None:
            filepath = file

        if not CheckEmbFATFSFilepath(filepath, lfn):
            print(f"Invalid filename {filepath} "
                  f"regexp pattern is : {RegexpEmbFATFSFilepath.pattern}")

        filepath = Filepath2EmbFATFSFilepath(filepath, lfn)

        binary = open(file, "rb").read()
        if chunks_size:
            binary = MakeChunks(binary, chunks_size)

        req = self.default_req | {
            "method": method.name,
            "url": (self.url + path).project(**{}),
            "data": binary,
            "headers": self.default_headers | {
                "App-Upload-Filepath": filepath,
            },
            "timeout": self.get_req_timeout(),
        }

        return requests.request(**req)
    
    def download(self, filepath: str):
        method, path = urls[API.FileDownload]

        req = self.default_req | {
            "method": method.name,
            "url": (self.url + path).project(filepath=filepath),
        }

        return requests.request(**req)

    def run_script(self, path: str):
        return self._request_json(API.ExecuteLua, None, None, {
            "App-Script-Filename": path,
        })

    def dfu_status(self) -> requests.Response:
        return self._request_json(API.DFUStatus)

    def dfu_upload(self, image: str, chunk_size: int = 4096) -> requests.Response:
        method, path = urls[API.DFUUpload]

        # get file size
        size = os.path.getsize(image)

        print(f"Uploading image {image} of size {size} to {self.host}")

        def image_reader(image: str, chunk_size: int):
            with open(image, "rb") as fp:
                while True:
                    chunk = fp.read(chunk_size)

                    if not chunk:
                        return

                    yield chunk

        req = self.default_req | {
            "method": method.name,
            "url": (self.url + path).project(),
            "data": image_reader(image, chunk_size),
            "timeout": self.get_req_timeout(),
        }

        start = time.perf_counter()
        resp = requests.request(**req)
        end = time.perf_counter()
        
        return resp, end - start

    def __enter__(self, did: int) -> DeviceContext:
        return DeviceContext(did)

if __name__ == "__main__":
    # "192.0.2.2"
    ctrl = Controller("192.0.2.1", 80, False)

