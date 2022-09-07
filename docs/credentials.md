# Credentials

A credential refers to either a private key, a certificate or a CA certificate.

Current credentials manager supports hardcoded credentials for all targets and
FLASH-stored credentials for the `nucleo_f429zi` target.

## Configuration options:

See [src/creds/Kconfig](../src/creds/Kconfig) :

| Option                    | Description                            |
| ------------------------- | -------------------------------------- |
| **`CREDENTIALS_MANAGER`** | Globally enable credentials management |
| `CREDS_HARDCODED`         | Enable credentials manager             |
| `CREDS_FLASH`             | Enable creds stored in FLASH           |
| `CREDS_FS`                | Enable creds stored in the filesystem  |

## Credentials ID

The ID identify the credential among the others, following IDs are defined:

If the ID is suffixed with a `_DER`, the credential is expected to be in DER.
If the suffix is not present, it can be either PEM or DER.

### HTTP server:

| ID                                | Description                                   | Value |
| --------------------------------- | --------------------------------------------- | ----- |
| CRED_HTTPS_SERVER_PRIVATE_KEY     | Private key for HTTPS server                  | 16    |
| CRED_HTTPS_SERVER_CERTIFICATE     | Certificate for HTTPS server                  | 17    |
| CRED_HTTPS_SERVER_PRIVATE_KEY_DER | Private key for HTTPS server in DER format    | 18    |
| CRED_HTTPS_SERVER_CERTIFICATE_DER | Certificate for HTTPS server in DER format    | 19    |
| CRED_HTTPS_SERVER_CLIENT_CA       | CA certificate for HTTPS server               | 20    |
| CRED_HTTPS_SERVER_CLIENT_CA_DER   | CA certificate for HTTPS server in DER format | 21    |

### AWS:

| ID                       | Description                               | Value |
| ------------------------ | ----------------------------------------- | ----- |
| CRED_AWS_PRIVATE_KEY     | Private key for AWS                       | 32    |
| CRED_AWS_CERTIFICATE     | Certificate for AWS                       | 33    |
| CRED_AWS_PRIVATE_KEY_DER | Private key for AWS in DER format         | 34    |
| CRED_AWS_CERTIFICATE_DER | Certificate for AWS in DER format         | 35    |
| CRED_AWS_ROOT_CA1        | Root CA certificate for AWS               | 36    |
| CRED_AWS_ROOT_CA3        | Root CA certificate for AWS               | 37    |
| CRED_AWS_ROOT_CA1_DER    | Root CA certificate for AWS in DER format | 38    |
| CRED_AWS_ROOT_CA3_DER    | Root CA certificate for AWS in DER format | 39    |

## Provisioning credentials

You need to provision credentials depending on the configuration.

The file [creds/creds.json](../creds/creds.json) list the credentials files
types and their location. It is used with both hardcoded and flash methods to
provision the credentials.

The credential file content  content is parsed to determine if it's a PEM or DER file, but no more 
validation is done, it is up to you to make sure a `_DER`-labelled credential is
a indeed a DER file.

Note that Mbed TLS requires PEM credentials to have the EOS marker `\0`, it is
automatically added if missing in embeded PEM files.

Before running python scripts, make sure you have activated the virtualenv 
with `source ../.venv/bin/activate`.

### Hardcoded credentials

For hardcoded credentials:
- You'll first need to configure [creds/creds.json](../creds/creds.json).
- Then run the script from the project root: `python3 scripts/creds/hardcoded_creds_xxd.py`
- The file [src/creds/hardcoded_creds_data.c](../src/creds/hardcoded_creds_data.c) will be generated.


### FLASH-stored credentials

For FLASH-stored credentials:
- You'll first need to configure [creds/creds.json](../creds/creds.json).
- Then run the script from the project root: `python3 scripts/creds/flash_creds.py`
- Ouput of the following form is expected:
```
Erasing credentials sector...
Writing credentials ...
Writting 17 from ./creds/https_server/rsa2048/cert.der to slot 0
Writting 16 from ./creds/https_server/rsa2048/key.der to slot 1
Writting 33 from ./creds/AWS/caniot-controller/cert.pem to slot 2
Writting 32 from ./creds/AWS/caniot-controller/key.pem to slot 3
Writting 35 from ./creds/AWS/caniot-controller/cert.der to slot 4
Writting 34 from ./creds/AWS/caniot-controller/key.der to slot 5
Writting 36 from ./creds/AWS/AmazonRootCA1.pem to slot 6
Writting 37 from ./creds/AWS/AmazonRootCA3.pem to slot 7
Writting 38 from ./creds/AWS/AmazonRootCA1.der to slot 8
Writting 39 from ./creds/AWS/AmazonRootCA3.der to slot 9
Credentials sector hexdump saved to ./tmp/openocd_creds_hexdump.txt
```
- When finished, the file [tmp/openocd_creds_hexdump.txt](../tmp/openocd_creds_hexdump.txt) should contain
the hexdump of the credentials sector in your target flash.

If having problems using openocd:
- Make sure you have a recent version of openocd, I'm currently using `Open On-Chip Debugger 0.11.0`.
- MIn order to debug, you can find a Makefile and openocd configuration files in
  the directory [scripts/openocd/](../scripts/openocd/).
- With ST-link V2 of nucleo_f429zi: Maybe try to hold reset button while running the script.

### Filesystem-stored credentials

*Not implemented yet*