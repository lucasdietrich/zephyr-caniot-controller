import subprocess
import os.path

from credentials import CredId, parse_creds_json, to_hex_c_array, get_format, CredFormat

def create_hardcoded_creds_file(creds: dict, 
                                loc: str = "./src/creds/hardcoded_creds_data.c"):
    with open(hardcoded_creds_file, "w+") as f:
        f.write('#include "hardcoded_creds.h"\n')
    
        for cred_id, loc in creds.items():
            with open(loc, "rb") as fr:
                data = fr.read()
                fmt = get_format(loc)

            # Add additional EOS required by MbedTLS to recognize a valid PEM certificate
            if fmt is CredFormat.PEM:
                data += b"\x00"

            f.write(to_hex_c_array(data, f"{cred_id.name.lower()}"))
        
        f.write('const struct hardcoded_cred creds_harcoded_array[CREDS_HARDCODED_MAX_COUNT] = {\n')
        for cred_id, loc in creds.items():
            f.write("\t{\n")
            f.write(f'\t\t{cred_id.name},\n')
            f.write(f"\t\t{cred_id.name.lower()},\n")
            f.write(f"\t\t{cred_id.name.lower()}_len\n")
            f.write("\t},\n")
        
        f.write("};\n")


if __name__ == "__main__":
    hardcoded_creds_file = "./src/creds/hardcoded_creds_data.c"
    creds = parse_creds_json()

    create_hardcoded_creds_file(creds, hardcoded_creds_file)