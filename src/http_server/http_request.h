#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <stdio.h>
#include <stdint.h>

#include <net/http_parser.h>

struct http_request
{
	/**
	 * @brief Request method (GET, POST, PUT, DELETE)
	 */
        enum http_method method;

	/* parsed url */
        char url[64];
        size_t url_len;

	/* parsed authentification */
        struct {
                char user[16];
                char password[16];
        };

        struct {
                char* buf;
                size_t size;
        } buffer;

        size_t len;

	/* HTTP content location */
        struct {
                const char *loc;
                size_t len;
        } payload;
};

struct http_response
{
        char *buf;
        size_t buf_size;
        size_t content_len;
        uint16_t status_code;
	enum {
		HTTP_CONTENT_TYPE_TEXT_PLAIN = 0,
		HTTP_CONTENT_TYPE_APPLICATION_JSON,
	} content_type;
};

/* unused */
typedef int (*http_handler_t)(struct http_request *request,
                             struct http_response *response);

/* unused */
struct http_ressource
{
        const char *url;
        http_handler_t handler;
};

#endif