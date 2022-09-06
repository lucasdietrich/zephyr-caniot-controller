import subprocess
import os.path

if __name__ == "__main__":
    hardcoded_creds_file = "./src/creds/hardcoded_creds_data.c"

    with open(hardcoded_creds_file, "w+") as f:
        f.write('#include "hardcoded_creds.h"\n')
        
        f.write('const struct hardcoded_cred creds_harcoded_array[CREDS_HARDCODED_MAX_COUNT] = { };\n')

    # Add "static const" in file
    with open(hardcoded_creds_file, "r") as f:
        data = f.read()
        
    data = data.replace("unsigned char", "static const unsigned char")
    data = data.replace("unsigned int", "static const unsigned int")

    with open(hardcoded_creds_file, "w") as f:
        f.write(data)
