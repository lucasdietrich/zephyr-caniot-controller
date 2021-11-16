# Crypto (generation, test, etc..)

## HTTPS Server

Passphrase : `caniot` :

### Generate RSA private key (2048 bits) :

Create a pair of self-signed x509 certiticates (private key, public certificate)

- https://stackoverflow.com/questions/10175812/how-to-generate-a-self-signed-ssl-certificate-using-openssl
```
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -sha256 -days 365
```

This key is protected by a passphrase, to create the non-protected version :
```
openssl rsa -in key.pem -out key2.der -outform DER
```

### Convert to DER

Public certificate : 
- https://www.ssl.com/guide/pem-der-crt-and-cer-x-509-encodings-and-conversions/
```
openssl x509 -inform PEM -in cert.pem -outform DER -out cert.der
```

Private key
- https://wiki.segger.com/HOWTO_convert_PEM_certificates_and_keys_to_DER_format_for_emSSL
```
openssl rsa -inform PEM -in key.pem -outform DER -out key.der
```

### Show certificate

```
openssl x509 -in cert.der -inform DER -text -noout
```

```
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            18:fd:79:34:35:c3:d4:63:89:f5:e8:ab:24:8f:ef:15:9e:0e:9c:8e
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: C = FR, ST = Some-State, L = Kogenheim, O = Internet Widgits Pty Ltd, CN = Lucas Dietrich, emailAddress = ld.adecy@gmail.com
        Validity
            Not Before: Nov 15 21:07:31 2021 GMT
            Not After : Nov 15 21:07:31 2022 GMT
        Subject: C = FR, ST = Some-State, L = Kogenheim, O = Internet Widgits Pty Ltd, CN = Lucas Dietrich, emailAddress = ld.adecy@gmail.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (2048 bit)
                Modulus:
                    00:c9:5d:1e:ee:bc:53:84:04:d3:bc:1a:9b:f9:65:
                    1d:80:ab:69:06:09:d4:21:e6:93:1b:8e:a8:a7:f0:
                    c5:0a:74:0d:db:b2:16:64:d1:f0:ae:59:56:c7:3a:
                    e6:b0:96:71:06:14:0c:a1:6e:c3:84:6e:95:1a:b4:
                    ff:bd:fa:c2:66:e2:6b:9a:9e:5a:b3:4d:64:e2:66:
                    fd:af:40:e3:d3:fa:5a:3d:96:0d:b7:4c:eb:e1:26:
                    c9:97:50:7f:1c:5f:b3:18:ec:43:54:82:aa:46:ac:
                    02:62:03:80:36:91:5b:fc:57:dc:65:93:e1:ee:81:
                    15:32:de:9a:19:c8:16:7e:03:be:29:cf:52:c8:3f:
                    c9:d5:69:ab:40:a1:e2:3a:2f:79:79:3f:c5:80:ed:
                    e3:a6:20:85:ed:2d:a9:ad:af:95:bd:5b:cb:cb:90:
                    74:49:a6:af:65:28:f3:5e:bd:ea:45:6b:0b:0a:cf:
                    33:a7:43:f9:de:e1:e2:8a:3c:9e:00:60:4b:dd:eb:
                    6d:1f:9c:9d:d3:21:20:a7:6c:61:d6:1c:5a:fa:e7:
                    9d:82:eb:38:32:94:33:ff:04:d4:20:2b:18:d7:3d:
                    2c:33:14:b6:7c:1d:98:56:24:71:fd:f6:79:7f:da:
                    5a:c6:28:ca:78:d5:78:f9:08:63:ac:27:25:f3:19:
                    2b:f3
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Subject Key Identifier:
                A6:89:7B:56:FD:F5:33:1B:0E:B2:EE:97:42:59:FA:B8:C4:4B:BB:CF
            X509v3 Authority Key Identifier:
                keyid:A6:89:7B:56:FD:F5:33:1B:0E:B2:EE:97:42:59:FA:B8:C4:4B:BB:CF

            X509v3 Basic Constraints: critical
                CA:TRUE
    Signature Algorithm: sha256WithRSAEncryption
         c0:dd:0a:24:d0:61:27:8d:de:68:03:f3:0e:0b:b4:6d:f1:3b:
         ca:03:77:2e:ae:42:34:b9:9d:8d:99:28:d8:f0:73:54:d3:d1:
         a8:33:75:a0:39:d5:3b:c1:41:01:7d:fc:fa:da:69:47:99:df:
         3c:63:b9:71:00:08:37:de:0e:5b:c6:eb:cb:a3:2e:81:df:a0:
         6e:95:04:5d:8d:f7:7d:04:66:c9:9c:93:6d:5c:d0:30:bb:3c:
         32:bd:44:e7:d1:cf:43:bb:6d:88:44:e7:62:e6:43:25:d2:fc:
         de:97:8c:f0:b0:1a:ae:e8:84:97:43:c1:d4:6d:41:ab:9b:53:
         7a:92:64:2f:5a:d2:c9:1e:e0:9a:fc:bd:9b:b5:79:91:94:cf:
         10:18:fd:8b:55:80:69:aa:1b:84:64:b3:40:06:1a:c3:76:fc:
         f4:34:9a:ff:22:dc:2b:ed:bd:9e:67:93:ce:d2:6c:05:45:42:
         d9:8c:d0:70:66:2a:21:13:a1:6f:db:0f:56:10:cd:66:1e:13:
         75:66:d1:89:ad:a7:aa:06:bd:b9:e3:27:dc:93:8b:f5:6a:42:
         ff:cd:3a:90:24:ae:8c:d4:66:69:f2:fd:f0:62:19:6a:7d:af:
         e0:5e:c7:20:ba:dc:49:73:4d:23:0c:71:75:40:6d:e9:d5:16:
         be:13:44:76
```

