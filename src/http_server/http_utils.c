#include <stdio.h>
#include <kernel.h>

#include "http_utils.h"

#include "utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_utils, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

struct code_str
{
        uint16_t code;
        char *str;
};

static const struct code_str status[] = {
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {204, "No Content"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {404, "Not Found"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"}
};

static const char *get_status_code_str(uint16_t status_code)
{
        for (const struct code_str *p = status;
             p <= &status[ARRAY_SIZE(status) - 1]; p++) {
                if (p->code == status_code) {
                        return p->str;
                }
        }
        return NULL;
}

int http_encode_status(char *buf, size_t len, uint16_t status_code)
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