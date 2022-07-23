from curses.ascii import BS
from email.mime import base
from enum import IntEnum
from random import randint
import struct
from typing import Iterable
import re
import os.path

from regex import B

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

RegexpEmbFATFSFilepath = re.compile(r"^(([A-Z0-9]{1,8}/)?[A-Z0-9]{1,8}(\.[A-Z0-9]{,3})?)$")
RegexpEmbFATFSFilepathLFN = re.compile(r"^(([A-Z0-9]/)?[A-Z0-9]*(\.[A-Z0-9]{,3})?)$")
RegexpEmbFATFSIllegalChars = re.compile(r"[^A-Z0-9.]")
def CheckEmbFATFSFilepath(filename: str, lfn: bool) -> bool:
    if lfn:
        return RegexpEmbFATFSFilepathLFN.match(filename) is not None and len(filename) <= 255
    else:
        return RegexpEmbFATFSFilepath.match(filename) is not None

def Filepath2EmbFATFSFilepath(filepath: str, lfn: bool = False) -> str:
    directory = os.path.dirname(filepath).split("/")[-1]

    basename = os.path.basename(filepath)
    basename = basename.upper()
    basename = RegexpEmbFATFSIllegalChars.sub("", basename)

    # "." should appear only once
    dotcount = basename.count(".")
    if dotcount > 1:
        basename = basename.replace(".", "", dotcount - 1)

        if basename[-1] == ".":
            basename = basename[:-1]

    if not lfn:
        name, ext = basename.split(".")
        name = name[:8]
        if ext:
            ext = ext[:3]
            basename = f"{name}.{ext}"
        else:
            basename = name

        directory = directory[:8]

    if directory:
        return f"{directory}/{basename}"
    else:
        return basename

def genrdmhex(size: int) -> str:
    return "".join(["%02x" % randint(0, 0xFF) for i in range(size)])