/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_SUBS_EXTENDED_H_
#define _HA_SUBS_EXTENDED_H_

/* Extended support for event subscription */

#include <zephyr/sys/dlist.h>
#include <zephyr/sys/slist.h>

#include "ha.h"

typedef enum {
	/* No filtering, simply update lookup table */
	HA_SUBS_EXT_FILTERING_TYPE_NONE = 0u,

	/* Look for duplicate, drop the event if found */
	HA_SUBS_EXT_FILTERING_TYPE_DUPLICATE,

	/* Look for events count, if count is reached, drop the event */
	HA_SUBS_EXT_FILTERING_TYPE_COUNT,

	/* Look for minimum interval between events in seconds, drop the event 
	 * if interval is too short */
	 HA_SUBS_EXT_FILTERING_TYPE_INTERVAL,

	/* Look for minimum interval between events in milliseconds, drop the 
	 * event if interval is too short */
	 HA_SUBS_EXT_FILTERING_TYPE_INTERVAL_MS,

	 /* Look for subsampling of events, keep only a fraction of events,
	  * drop the rest */
	  HA_SUBS_EXT_FILTERING_TYPE_SUBSAMPLING,
} ha_subs_ext_filtering_type_t;

typedef enum {
	/* Match any event */
	HA_SUBS_EXT_LOOKUP_TYPE_ANY = 0u,

	/* Match by sdevuid only
	 * 
	 * Prefer the use of HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID over
	 * HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR because it is more efficient.
	 * 
	 * However if your devices could get removed and re-added, 
	 * HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR could be required to keep track of
	 * the device for the lifetime of the subscription.
	 */
	HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID,

	/* Match by sdevuid and endpoint index
	 * 
	 * Prefer the use of HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID_ENDPOINT over
	 * HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR_ENDPOINT because it is more efficient.
	 * 
	 * However if your devices could get removed and re-added, 
	 * HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR_ENDPOINT could be required to keep 
	 * track of the device for the lifetime of the subscription.
	 */
	HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID_ENDPOINT,

	/* Match by device address only */
	HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR,

	/* Match by device address and endpoint index */
	HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR_ENDPOINT,

	/* Match by device endpoint id */
	HA_SUBS_EXT_LOOKUP_TYPE_ENDPOINTID,
} ha_subs_ext_lookup_type_t;

typedef union {
	uint32_t any;

	uint32_t max_count;	/* HA_SUBS_EXT_FILTERING_TYPE_COUNT */
	uint32_t interval;	/* HA_SUBS_EXT_FILTERING_TYPE_INTERVAL */
	uint32_t interval_ms;	/* HA_SUBS_EXT_FILTERING_TYPE_INTERVAL_MS */
	uint32_t subsampling;	/* HA_SUBS_EXT_FILTERING_TYPE_SUBSAMPLING */
} ha_subs_ext_filtering_param_t;

#define HA_SUBS_EXT_FILTERING_PARAM_NONE \
	(ha_subs_ext_filtering_param_t) { .any = 0u }

#define HA_SUBS_EXT_FILTERING_PARAM_MAX_COUNT(_max_count) \
	(ha_subs_ext_filtering_param_t) { .max_count = _max_count }

#define HA_SUBS_EXT_FILTERING_PARAM_INTERVAL(_interval) \
	(ha_subs_ext_filtering_param_t) { .interval = _interval }

#define HA_SUBS_EXT_FILTERING_PARAM_INTERVAL_MS(_interval_ms) \
	(ha_subs_ext_filtering_param_t) { .interval_ms = _interval_ms }

#define HA_SUBS_EXT_FILTERING_PARAM_SUBSAMPLING(_subsampling) \
	(ha_subs_ext_filtering_param_t) { .subsampling = _subsampling }

typedef union {
	uint32_t any;		/* 0 if uninitialized */
	uint32_t found; 	/* HA_SUBS_EXT_FILTERING_TYPE_DUPLICATE */
	uint32_t count;		/* HA_SUBS_EXT_FILTERING_TYPE_COUNT */
	uint32_t timestamp;	/* HA_SUBS_EXT_FILTERING_TYPE_INTERVAL */
	uint32_t timestamp_ms;	/* HA_SUBS_EXT_FILTERING_TYPE_INTERVAL_MS */
	uint32_t mod;		/* HA_SUBS_EXT_FILTERING_TYPE_SUBSAMPLING */
} ha_subs_ext_lookup_param_value_t;

struct ha_subs_ext_lookup_table_entry {
	/* slist handle */
	sys_snode_t _node;

	/* Session Device Unique ID */
	uint16_t sdevuid;

	/* Endpoint index */
	uint16_t endpoint_index;

	// /* Device address */
	// ha_dev_addr_t devaddr;

	// /* Device endpoint id */
	// ha_endpoint_id_t endpointid;

	/* Attribute */
	ha_subs_ext_lookup_param_value_t param_value;
};
typedef struct ha_subs_ext_lookup_table_entry ha_subs_ext_lte_t;

struct ha_subs_ext_lookup_table {
	/* Lookup table list */
	sys_slist_t _list;

	/* Lookup table type */
	ha_subs_ext_lookup_type_t lookup_type;

	/* Filtering type */
	ha_subs_ext_filtering_type_t filtering_type;

	/* Filtering parameter */
	ha_subs_ext_filtering_param_t filtering_param;
};

typedef struct ha_subs_ext_lookup_table ha_subs_ext_lt_t;

/**
 * @brief Wrap the subscription configuration to add lookup table filtering.
 * This function requires the subscription configuration to be initialized
 * with ha_ev_subs_conf_init(). It can be already partially configured, but
 * the callback and user_data fields should not be set to enable this extended
 * support.
 * 
 * The filtering type allows to choose type of lookup table to use:
 * - HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID_ENDPOINT: match pair (sdevuid, endpoint)
 * - HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID: match (sdevuid) only
 *
 * @param conf Configuration to wrap
 * @param lookup_table Lookup table to use
 * @param lookup_type Lookup type
 * @param filtering_type Filtering type
 * @param filtering_param Parameter for the filtering type (ignored in some cases)
 * @return int
 */
int ha_subs_ext_conf_set(struct ha_ev_subs_conf *conf,
			 struct ha_subs_ext_lookup_table *lookup_table,
			 ha_subs_ext_lookup_type_t lookup_type,
			 ha_subs_ext_filtering_type_t filtering_type,
			 ha_subs_ext_filtering_param_t filtering_param);

/**
 * @brief Clear the lookup table and free the memory allocated for it.
 * 
 * Note: Should be called after unsubscription, only if ha_subs_ext_conf_set()
 * was called on the subscription configuration.
 * 
 * @param lookup_table Lookup table to clear
 * @return int 
 */
int ha_subs_ext_lt_clear(struct ha_subs_ext_lookup_table *lt);

/**
 * @brief Iterate over the lookup table entries.
 * 
 * @param lt Lookup table to iterate over
 * @param cb Callback to call for each entry
 * @param user_data User data to pass to the callback
 * @return int 
 */
int ha_subs_ext_lt_iterate(struct ha_subs_ext_lookup_table *lt,
			   int (*cb)(struct ha_subs_ext_lookup_table_entry *lte,
				     void *user_data),
			   void *user_data);

#endif /* _HA_SUBS_EXTENDED_H_ */