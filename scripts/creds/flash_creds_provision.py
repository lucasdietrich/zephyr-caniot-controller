from cmath import log
import struct
import subprocess
from os import path
from dataclasses import dataclass
from typing import List, Optional, Iterable
from enum import IntEnum
import zlib

import logging
logger = logging.getLogger(__name__)


class CredId(IntEnum):
	HTTPS_SERVER_PRIVATE_KEY = 0
	HTTPS_SERVER_CERTIFICATE = 1

	HTTPS_CLIENT_PRIVATE_CERTIFICATE = 2
	HTTPS_CLIENT_CA = 3

	AWS_PRIVATE_KEY = 4
	AWS_CERTIFICATE = 5

	AWS_PRIVATE_KEY_DER = 6
	AWS_CERTIFICATE_DER = 7

	AWS_ROOT_CA1 = 8
	AWS_ROOT_CA3 = 9

	AWS_ROOT_CA1_DER = 10
	AWS_ROOT_CA3_DER = 11

class CredFormat(IntEnum):
    UNKNOWN = 0
    PEM = 1
    DER = 2

class CredStatus(IntEnum):
    UNKNOWN = 0
    UNALLOCATED = 1
    VALID = 2
    REVOKED = 3
    CRC_ERROR = 4
    SIZE_ERROR = 5

@dataclass
class Cred:
    slot: int

    status: CredStatus = CredStatus.UNALLOCATED
    cid: CredId = None
    fmt: CredFormat = None
    strength: int = 0
    version: int = 0
    crc32: hex = 0
    revoked: bool = False
    valid: bool = False
    data: bytes = b''
    size: int = 0

@dataclass
class TargetCredsConfig:
    flash_addr: hex
    offset: hex
    size: hex
    openocd_base_args: List[str]

    cred_block_size: hex = 0x1000

stm32f4_cfg = TargetCredsConfig(
    flash_addr=0x08000000,
    offset=0x1e0000,
    size=0x20000,
    openocd_base_args=[
        "-f", "interface/stlink.cfg",
        "-f", "target/stm32f4x.cfg",
    ]
)


BLANK = 0xFFFFFFFF

