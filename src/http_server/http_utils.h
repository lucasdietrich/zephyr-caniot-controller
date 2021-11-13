#ifndef _HTTP_UTILS_H_
#define _HTTP_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

int http_encode_status(char *buf, size_t len, uint16_t status_code);

int http_encode_header_content_length(char *buf, size_t len, size_t content_length);

int http_encode_header_connection(char *buf, size_t len, bool keep_alive);

int http_encode_header_end(char *buf, size_t len);

bool http_code_has_payload(uint16_t status_code);

#endif