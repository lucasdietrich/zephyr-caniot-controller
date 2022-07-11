#ifndef _HTTP_FILES_SERVER_H_
#define _HTTP_FILES_SERVER_H_

#include <stdint.h>

#include <data/json.h>
#include <net/http_parser.h>

#include "http_utils.h"
#include "http_request.h"

int http_file_upload(struct http_request *req,
		     struct http_response *resp)
{
	return 0;
}

#endif /* _HTTP_FILES_SERVER_H_ */