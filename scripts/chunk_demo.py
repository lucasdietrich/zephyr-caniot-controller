#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from tkinter import Y
import requests
from requests_toolbelt import MultipartEncoder
from pprint import pprint
import hexdump

ip = "192.0.2.1"

chunks_length = [1024, 10, 234, 342]

def gen_data(offset, size):
    return b"".join(f"{x:04X}".encode('utf-8') for x in range(offset, offset + size, 4))

def gen_data2(offset, size):
    return bytes(size)

def chunks_generator(lengths):
    pos = 0
    for chunk, length in enumerate(lengths):
        yield gen_data(pos, length)
        pos += length

def get_chunk_length(chunk_id):
    if chunk_id < len(chunks_length):
        return chunks_length[chunk_id]
    else:
        return 512

def get_file(size = 8192):
    return gen_data2(0, size)

# "application/octet-stream"

m = MultipartEncoder(
    fields={
        "KEY1": "VALUE___AA",
        "file1": ("myfile.bin", get_file(1000000)),
    },
    boundary="----WebKitFormBoundary7MA4YWxkTrZu0gW",
)

def file_chunks_generator():
    i = 0
    while True:
        chunk = m.read(get_chunk_length(i))
        if chunk:
            i += 1
            yield chunk
        else:
            break



response = requests.post(f"http://{ip}/test/streaming",
                         data=file_chunks_generator(),
                         headers={"Content-Type": m.content_type})
print(response.status_code)

try:
    pprint(response.json())
except:
    print(response.text)