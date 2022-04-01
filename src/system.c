#include "system.h"

#include <device.h>
#include <drivers/sensor.h>


controller_status_t controller_status = {
    .has_ipv4_addr = 0,
    .valid_system_time = 0,
};