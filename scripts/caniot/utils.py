from enum import IntEnum
import struct
from typing import Iterable

class Method(IntEnum):
    GET = 0
    POST = 1
    PUT = 2
    DELETE = 3


def BytesToU32(b: bytes) -> int:
    return struct.unpack("<L", b)[0]

def data_gen_zeros(size: int) -> bytes:
    return bytes(size)

def MakeChunks(data: bytes, size: int) -> Iterable:
    for i in range(0, len(data), size):
        yield data[i:i+size]