### Show RSA private key

```
openssl rsa -in key.der -inform DER -text -noout
```

```
RSA Private-Key: (2048 bit, 2 primes)
modulus:
    00:c9:5d:1e:ee:bc:53:84:04:d3:bc:1a:9b:f9:65:
    1d:80:ab:69:06:09:d4:21:e6:93:1b:8e:a8:a7:f0:
    c5:0a:74:0d:db:b2:16:64:d1:f0:ae:59:56:c7:3a:
    e6:b0:96:71:06:14:0c:a1:6e:c3:84:6e:95:1a:b4:
    ff:bd:fa:c2:66:e2:6b:9a:9e:5a:b3:4d:64:e2:66:
    fd:af:40:e3:d3:fa:5a:3d:96:0d:b7:4c:eb:e1:26:
    c9:97:50:7f:1c:5f:b3:18:ec:43:54:82:aa:46:ac:
    02:62:03:80:36:91:5b:fc:57:dc:65:93:e1:ee:81:
    15:32:de:9a:19:c8:16:7e:03:be:29:cf:52:c8:3f:
    c9:d5:69:ab:40:a1:e2:3a:2f:79:79:3f:c5:80:ed:
    e3:a6:20:85:ed:2d:a9:ad:af:95:bd:5b:cb:cb:90:
    74:49:a6:af:65:28:f3:5e:bd:ea:45:6b:0b:0a:cf:
    33:a7:43:f9:de:e1:e2:8a:3c:9e:00:60:4b:dd:eb:
    6d:1f:9c:9d:d3:21:20:a7:6c:61:d6:1c:5a:fa:e7:
    9d:82:eb:38:32:94:33:ff:04:d4:20:2b:18:d7:3d:
    2c:33:14:b6:7c:1d:98:56:24:71:fd:f6:79:7f:da:
    5a:c6:28:ca:78:d5:78:f9:08:63:ac:27:25:f3:19:
    2b:f3
publicExponent: 65537 (0x10001)
privateExponent:
    37:4c:99:88:1a:b4:1c:d7:6e:86:84:10:3d:a7:65:
    38:c6:da:2a:cc:5c:33:8b:2c:ef:2e:78:66:d6:9a:
    66:4a:84:db:c6:c6:9b:9d:84:29:7d:63:75:87:59:
    7f:39:71:84:a7:d0:e8:2c:91:09:37:8d:3f:2e:61:
    e5:7c:8e:12:cf:2f:99:e0:cf:2b:da:9f:50:05:d4:
    b9:62:6a:ae:f6:5b:fd:f9:cd:7e:7f:63:70:0b:c4:
    e5:07:38:41:44:8e:dc:d3:55:92:49:e8:15:4c:7b:
    c3:0b:0f:14:ed:4a:83:bc:65:4c:88:16:4d:f7:a9:
    28:b5:35:40:cb:50:51:fe:9c:35:53:ce:a4:b7:aa:
    81:df:51:19:ee:04:34:d9:50:e5:e5:a0:02:9d:a1:
    f6:30:44:29:38:fd:65:46:2b:47:56:5d:d9:58:27:
    3d:50:19:99:be:df:41:f2:e6:71:23:c1:4b:dd:13:
    1a:a5:41:36:67:aa:f8:bc:85:b0:25:10:2b:a7:47:
    56:19:01:1b:7e:90:75:b6:d8:6b:58:d4:ce:b1:ed:
    9b:fa:3a:b7:6d:f2:8b:79:80:22:ea:78:b2:50:35:
    3a:c6:b2:d0:23:25:c8:a1:0d:fb:4b:58:f8:42:87:
    c8:cb:f8:37:f3:78:6b:f5:70:03:e4:6e:45:9e:9c:
    21
prime1:
    00:f1:42:25:2c:1f:b2:fd:a4:f9:24:35:c7:90:cd:
    cb:a6:65:91:26:c2:8d:ea:2b:fc:76:04:89:72:a6:
    01:53:aa:2a:8f:1f:be:03:49:16:32:a3:ce:43:35:
    4e:e9:8f:ae:38:7e:07:93:33:03:c0:25:59:74:42:
    3e:e1:78:0a:a2:42:88:a2:c4:55:31:02:a7:ab:df:
    37:2d:dd:03:0e:1f:64:c5:c9:2a:4d:a2:99:b2:90:
    99:6f:ad:9b:9e:2b:9e:56:67:2d:7b:6e:02:6b:81:
    f1:7e:ff:d2:d6:bf:5a:39:f0:52:36:45:60:2b:3f:
    b0:fd:e7:49:31:06:c8:a5:ab
prime2:
    00:d5:aa:ed:c5:79:52:93:8e:7d:66:82:51:1d:02:
    b6:c0:bd:47:7f:b4:32:1b:d8:28:2e:9c:69:f9:52:
    c9:df:74:42:b7:24:69:51:b7:a9:9f:3c:69:70:2b:
    e8:03:69:70:34:f4:a3:00:3d:bd:7b:78:d4:9f:51:
    b2:f1:10:d8:0f:d6:64:e1:a9:fd:1a:4f:28:02:e6:
    ff:7d:f6:68:2e:e8:ac:00:85:53:b2:2f:22:29:73:
    7b:87:17:3c:29:62:69:72:0b:91:ec:12:99:f7:9b:
    4a:b9:02:f4:d5:ef:4d:cd:c7:22:60:99:22:66:e8:
    82:70:6a:fe:a4:04:74:3a:d9
exponent1:
    6f:97:01:ac:94:08:ed:4b:26:35:ba:6e:51:2a:ef:
    1c:4a:f4:45:72:4d:c3:d7:8e:91:63:ed:d3:4a:b7:
    68:64:58:05:15:50:85:22:84:12:ee:33:54:60:ce:
    dc:37:2a:05:55:3d:d9:b7:09:f0:11:16:7c:30:bb:
    f6:fb:d8:27:4f:10:f6:00:4b:cb:3c:88:23:76:3e:
    86:87:28:87:9a:bb:b8:c4:20:3e:02:8c:86:cc:5d:
    3c:0b:97:e4:24:16:bb:ae:43:9a:48:ba:f3:d1:09:
    cb:8d:36:8f:3f:b8:d2:fd:b6:79:05:c3:c2:9d:56:
    17:4a:a8:4e:f5:ed:4e:bb
exponent2:
    00:ce:35:4d:8e:04:9d:b6:3c:91:37:aa:53:40:0d:
    4b:74:cd:f7:bf:fe:97:51:9a:16:85:8d:7d:15:1b:
    5a:2a:a6:d8:70:49:da:be:fb:e2:df:03:fd:ba:3d:
    15:88:9d:6c:a6:1e:e7:65:27:30:c4:86:03:a5:d2:
    c2:40:b7:01:de:9f:09:f3:64:0c:1a:25:04:b7:70:
    5a:69:25:b2:bc:7a:de:ed:0d:bf:8d:ba:c1:5c:81:
    d0:58:bb:0a:db:e1:d7:64:32:58:5d:1e:42:ab:dd:
    9a:8a:dd:98:8f:13:89:e2:2b:ea:38:91:f1:ca:a1:
    60:a9:c0:09:0b:20:25:50:59
coefficient:
    30:92:5a:bb:45:b5:d1:45:33:22:16:d9:9c:cf:63:
    50:2e:44:af:24:16:1f:8e:95:42:6e:6f:9e:27:99:
    5c:91:16:f9:f0:fa:51:c4:0d:13:88:47:9f:f1:6c:
    80:25:3a:5f:91:0b:1c:ea:e9:72:cf:ad:c5:d6:54:
    b5:59:56:eb:12:5f:ca:ad:57:b0:0e:fb:00:55:54:
    e8:aa:ba:d0:e0:c6:ba:ce:f6:83:2e:e6:ce:af:24:
    3e:de:a8:a3:01:1a:80:ca:e5:f9:73:cb:32:ba:71:
    33:ab:c4:c5:22:67:b9:d6:0e:ff:b8:d0:ef:60:f1:
    94:5b:0c:e4:e8:e7:79:6f
```

---

## Misc

### PEM

```
openssl genrsa -out key.pem 2048
```

Extract public certificate in PEM
```
openssl rsa -in key.pem -inform PEM -pubout -outform PEM -out public.pem
```

### DER

Convert private key to DER
```
openssl rsa -in key.pem -inform PEM -outform DER -out key.der
```

Extract public certificate in DER
```
openssl rsa -in key.pem -inform PEM -pubout -outform DER -out public.der
```

#### Misc
```
> openssl asn1parse -inform DER -in public.der
    0:d=0  hl=4 l= 290 cons: SEQUENCE
    4:d=1  hl=2 l=  13 cons: SEQUENCE
    6:d=2  hl=2 l=   9 prim: OBJECT            :rsaEncryption
   17:d=2  hl=2 l=   0 prim: NULL
   19:d=1  hl=4 l= 271 prim: BIT STRING
```

## ~~Check that a RSA pair (public cert/private key) is valid~~

```
openssl x509 -noout -modulus -inform PEM public.pem -in public.pem | openssl md5 > /tmp/crt.pem.pub
openssl rsa -noout -modulus -inform PEM -in key.pem | openssl md5 > /tmp/key.pem.pub
diff /tmp/crt.pem.pub /tmp/key.pem.pub
```