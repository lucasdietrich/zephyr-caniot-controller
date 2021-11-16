# MBEDTLS

Optimisation : https://tls.mbed.org/kb/how-to/reduce-mbedtls-memory-and-storage-footprint

# Issues

## DONE - 3s HTTPS request to 0.5s

- Very long latency : 2.718s

Using CCM instead of RAM
- NO - RAM -> CCM
- NO - CONFIG_NET_BUF_TX_COUNT 36 -> 72 (and CONFIG_NET_BUF_RX_COUNT) no change
- NO - MBEDTLS debug mode (but massive debug increase a lot the reponse time -> 30s with full logs)
- 1.5s - reducing number of ciphers helped a lot to reduce 
```
# CONFIG_MBEDTLS_CIPHER_ALL_ENABLED=y
CONFIG_MBEDTLS_CIPHER_AES_ENABLED=y
CONFIG_MBEDTLS_CIPHER_MODE_CBC_ENABLED=y

# CONFIG_MBEDTLS_KEY_EXCHANGE_ALL_ENABLED=y
CONFIG_MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED=y
CONFIG_MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED=y

# CONFIG_MBEDTLS_MAC_ALL_ENABLED=y
CONFIG_MBEDTLS_MAC_MD5_ENABLED=y
CONFIG_MBEDTLS_MAC_SHA1_ENABLED=y
CONFIG_MBEDTLS_MAC_SHA256_ENABLED=y
CONFIG_MBEDTLS_MAC_SHA512_ENABLED=y

# CONFIG_MBEDTLS_ECP_ALL_ENABLED=y
CONFIG_MBEDTLS_ECP_DP_BP256R1_ENABLED=y
```
- NO - MBEDTLS_SSL_KEEP_PEER_CERTIFICATE has no effect
- 0.5s - Reducing rsa key size from 2048 to 1024 helped to get full https request in 0.5s

## MBTLS Polling returning 0 in TLS (if close notify is missing ?)


## Accept = -1 ?
