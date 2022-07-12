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

	uint16_t content_length;

	union {
		
		buffer_t buffer;

		/* This is as VERY DANGEROUS trick */
		/* TODO remove this struct and only use the "buffer_t" member */
		// struct {
		// 	char *buf;
		// 	size_t buf_size;
		// 	size_t content_len;
		// };
	};
};

typedef struct http_response http_response_t;

void http_response_init(http_response_t *resp);

#endif