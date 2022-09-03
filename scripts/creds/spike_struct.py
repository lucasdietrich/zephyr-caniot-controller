import struct 

cred_id = 3
size = 1380
revoked = 0xFFFFFFFF
crc32 = 0x12345678

z = struct.pack("<LLLL", cred_id, size, revoked, crc32)

print(z)