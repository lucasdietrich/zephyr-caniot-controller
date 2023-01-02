# Credentials manager

Credentials can be stored either:
1. Hardcoded in the flash (codespace)
2. In the flash, see `certificates_partition` partion in the overlay file.
3. In the filesystem

Yet only harcoded credentials are supported (option 1)

## Description 

Flash sector 17 at offset `0x1e0000` to `0x200000` (128KB) is reserved for the credentials.

16 Credentials can be stored, by blocks of 8KB :

- First 4 bytes of the block are the credential *descriptor* (LE).
  - *id*: HTTP, AWS, ...
  - *format*: PEM/DER
  - *strength*: unimplemented yet
  - *version*: version of the credential, unimplemented yet
- Next 4 bytes are the credential *size* (LE).
- Next 4 bytes contain the *crc32* (IEEE Ethernet) of the credential data (LE).
- Next 4 bytes tells if credential has been *revoked* (different from 0xFFFFFFFF if revoked) (LE).
- The (4096 - 16) bytes contain the credential *data*.

If type, size are inconsistent, or if CRC32 is not correct, the credential is considered as invalid.

*type* should be written first, this reserves the slots and is used to determine the number of valid credentials in the flash.

A block is considered empty if `descr` is 0xFFFFFFFF.

C representation of a credential block in the flash :

```c
struct flash_cred_header
{
	union {
		struct
		{
			cred_id_t id : 8u;
			cred_format_t format : 8u;
			uint32_t strength : 8u;
			uint32_t version : 8u;
		};
		uint32_t descr;
	};
	uint32_t size;
	uint32_t crc32;
	uint32_t revoked;
};
```

## Hardcoded credentials

If `CONFIG_APP_CREDS_HARDCODED` is set, hardcoded credentials file can be generated
with the script [scripts/creds/hardcoded_creds_xxd.py](../../scripts/creds/hardcoded_creds_xxd.py).