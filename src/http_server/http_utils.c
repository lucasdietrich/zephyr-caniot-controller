#include <stdio.h>
#include <kernel.h>

#include "http_utils.h"

#include "utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_utils, LOG_LEVEL_DBG);

/*____________________________________________________________________________*/

struct code_str
{
        http_status_code_t code;
        char *str;
};

static const struct code_str status[] = {
	{HTTP_OK, "OK"},
	{HTTP_CREATED, "Created"},
	{HTTP_ACCEPTED, "Accepted"},
	{HTTP_NO_CONTENT, "No Content"},

	{HTTP_BAD_REQUEST, "Bad Request"},
	{HTTP_UNAUTHORIZED, "Unauthorized"},
	{HTTP_FORBIDDEN, "Forbidden"},
	{HTTP_NOT_FOUND, "Not Found"},
	{HTTP_REQUEST_TIMEOUT, "Request Timeout"},
	{HTTP_LENGTH_REQUIRED, "Length Required"},
	{HTTP_REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large"},
	{HTTP_REQUEST_URI_TOO_LONG, "Request-URI Too Long"},
	{HTTP_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type"},

	{HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error"},
	{HTTP_NOT_IMPLEMENTED, "Not Implemented"},
	{HTTP_BAD_GATEWAY, "Bad Gateway"},
	{HTTP_SERVICE_UNAVAILABLE, "Service Unavailable"},
	{HTTP_GATEWAY_TIMEOUT, "Gateway Timeout"},
	{HTTP_HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported"},
	{HTTP_INSUFFICIENT_STORAGE, "Insufficient Storage"},
};

static const char *get_status_code_str(http_status_code_t status_code)
{
        for (const struct code_str *p = status;
             p <= &status[ARRAY_SIZE(status) - 1]; p++) {
                if (p->code == status_code) {
                        return p->str;
                }
        }
        return NULL;
}

int http_encode_status(char *buf, size_t len, http_status_code_t status_code)
{
        const char *code_str = get_status_code_str(status_code);
        if (code_str == NULL) {
                LOG_ERR("unknown status_code %d", status_code);
                return -1;
        }

        return snprintf(buf, len, "HTTP/1.1 %d %s\r\n", status_code, code_str);
}

int http_encode_header_content_length(char *buf, size_t len, size_t content_length)
{
        return snprintf(buf, len, "Content-Length: %u\r\n", content_length);
}

int http_encode_header_connection(char *buf, size_t len, bool keep_alive)
{
        static const char *connection_str[] = {
                "close",
                "keep-alive"
        };

        return snprintf(buf, len, "Connection: %s\r\n",
                        keep_alive ? connection_str[1] : connection_str[0]);
}

int http_encode_header_content_type(char *buf,
				    size_t len,
				    http_content_type_t type)
{
	char *content_type_str;

	switch (type) {
	case HTTP_CONTENT_TYPE_TEXT_HTML:
		content_type_str = "text/html";
		break;
	case HTTP_CONTENT_TYPE_APPLICATION_JSON:
		content_type_str = "application/json";
		break;
	case HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA:
		content_type_str = "multipart/form-data";
		break;
	case HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM:
		content_type_str = "application/octet-stream";
		break;
	case HTTP_CONTENT_TYPE_TEXT_PLAIN:
	default:
		content_type_str = "text/plain";
		break;
	}

	const char *parts[] = {
		"Content-Type: ",
		content_type_str,
		"\r\n"
	};

	return mem_append_strings(buf, len, parts, ARRAY_SIZE(parts));
}

int http_encode_header_end(char *buf, size_t len)
{
        return snprintf(buf, len, "\r\n");
}

bool http_code_has_payload(uint16_t status_code)
{
        return (status_code == 200);
}

const char *http_content_type_to_str(http_content_type_t content_type)
{
	switch (content_type) {
	case HTTP_CONTENT_TYPE_NONE:
		return "{notset}";
	case HTTP_CONTENT_TYPE_TEXT_HTML:
		return "text/html";
	case HTTP_CONTENT_TYPE_APPLICATION_JSON:
		return "application/json";
	case HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM:
		return "application/octet-stream";
	case HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA:
		return "multipart/form-data";
	case HTTP_CONTENT_TYPE_TEXT_PLAIN:
	default:
		return "text/plain";
	}
}