class CredsManager():
    def __init__(self, target_cfg: TargetCredsConfig) -> None:
        self.tcfg = target_cfg
        self.rtmp = "./tmp/tmp_openocd_read.bin"
        self.wtmp = "./tmp/tmp_openocd_write.bin"

        self.stderr = open("./tmp/tmp_openocd_stderr.txt", "w")

        self.CONTROL_BLOCK_SIZE = 0x10
        self.CRED_MAX_SIZE = self.tcfg.cred_block_size - self.CONTROL_BLOCK_SIZE
        self.SLOTS_COUNT = self.tcfg.size // self.tcfg.cred_block_size

    def read_sector(self) -> bytes:
        cmd_args = [
            "init",
            "reset halt",
            f"flash read_bank 0 {self.rtmp} 0x{self.tcfg.offset:08x} 0x{self.tcfg.size:x}",
            "reset halt",
            "shutdown"
        ]

        openocd_args = self.tcfg.openocd_base_args + ["-c", "; ".join(cmd_args)]
        openocd_cmd = ["openocd"] + openocd_args

        try:
            output = subprocess.check_output(openocd_cmd, stderr=self.stderr)
            logger.info(f"OpenOCD read_bank 0 0x{self.tcfg.offset:08x} "
                  f"0x{self.tcfg.size:x} -> {self.rtmp}")
            return True
        except Exception as e:
            logger.error("Error: ", e)
            return False

    def erase_sector(self) -> None:
        cmd_args = [
            "init",
            "reset halt",
            f"flash erase_address 0x{self.tcfg.flash_addr + self.tcfg.offset:08x} 0x{self.tcfg.size:x}",
            "reset halt",
            "shutdown"
        ]

        openocd_args = self.tcfg.openocd_base_args + ["-c", "; ".join(cmd_args)]
        openocd_cmd = ["openocd"] + openocd_args

        try:
            output = subprocess.check_output(openocd_cmd, stderr=self.stderr)
            logger.info(f"OpenOCD erase_address "
                  f"0x{self.tcfg.flash_addr + self.tcfg.offset:08x} 0x{self.tcfg.size:x}"
                  " -> OK")
            return True
        except Exception as e:
            logger.error("Error: ", e)
            return False


    def write_slot(self, slot: int, file: str):
        if slot < 0 or slot >= self.SLOTS_COUNT:
            raise Exception(f"Slot {slot} out of range")

        with open(file, "rb") as f:
            data = f.read()
            if len(data) > self.tcfg.cred_block_size:
                raise Exception(f"Invalid file size {len(data)} != {self.tcfg.cred_block_size}")

        cmd_args = [
            "init",
            "reset halt",
            f"flash write_bank 0 {file} 0x{self.tcfg.offset + slot * self.tcfg.cred_block_size:x}",
            "reset halt",
            "shutdown",
        ]

        openocd_args = self.tcfg.openocd_base_args + ["-c", "; ".join(cmd_args)]
        openocd_cmd = ["openocd"] + openocd_args

        try:
            output = subprocess.check_output(openocd_cmd, stderr=self.stderr)
            logger.info(f"OpenOCD write_bank 0 {file} "
                  f"0x{self.tcfg.offset + slot * self.tcfg.cred_block_size:x} -> OK")
            return True
        except Exception as e:
            logger.error("Error: ", e)
            return False
        

    def parse_sector_bin(self, file: str):
        with open(file, "rb") as f:
            for slot in range(self.SLOTS_COUNT):
                cred = Cred(slot=slot)
                
                # Read block
                block = f.read(self.tcfg.cred_block_size)
                if len(block) != self.tcfg.cred_block_size:
                    break

                # Read control block
                cb = block[:0x10]

                descr, size, crc32, revoked = struct.unpack("<LLLL", cb)

                cred.size = size
                cred.crc32 = crc32

                calc_crc32 = zlib.crc32(block[0x10:size + 0x10], 0x0)

                if descr == BLANK:
                    cred.status = CredStatus.UNALLOCATED
                elif revoked != BLANK:
                    logger.info("Credential revoked")
                    cred.revoked = True
                    cred.status = CredStatus.REVOKED
                elif size > self.CRED_MAX_SIZE or size == 0:
                    logger.info("Size is invalid")
                    cred.status = CredStatus.SIZE_ERROR
                elif crc32 != calc_crc32:
                    logger.info(f"CRC32 mismatch: {crc32:08x} != {calc_crc32:08x}")
                    cred.status = CredStatus.CRC_ERROR
                else:
                    cred.status = CredStatus.VALID
                    cred.data = block[0x10:0x10 + size]
                    
                    cred.fmt = CredFormat((descr & 0xFF00) >> 8)
                    cred.cid = CredId(descr & 0xFF)
                    cred.strength = (descr & 0xFF0000) >> 16
                    cred.version = (descr & 0xFF000000) >> 24
                    cred.valid = True

                yield cred

    def find_first_free_slot(self, creds: Iterable[Cred]) -> int:
        for cred in creds:
            if cred.status == CredStatus.UNALLOCATED:
                return cred.slot

        return -1

    def _prepare_credential_bin(self, file: str, cid: CredId, 
                                format: CredFormat = CredFormat.UNKNOWN, 
                                strength: int = 0, version: int = 0) -> None:
        with open(file, "rb") as fr:
            data = fr.read()
            clen = len(data)

            if clen > self.CRED_MAX_SIZE:
                raise Exception(f"File {file} too large ({clen} > {self.CRED_MAX_SIZE})")

            # calculate ieee crc32 of the data
            crc32 = zlib.crc32(data, 0x0)
            logger.info(f"Calculated CRC32={crc32:08x}")

            # Prepare control block
            descr = int(cid) | (int(format) << 8) | (strength << 16) | (version << 24)
            cb = struct.pack("<LLLL", descr, clen, crc32, BLANK)

            with open(self.wtmp, "bw+") as fw:
                fw.write(cb)
                fw.write(data)

    def write_credential(self, slot: int, file: str, cid: CredId,
                         format: CredFormat = CredFormat.UNKNOWN,
                         strength: int = 0, version: int = 0) -> None:
        self._prepare_credential_bin(file, cid, format, strength, version)
        return self.write_slot(slot, self.wtmp)

            

