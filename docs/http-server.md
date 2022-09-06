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
| **`HTTP_SERVER`**                      | Globally enable HTTP Server                                  |
| `HTTP_SERVER_NONSECURE`            | Enable HTTP Server non-secure port (HTTP 80)                 |
| `HTTP_SERVER_SECURE`               | Enable HTTP Server secure port (HTTPS 443)                   |
| `HTTP_MAX_CONNECTIONS`             | Maximum number of simultaneous connections                   |
| `HTTP_REQUEST_HEADERS_BUFFER_SIZE` | Size of the buffer used to store HTTP request custom headers |
| `HTTP_TEST`                        | Enable HTTP test tool                                        |
| `HTTP_TEST_SERVER`                 | Enable HTTP test server tool , with test resources           |
| `FILE_UPLOAD_MOUNT_POINT`          | Mount point for file upload to FILE server                   |


## Provision HTTPS certificates

Please refer to the [credentials documentation](credentials.md) for more information.

Basically, if you need to provision the following credentials (which can be either PEM or DER encoded):
- `CRED_HTTPS_SERVER_PRIVATE_KEY`
- `CRED_HTTPS_SERVER_CERTIFICATE`

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