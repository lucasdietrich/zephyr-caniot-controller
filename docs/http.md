# HTTP

## Todo

- Improve HTTP performances (50ms per request is too much)
        - check with wireshark
        - it depends on the client :
                - 50ms with VS code REST
                - 1 ms from controller PoV
                - 50ms from python REQUESTS
                - 2 > 20 ms from python raw TCP