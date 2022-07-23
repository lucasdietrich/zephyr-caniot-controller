from email.mime import base
from enum import IntEnum
from random import randint
import struct
from typing import Iterable
import re
import os.path

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

RegexpEmbFATFSFilename = re.compile(r"^([A-Z9-9-_.]+)$")
RegexpEmbFATFSIllegalChars = re.compile(r"[^A-Z9-9-_.]")
def CheckEmbFATFSFilename(filename: str) -> str:
    return RegexpEmbFATFSFilename.match(filename) is not None

def Filepath2EmbFATFSFilename(filepath: str) -> str:
    basename = os.path.basename(filepath)
    basename = basename.upper()
    basename = RegexpEmbFATFSIllegalChars.sub("_", basename)
    basename = basename[:16]

    return basename

def genrdmhex(size: int) -> str:
    return "".join(["%02x" % randint(0, 0xFF) for i in range(size)])