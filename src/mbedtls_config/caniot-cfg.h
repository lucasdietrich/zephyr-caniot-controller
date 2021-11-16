/* https://tls.mbed.org/kb/how-to/reduce-mbedtls-memory-and-storage-footprint */

#define MBEDTLS_AES_ROM_TABLES

#define MBEDTLS_HAVE_TIME
#define MBEDTLS_HAVE_TIME_DATE
#define MBEDTLS_PLATFORM_TIME_ALT

#undef MBEDTLS_SSL_KEEP_PEER_CERTIFICATE