if __name__ == "__main__":
    cm = CredsManager(stm32f4_cfg)

    cm.erase_sector()

    cm.read_sector()
    subprocess.call(["hexdump", "-C", cm.rtmp], stdout=open("./tmp/openocd_creds_hexdump_start.txt", "w"))
    print("Hexdump saved to ./tmp/openocd_creds_hexdump_start.txt")

    changes = False

    for cred in cm.parse_sector_bin(cm.rtmp):
        if cred.slot == 0 and not cred.valid:
            print(f"HTTPS_SERVER_CERTIFICATE {cred}")
            cm._prepare_credential_bin("./creds/https_server/rsa2048/cert.der",
                                       CredId.HTTPS_SERVER_CERTIFICATE,
                                       CredFormat.DER)
            cm.write_slot(0, cm.wtmp)
            changes = True
        elif cred.slot == 1 and not cred.valid:
            print(f"HTTPS_SERVER_PRIVATE_KEY {cred}")
            cm._prepare_credential_bin("./creds/https_server/rsa2048/key.der",
                                       CredId.HTTPS_SERVER_PRIVATE_KEY,
                                       CredFormat.DER)
            cm.write_slot(1, cm.wtmp)
            changes = True
        elif cred.slot == 2 and not cred.valid:
            print(f"AWS_ROOT_CA1 {cred}")
            cm._prepare_credential_bin("./creds/AWS/AmazonRootCA1.pem",
                                       CredId.AWS_ROOT_CA1,
                                       CredFormat.PEM)
            cm.write_slot(2, cm.wtmp)
            changes = True
        elif cred.slot == 3 and not cred.valid:
            print(f"AWS_ROOT_CA3 {cred}")
            cm._prepare_credential_bin("./creds/AWS/AmazonRootCA3.pem",
                                       CredId.AWS_ROOT_CA3,
                                       CredFormat.PEM)
            cm.write_slot(3, cm.wtmp)
            changes = True
        elif cred.slot == 4 and not cred.valid:
            print(f"AWS_CERTIFICATE {cred}")
            cm._prepare_credential_bin("./creds/AWS/caniot-controller/9e99599b7b81772d43869b44f1ff4495be6e5c448b020c0a29a87e133c145c0d-certificate.pem.crt",
                                       CredId.AWS_CERTIFICATE,
                                       CredFormat.PEM)
            cm.write_slot(4, cm.wtmp)
            changes = True
        elif cred.slot == 5 and not cred.valid:
            print(f"AWS_PRIVATE_KEY {cred}")
            cm._prepare_credential_bin("./creds/AWS/caniot-controller/9e99599b7b81772d43869b44f1ff4495be6e5c448b020c0a29a87e133c145c0d-private.pem.key",
                                       CredId.AWS_PRIVATE_KEY,
                                       CredFormat.PEM)
            cm.write_slot(5, cm.wtmp)
            changes = True
        elif cred.slot == 6 and not cred.valid:
            print(f"AWS_CERTIFICATE_DER {cred}")
            cm._prepare_credential_bin("./creds/AWS/caniot-controller/cert.der",
                                       CredId.AWS_CERTIFICATE_DER,
                                       CredFormat.DER)
            cm.write_slot(6, cm.wtmp)
            changes = True
        elif cred.slot == 7 and not cred.valid:
            print(f"AWS_PRIVATE_KEY_DER {cred}")
            cm._prepare_credential_bin("./creds/AWS/caniot-controller/key.der",
                                       CredId.AWS_PRIVATE_KEY_DER,
                                       CredFormat.DER)
            cm.write_slot(7, cm.wtmp)
            changes = True
        elif cred.slot == 8 and not cred.valid:
            print(f"AWS_ROOT_CA1_DER {cred}")
            cm._prepare_credential_bin("./creds/AWS/AmazonRootCA1.der",
                                       CredId.AWS_ROOT_CA1_DER,
                                       CredFormat.DER)
            cm.write_slot(8, cm.wtmp)
            changes = True
        elif cred.slot == 9 and not cred.valid:
            print(f"AWS_ROOT_CA3_DER {cred}")
            cm._prepare_credential_bin("./creds/AWS/AmazonRootCA3.der",
                                       CredId.AWS_ROOT_CA3_DER,
                                       CredFormat.DER)
            cm.write_slot(9, cm.wtmp)
            changes = True

        print(cred)

    if changes:
        cm.read_sector()
        subprocess.call(["hexdump", "-C", cm.rtmp], stdout=open("./tmp/openocd_creds_hexdump.txt", "w"))
        print("Hexdump saved to ./tmp/openocd_creds_hexdump.txt")