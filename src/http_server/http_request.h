#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <stdio.h>
#include <stdint.h>

#include <net/http_parser.h>

struct http_request
{
        enum http_method method;

        char url[32];
        size_t url_len;

        struct {
                char user[16];
                char password[16];
        };

        struct {
                char* buf;
                size_t size;
        } buffer;

        size_t len;

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