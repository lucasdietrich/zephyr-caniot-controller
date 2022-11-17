# HTTP Server

The HTTP server currently supports the following features:
- REST API
- Simple static file server
- Prometheus metrics
- Simple Webserver

The server can be accessed via either HTTP (80) or HTTPS (443).

## Configuration options*

See [src/http_server/Kconfig](../src/http_server/Kconfig) :

| Option                             | Description                                                  |
| ---------------------------------- | ------------------------------------------------------------ |
| **`HTTP_SERVER`**                  | Globally enable HTTP Server                                  |
| `HTTP_SERVER_NONSECURE`            | Enable HTTP Server non-secure port (HTTP 80)                 |
| `HTTP_SERVER_SECURE`               | Enable HTTP Server secure port (HTTPS 443)                   |
| `HTTP_SERVER_VERIFY_CLIENT`        | Enable client certificate verification                       |
| `HTTP_MAX_CONNECTIONS`             | Maximum number of simultaneous connections                   |
| `HTTP_REQUEST_HEADERS_BUFFER_SIZE` | Size of the buffer used to store HTTP request custom headers |
| `HTTP_TEST`                        | Enable HTTP test tool                                        |
| `HTTP_TEST_SERVER`                 | Enable HTTP test server tool , with test resources           |
| `FILES_SERVER_MOUNT_POINT`          | Mount point for file upload to FILE server                   |

## Provision HTTPS certificates

Basically, if you need to provision the following credentials (which can be either PEM or DER encoded):
- `CRED_HTTPS_SERVER_PRIVATE_KEY`
- `CRED_HTTPS_SERVER_CERTIFICATE`

Please refer to the [credentials documentation](credentials.md) for general information.

The can use the following [creds/https_server/Makefile](../creds/https_server/Makefile) to
generate a private key and a certificate for the HTTPS server.

## Client certificate verification

If you want to verify certificates of clients connecting to the HTTPS server, 
you need to provision the credential `HTTPS_SERVER_CLIENT_CA` (or `HTTPS_SERVER_CLIENT_CA_DER`).
You can test your credentials are valid by using the following command:
    
        openssl s_client -connect 192.0.2.1:443 -cert creds/https_server/rsa2048/cert.pem -key creds/https_server/rsa2048/key.pem -CAfile creds/https_server/rsa2048/ca.cert.pem

Expected result :
```
[lucas@fedora stm32f429zi-caniot-controller]$   openssl s_client -connect 192.0.2.1:443 -cert creds/https_server/rsa2048/cert.pem -key creds/https_server/rsa2048/key.pem -CAfile creds/https_server/rsa2048/ca.cert.pem
CONNECTED(00000003)
Can't use SSL_get_servername
depth=1 C = FR, ST = Alsace, L = Benfeld, O = Home, CN = caniot-ca
verify return:1
depth=0 C = FR, ST = Alsace, L = Benfeld, O = Home, CN = caniot-lucas-dev
verify return:1
---
Certificate chain
 0 s:C = FR, ST = Alsace, L = Benfeld, O = Home, CN = caniot-lucas-dev
   i:C = FR, ST = Alsace, L = Benfeld, O = Home, CN = caniot-ca
   a:PKEY: rsaEncryption, 2048 (bit); sigalg: RSA-SHA256
   v:NotBefore: Sep  7 22:55:23 2022 GMT; NotAfter: Sep  4 22:55:23 2032 GMT
---
Server certificate
-----BEGIN CERTIFICATE-----
MIIDNDCCAhwCFBvtjN6cQq8DP1a0UHFjThQmKnqNMA0GCSqGSIb3DQEBCwUAMFMx
CzAJBgNVBAYTAkZSMQ8wDQYDVQQIDAZBbHNhY2UxEDAOBgNVBAcMB0JlbmZlbGQx
DTALBgNVBAoMBEhvbWUxEjAQBgNVBAMMCWNhbmlvdC1jYTAeFw0yMjA5MDcyMjU1
MjNaFw0zMjA5MDQyMjU1MjNaMFoxCzAJBgNVBAYTAkZSMQ8wDQYDVQQIDAZBbHNh
Y2UxEDAOBgNVBAcMB0JlbmZlbGQxDTALBgNVBAoMBEhvbWUxGTAXBgNVBAMMEGNh
bmlvdC1sdWNhcy1kZXYwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC9
sSZvHOCpDUf2zCttpbvSHCtabj1dcYK2gs1ufg02kCKhNpsB/cTh60CBMaW8Q0Er
Ta6D62hgHeb5yFs+Ige0V4gUzzG3pssn1daCjJwE4jucgmEGB8qSXAbG5yBKn/P5
2DWC5o6zTBU5MSoRy81OAVg+5C0iE69Z1HlKjqssR/BP8oK9RhXo4WK8YNB1HE1F
RHznyF9/b0gF+dTSVD6UuqYfUOyf4Qnbp9/+DWrJc2Wv4y99SZgqCbQS7imtmEg6
fBLqNuCFAUXa4aHjAwnKMb1sFDl0tBRb5itSnaBHmER6TOpqS/hsGceXLfcnDYTW
5SRpUATutQLWD58wK4sBAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAJb5lEg5QIXk
wDbFZhHepjbzuSMRc8BuFdwcmVg3lR8+xLMrmeJgzxF/JRPEIpZ3sPThYX8OFaIu
J2G50ZOqnNIDc7iby0psNZGa2fnh6RiecuBetfWRtFYEsugKSiqDO48wWCAO/S0v
z/IX2cRO4IdOjIxURVaIEA0YOCCL9q1hWdEIS56SfX2GWtVjNrzjT9KDkRcPKkN/
+YWOylMGTlbMmc0xf9fIfGdw0JK3pJe914+FzJLADtrbce26b1XC2NRZ0pv68ixd
5l8W6EySTV0pU4dY8xDn+f4YC/djnxTH/NZuCFli7zoVztq1N5eNZS81TvAln5hV
5JJUpgmu/yY=
-----END CERTIFICATE-----
subject=C = FR, ST = Alsace, L = Benfeld, O = Home, CN = caniot-lucas-dev
issuer=C = FR, ST = Alsace, L = Benfeld, O = Home, CN = caniot-ca
---
No client certificate CA names sent
---
SSL handshake has read 1025 bytes and written 653 bytes
Verification: OK
---
New, TLSv1.2, Cipher is AES256-SHA256
Server public key is 2048 bit
Secure Renegotiation IS supported
Compression: NONE
Expansion: NONE
No ALPN negotiated
SSL-Session:
    Protocol  : TLSv1.2
    Cipher    : AES256-SHA256
    Session-ID: 0E884FA73852EAE2621C851E8CE61F5AB5B0BA95E07A55D10A45F00C340F8B48
    Session-ID-ctx: 
    Master-Key: DF63A1384017FE9E86409B3E8B5B4DE0D0B6FA6F24A2AEBCC00C29D1384F4F667290E0CB50A756BBA42E1CDD4F42754A
    PSK identity: None
    PSK identity hint: None
    SRP username: None
    Start Time: 1662591978
    Timeout   : 7200 (sec)
    Verify return code: 0 (ok)
    Extended master secret: no
---
```

## REST API

The Swagger describing the REST API is available here [swagger-local-api.yaml](./swagger-local-api.yaml).

## Webserver

If using the webserver, I advice to use Mozilla Firefox to access it, chrome 
makes many concurrent requests (to retrieve favicons, etc.) and it makes
the debug noisy.

## Prometheus metrics

Prometheus metrics are available at `/metrics` endpoint.

## File server

*Todo documentation*

## Todo

Add support for ECC certificates