from enum import IntEnum, auto

class CredFormat(IntEnum):
    UNKNOWN = 0
    PEM = 1
    DER = 2
    
class CredId(IntEnum):
	CRED_HTTPS_SERVER_PRIVATE_KEY = 0x10
	CRED_HTTPS_SERVER_CERTIFICATE = auto()
	CRED_HTTPS_SERVER_PRIVATE_KEY_DER = auto()
	CRED_HTTPS_SERVER_CERTIFICATE_DER = auto()
	CRED_HTTPS_SERVER_CLIENT_CA = auto()
	CRED_HTTPS_SERVER_CLIENT_CA_DER = auto()

	CRED_AWS_PRIVATE_KEY = 0x20
	CRED_AWS_CERTIFICATE = auto()
	CRED_AWS_PRIVATE_KEY_DER = auto()
	CRED_AWS_CERTIFICATE_DER = auto()
	CRED_AWS_ROOT_CA1 = auto()
	CRED_AWS_ROOT_CA3 = auto()
	CRED_AWS_ROOT_CA1_DER = auto()
	CRED_AWS_ROOT_CA3_DER = auto()

def is_pem(path: str):
    with open(path, "r") as f:
        data = f.read()
    
    return data.startswith("-----BEGIN")

def is_der(path: str):
    with open(path, "rb") as f:
        data = f.read()
    
    return not data.startswith("-----BEGIN") and data.startswith(b"\x30")

def get_format(path: str):
    """
    Return the format of the file at path.
    """
    with open(path, "rb") as f:
        data = f.read()
    
    if data.startswith(b"-----BEGIN"):
        return CredFormat.PEM
    elif data.startswith(b"\x30"):
        return CredFormat.DER
    else:
        return CredFormat.UNKNOWN

def parse_creds_json(loc: str = "./creds/creds.json"):
    import json
    with open(loc, "r") as f:
        creds = json.load(f)

    # transform each key to CredId
    creds = {CredId[k]: v for k, v in creds.items()}
    
    return creds

def to_hex_c(data: bytes, linesep: str = "\t"):
    # by group of 16 bytes
    out = linesep
    for i, b in enumerate(data):
        out += f"0x{b:02x}, "
        if i % 16 == 15:
            out += "\n" + linesep
    return out

def to_hex_c_array(data: bytes, name: str):
    out = f"static const unsigned char {name}[] = {{\n"
    out += to_hex_c(data)
    out += f"\n}};\n"
    out += f"static const unsigned int {name}_len = {len(data)}u;\n"
    return out

if __name__ == "__main__":
    print("OK" if CredId.CRED_AWS_ROOT_CA3_DER.value == 39 else "NOK")
    print("OK" if CredId.CRED_AWS_CERTIFICATE_DER == 35 else "NOK")
    print(to_hex_c_array(b"oifezhoazfiheafoihzhoifezhfoiazeohifazeoihafze", "myvar"))