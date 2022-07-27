#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

import re
import requests
from hexdump import hexdump
from requests_toolbelt import MultipartEncoder
from pprint import pprint
import random
from typing import Callable, Iterator, List, Dict, Optional, Union

from caniot.controller import Controller, Method, RouteType
from caniot.utils import data_gen_zeros

ChunksGeneratorType = Iterator[bytes]

class TestClient(Controller):

    def test_big_data(self, size = 32768) -> requests.Response:
        data = data_gen_zeros(size)
        req = self.default_req | {
            "method": Method.POST.name,
                "url": self.url + "test/messaging",
                "timeout": self.get_req_timeout(),
                "headers": self.default_headers,
                "data": data
            }
        return requests.request(**req)

    def test_simultaneous(self, simultaneous: int = 5, count: int = 5):
        sessions = [requests.Session() for _ in range(simultaneous)]
        conn_refused = 0

        for reqid in range(count):
            for clientid, s in enumerate(sessions):
                req = self.default_req | {
                    "method": Method.GET.name,
                    "url": self.url + "info",
                    "json": { "clientid": clientid, "reqid": reqid},
                    "timeout": self.get_req_timeout(),
                    "headers": self.default_headers
                }
                try:
                    resp = s.request(**req)
                except requests.exceptions.ConnectionError:
                    conn_refused += 1
                    sessions.remove(s)

                print(resp.status_code, resp.text[:50] + " ...")

            random.shuffle(sessions)

        print("Connection refused:", conn_refused)
            

    def test_session(self, count: int = 3):
        with requests.sessions.Session() as s:
            for i in range(count):
                req = self.default_req | {
                    "method": Method.GET.name,
                    "url": self.url + "info",
                    "json": { "cnt": i},
                    "timeout": self.get_req_timeout(),
                    "headers": self.default_headers
                }

                resp = s.request(**req)

                print(resp.status_code, resp.text[:50])

    def test_stream(self, chunks_generator: ChunksGeneratorType) -> requests.Response:
        req = self.default_req | {
            "method": Method.POST.name,
            "url": self.url + "test/streaming",
            "data": chunks_generator,
            "headers": {
                "Content-Type": "application/octet-stream"
            },
        }

        return requests.request(**req)

    def test_multipart(self, size: int = 128768,
                       chunk_length_default: int = 512,
                       chunks_lengths: list = None) -> requests.Response:
        if chunks_lengths is None:
            chunks_lengths = list()

        def get_chunk_length(chunk_id):
            if chunk_id < len(chunks_lengths):
                return chunks_lengths[chunk_id]
            else:
                return chunk_length_default

        multipart = MultipartEncoder(
            fields={
                "KEY1": "VALUE___AA",
                "myfile.txt": ("myfile.bin", data_gen_zeros(size)),
            },
            boundary="----WebKitFormBoundary7MA4YWxkTrZu0gW",
        )

        def file_chunks_generator():
            i = 0
            while True:
                chunk = multipart.read(get_chunk_length(i))
                if chunk:
                    i += 1
                    yield chunk
                else:
                    break

        req = self.default_req | {
            "method": Method.POST.name,
            "url": self.url + "test/streaming",
            "data": file_chunks_generator(),
            "headers": {
                "Content-Type": multipart.content_type,
                # "Content-Length": multipart.len,
            },
        }

        return requests.request(**req)

    def test_route_args(self, a: int = 1, b: int = 2, c: int = 3) -> requests.Response:
        req = self.default_req | {
            "method": Method.GET.name,
            "url": (self.url + "test/route_args/{a}/{b}/{c}").project(a=a, b=b, c=c),
            "headers": self.default_headers
        }
        return requests.request(**req)

    def test_headers(self, headers: dict = None):
        if headers is None:
            headers = {}

        req = self.default_req | {
            "method": Method.GET.name,
            "url": (self.url + "test/headers"),
            "headers": self.default_headers | headers
        }
        return requests.request(**req)
