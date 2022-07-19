#ifndef _HA_UTILS_H
#define _HA_UTILS_H

#include <stdint.h>
#include <caniot/caniot.h>
#include <drivers/can.h>

int zcan_to_caniot(const struct zcan_frame *zcan,
		   struct caniot_frame *caniot);


int caniot_to_zcan(struct zcan_frame *zcan,
		   const struct caniot_frame *caniot);

int ha_parse_ss_command(const char *str);

int ha_parse_xps_command(const char *str);

#endif /* _HA_UTILS_H */