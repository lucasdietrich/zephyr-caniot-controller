# Have a HTTP(S) server properly working in any cases
- add timeout on old connections
- fix no close notify problem
- fix poll return 0 problem
- fix accept fail problem (TLS context not deallocated ?)
- Add Kconfig for RSA key size 1024 or 2048 ?

# Use a proper zephyr environnement instead of th PlatformIO one
- or use both ??
