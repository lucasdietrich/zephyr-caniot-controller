/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ha_devs.h
 * @brief
 * @version 0.1
 * @date 2022-03-31
 
 *
 * TODO: shorten "ha_dev" to "mydev" or "myd"
 *
 */

#ifndef _HA_DEVS_H_
#define _HA_DEVS_H_

#include <zephyr.h>

#include "data.h"

struct ha_dev_stats_t
{
	uint32_t rx; /* number of received packets */
	uint32_t rx_bytes; /* number of received bytes */

	uint32_t tx; /* number of transmitted packets */
	uint32_t tx_bytes; /* number of transmitted bytes */

	uint32_t max_inactivity; /* number of seconds without any activity */
};

typedef struct {
	/* Addr which uniquely identifies the device */
	ha_dev_addr_t addr;

	/* UNIX timestamps in seconds */
	uint32_t registered_timestamp;

	/* Device API */
	void *api;

	/* Device statistics */
	struct ha_dev_stats_t stats;

	/* Device data */

	/* TODO Dynamically allocate the dataset and reference the pointer here */
	struct {
		uint32_t measurements_timestamp;

		union {
			struct ha_xiaomi_dataset xiaomi;
			struct ha_caniot_blt_dataset caniot;
			struct ha_f429zi_dataset nucleo_f429zi;
		};
	} data;
	
} ha_dev_t;

bool ha_dev_valid(ha_dev_t *const dev);

typedef void ha_dev_iterate_cb_t(ha_dev_t *dev,
				 void *user_data);

int ha_dev_addr_cmp(const ha_dev_addr_t *a,
		    const ha_dev_addr_t *b);

size_t ha_dev_iterate(ha_dev_iterate_cb_t callback,
		      ha_dev_filter_t *filter,
		      void *user_data);

size_t ha_dev_iterate_filter_by_type(ha_dev_iterate_cb_t callback,
				  void *user_data,
				  ha_dev_type_t type);

static inline size_t ha_dev_xiaomi_iterate(ha_dev_iterate_cb_t callback,
					   void *user_data)
{
	return ha_dev_iterate_filter_by_type(callback,
					  user_data,
					  HA_DEV_TYPE_XIAOMI_MIJIA);
}


static inline size_t ha_dev_caniot_iterate(ha_dev_iterate_cb_t callback,
					   void *user_data)
{
	return ha_dev_iterate_filter_by_type(callback,
					     user_data,
					     HA_DEV_TYPE_CANIOT);
}

static inline void ha_dev_inc_stats_rx(ha_dev_t *dev, uint32_t rx_bytes)
{
	__ASSERT(dev != NULL, "dev is NULL");

	dev->stats.rx_bytes += rx_bytes;
	dev->stats.rx++;
}

static inline void ha_dev_inc_stats_tx(ha_dev_t *dev, uint32_t tx_bytes)
{
	__ASSERT(dev != NULL, "dev is NULL");

	dev->stats.tx_bytes += tx_bytes;
	dev->stats.tx++;
}

int ha_register_xiaomi_from_dataframe(xiaomi_dataframe_t *frame);

int ha_dev_register_die_temperature(uint32_t timestamp,
				    float die_temperature);

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     caniot_did_t did,
				     struct caniot_board_control_telemetry *data);

/*____________________________________________________________________________*/

/* move to specific header */
struct ha_dev_garage_cmd
{
	uint8_t actuate_left: 1;
	uint8_t actuate_right: 1;
};

void ha_dev_garage_cmd_init(struct ha_dev_garage_cmd *cmd);

int ha_dev_garage_cmd_send(const struct ha_dev_garage_cmd *cmd);

#endif /* _HA_DEVS_H_ */