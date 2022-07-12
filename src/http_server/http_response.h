#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <stdio.h>
#include <stdint.h>

#include "utils.h"
#include "http_utils.h"

struct http_response
{
	http_content_type_t content_type;

	uint16_t status_code;

	union {
		/* This is as VERY DANGEROUS trick */
		buffer_t buffer;
		struct {
			char *buf;
			size_t buf_size;
			size_t content_len;
		};
	};
};

typedef struct http_response http_response_t;

#endif