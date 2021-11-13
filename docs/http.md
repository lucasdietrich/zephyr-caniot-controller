# HTTP


## Todo

- Add timer to close sockets with no activity
- Bug : Cannot have chrome keep-alive (2 sockets) + VS CODE REST request at the same time

## Done

- Improve HTTP performances (50ms per request is too much)
        - check with wireshark
        - it depends on the client :
                - 50ms with VS code REST
                - 1 ms from controller PoV
                - 50ms from python REQUESTS
                - 2 > 20 ms from python raw TCP

-  Still problems with 3 simultaneous connectiosn : 

```
[00:01:09.558,000] <inf> http_server: (6) Connection closed by peer
[00:01:09.558,000] <inf> http_server: Closing sock (6) conn 0x20000e70
[00:01:09.558,000] <err> net_sock: invalid access on sock 6 by thread 0x20000ea8
[00:01:09.558,000] <err> http_server: recv failed = -1
[00:01:09.558,000] <err> net_sock: invalid access on sock 6 by thread 0x20000ea8
[00:01:09.558,000] <inf> http_server: Closing sock (6) conn 0x20000e38
```