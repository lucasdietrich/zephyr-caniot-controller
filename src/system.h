#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <stdint.h>

struct {
        uint32_t has_ipv4_addr: 1;
        uint32_t valid_system_time: 1;
} status;

#endif