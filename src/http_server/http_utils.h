#ifndef _HTTP_UTILS_H_
#define _HTTP_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	HTTP_CONTENT_TYPE_NONE = 0,
	HTTP_CONTENT_TYPE_TEXT_PLAIN,
	HTTP_CONTENT_TYPE_TEXT_HTML,
	HTTP_CONTENT_TYPE_APPLICATION_JSON,
	HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA,
	HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM,
} http_content_type_t;

typedef enum {
	/* 400 */
	HTTP_OK = 200,
	HTTP_CREATED = 201,
	HTTP_ACCEPTED = 202,
	HTTP_NO_CONTENT = 204,

	/* 400 */
	HTTP_BAD_REQUEST = 400,
	HTTP_UNAUTHORIZED = 401,
	HTTP_FORBIDDEN = 403,
	HTTP_NOT_FOUND = 404,
	// HTTP_METHOD_NOT_ALLOWED = 405,
	// HTTP_NOT_ACCEPTABLE = 406,
	HTTP_REQUEST_TIMEOUT = 408,
	// HTTP_CONFLICT = 409,
	// HTTP_GONE = 410,
	HTTP_LENGTH_REQUIRED = 411,
	// HTTP_PRECONDITION_FAILED = 412,
	HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
	HTTP_REQUEST_URI_TOO_LONG = 414,
	HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
	// HTTP_RANGE_NOT_SATISFIABLE = 416,

	/* 500 */
	HTTP_INTERNAL_SERVER_ERROR = 500,
	HTTP_NOT_IMPLEMENTED = 501,
	HTTP_BAD_GATEWAY = 502,
	HTTP_SERVICE_UNAVAILABLE = 503,
	HTTP_GATEWAY_TIMEOUT = 504,
	HTTP_HTTP_VERSION_NOT_SUPPORTED = 505,
	HTTP_INSUFFICIENT_STORAGE = 507,

} http_status_code_t;

int http_encode_status(char *buf, size_t len, uint16_t status_code);

int http_encode_header_content_length(char *buf, size_t len, size_t content_length);

int http_encode_header_connection(char *buf, size_t len, bool keep_alive);

int http_encode_header_content_type(char *buf, size_t len, http_content_type_t type);

int http_encode_header_end(char *buf, size_t len);

bool http_code_has_payload(uint16_t status_code);

const char *http_content_type_to_str(http_content_type_t content_type);

// typedef struct
// {
// 	uint8_t *buf;
// 	uint16_t 
// 	uint16_t len;
// 	uint16_t id;
// } http_chunk_t;

 

#endif