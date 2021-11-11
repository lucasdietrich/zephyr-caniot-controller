#include "http_utils.h"

#include <kernel.h>

#include <stdio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(http_utils, LOG_LEVEL_DBG);

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

const char *get_status_code_str(uint16_t status_code)
{
        for (const struct code_str *p = status;
             p <= &status[ARRAY_SIZE(status) - 1]; p++) {
                if (p->code == status_code) {
                        return p->str;
                }
        }
        return NULL;
}

int encode_status_code(char *buf, uint16_t status_code)
{
        const char *code_str = get_status_code_str(status_code);
        if (code_str == NULL) {
                LOG_ERR("unknown status_code %d", status_code);
                return -1;
        }

        return sprintf(buf, "HTTP/1.1 %d %s\r\nConnection: close\r\n\r\n", 
                       status_code, code_str);
}
