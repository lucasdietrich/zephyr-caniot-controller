# Net

Following features use NET functions:

- UDP discovery server
- HTTP(S) server:
  - REST API
  - FILE server
  - WEBSERVER
  - TEST server
- Cloud client
  - AWS IoT

## Memory usage

- mbedTLS stack
- NET buffers

## Improve bandwidth

Following configuration improve bandwidth for transmission:

```
CONFIG_NET_BUF_DATA_SIZE=128

CONFIG_NET_PKT_RX_COUNT=16
CONFIG_NET_BUF_RX_COUNT=36

CONFIG_NET_PKT_TX_COUNT=32
CONFIG_NET_BUF_TX_COUNT=128
```