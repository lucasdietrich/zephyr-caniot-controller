def bin2c(data: bytes, name: str = "array"):
        cols = 16

        s = f"static const char {name}[] = " + "{\n" + " "*8

        for i, b in enumerate(data):
                s += f"0x{b:02x}, "
                if i & 0x7 == 0x7: 
                        s += "\n" + " "*8

        return s + "};"

with open ("./src/http_server/creds/rsa1024/cert.der", "rb+") as fp:
        c_array = bin2c(fp.read())
        print(c_array)

with open ("./src/http_server/creds/rsa1024/key2.der", "rb+") as fp:
        c_array = bin2c(fp.read())
        print(c_array)