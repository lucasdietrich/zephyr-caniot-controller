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

int http_encode_status(char *buf, size_t len, uint16_t status_code);

int http_encode_header_content_length(char *buf, size_t len, size_t content_length);

int http_encode_header_connection(char *buf, size_t len, bool keep_alive);

int http_encode_header_content_type(char *buf, size_t len, http_content_type_t type);

int http_encode_header_end(char *buf, size_t len);

bool http_code_has_payload(uint16_t status_code);

const char *http_content_type_to_str(http_content_type_t content_type);

typedef struct
{
	uint8_t *loc;
	uint16_t len;
	uint16_t id;
} http_chunk_t;

 

#endif