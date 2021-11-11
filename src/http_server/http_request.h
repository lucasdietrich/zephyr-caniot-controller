#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <stdio.h>
#include <stdint.h>

struct http_request
{
        char url[32];

        struct {
                char user[16];
                char password[16];
        };

        const char *payload;
        size_t len;
};

struct http_response
{
        char *buf;
        size_t len;
        uint16_t status_code;
};

typedef int (*http_handler_t)(struct http_request *request,
                             struct http_response *response);

struct http_ressource
{
        const char *url;
        http_handler_t handler;
};

#endif