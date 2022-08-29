# Credentials manager

Credentials can be stored either:
1. Hardcoded in the flash (codespace)
2. In the flash, see `certificates_partition` partion in the overlay file.
3. In the filesystem

Yet only harcoded credentials are supported (option 1)

## Description 

Flash sector 17 at offset `0x1e0000` to `0x200000` (128KB) is reserved for the credentials.

16 Credentials can be stored, by blocks of 8KB :

- First 4 bytes of the block are the credential *type* (LE)
- Next 4 bytes are the credential *size* (LE).
- Next 4 bytes contain the *CRC32* (IEEE Ethernet) of the credential data (LE).
- Next 4 bytes are *unused* yet.
- The (8192 - 12) bytes contain the credential *data*.

If type, size are inconsistent, or if CRC32 is not correct, the credential is considered as invalid.

*type* should be written first, this reserves the slots and is used to determine the number of valid credentials in the flash.

A block is considered empty if type is 0xFFFFFFFF.

C representation of a credential block in the flash :

```c
struct flash_cred {
    uint32_t type;
    uint32_t size;
    uint32_t crc32;
    uint32_t unused;
    uint8_t data[8192u - 16u];
};
```

## TODO:
- How to upload a binary into flash at a specific address using OpenOCD?