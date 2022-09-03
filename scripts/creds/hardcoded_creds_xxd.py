import subprocess
import os.path

if __name__ == "__main__":
    creds = {
        "CRED_HTTPS_SERVER_CERTIFICATE": "./creds/https_server/rsa2048/cert.der",
        "CRED_HTTPS_SERVER_PRIVATE_KEY": "./creds/https_server/rsa2048/key.der",

        "CRED_AWS_CERTIFICATE": "./creds/AWS/caniot-controller/cert.pem",
        "CRED_AWS_PRIVATE_KEY": "./creds/AWS/caniot-controller/key.pem",

        "CRED_AWS_CERTIFICATE_DER": "./creds/AWS/caniot-controller/cert.der",
        "CRED_AWS_PRIVATE_KEY_DER": "./creds/AWS/caniot-controller/key.der",

        "CRED_AWS_ROOT_CA1": "./creds/AWS/AmazonRootCA1.pem",
        "CRED_AWS_ROOT_CA3": "./creds/AWS/AmazonRootCA3.pem",

        "CRED_AWS_ROOT_CA1_DER": "./creds/AWS/AmazonRootCA1.der",
        "CRED_AWS_ROOT_CA3_DER": "./creds/AWS/AmazonRootCA3.der",
    }

    hardcoded_creds_file = "./src/creds/hardcoded_creds_data.c"

    with open(hardcoded_creds_file, "w+") as f:
        f.write('#include "hardcoded_creds.h"\n')

        for cred, path in creds.items():
            output = subprocess.call(["xxd", "--include", "-n", cred.lower(), path], stdout=f)
        
        f.write('const struct hardcoded_cred creds_harcoded_array[CREDS_HARDCODED_MAX_COUNT] = {\n')
        for cred, path in creds.items():
            f.write("\t{\n")
            f.write(f'\t\t{cred},\n')
            f.write(f"\t\t{cred.lower()},\n")
            f.write(f"\t\t{cred.lower()}_len\n")
            f.write("\t},\n")
        
        f.write("};\n")

    # Add "static const" in file
    with open(hardcoded_creds_file, "r") as f:
        data = f.read()
    
    data = data.replace("unsigned char", "static const unsigned char")
    data = data.replace("unsigned int", "static const unsigned int")

    with open(hardcoded_creds_file, "w") as f:
        f.write(data)
