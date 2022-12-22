/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "subs_extended.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(subs_ext, LOG_LEVEL_DBG);

typedef bool (*lt_cmp_func_t)(ha_subs_ext_lte_t *lte, ha_ev_t *event);

static bool lt_cmd_any(ha_subs_ext_lte_t *lte, ha_ev_t *event)
{
	return true;
}

static bool lt_cmd_sdevuid(ha_subs_ext_lte_t *lte, ha_ev_t *event)
{
	return (lte->sdevuid == event->dev->sdevuid);
}

static bool lt_cmd_sdevuid_endpoint(ha_subs_ext_lte_t *lte, ha_ev_t *event)
{
	/* TODO */
	return false;
}

static bool lt_cmd_endpoint(ha_subs_ext_lte_t *lte, ha_ev_t *event)
{
	/* TODO */
	return false;
}

static lt_cmp_func_t lte_cmp_func_get(ha_subs_ext_lookup_type_t lt_type)
{
	switch (lt_type) {
	case HA_SUBS_EXT_LOOKUP_TYPE_ANY:
		return lt_cmd_any;
	case HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID:
		return lt_cmd_sdevuid;
	case HA_SUBS_EXT_LOOKUP_TYPE_ENDPOINTID:
		return lt_cmd_endpoint;
	case HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID_ENDPOINT:
		return lt_cmd_sdevuid_endpoint;
	case HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR_ENDPOINT:
	case HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR:
	default:
		return NULL;
	}
}

static ha_subs_ext_lte_t *lt_find(ha_subs_ext_lt_t *lt, ha_ev_t *event)
{
	const lt_cmp_func_t cmp_func = lte_cmp_func_get(lt->lookup_type);

	if (cmp_func != NULL) {
		ha_subs_ext_lte_t *lte;
		SYS_SLIST_FOR_EACH_CONTAINER(&lt->_list, lte, _node)
		{
			if (cmp_func(lte, event) == true) {
				LOG_DBG("Event %p found in LT %p -> %p", 
					event, lt, lte);
				return lte;
			}
		}
	}

	LOG_DBG("Event %p not found in LT %p", event, lt);

	return NULL;
}

static ha_subs_ext_lte_t *lt_find_otherwise_allocate(ha_subs_ext_lt_t *lt,
						     ha_ev_t *event,
						     bool *created)
{
	bool zcreated = false;
	ha_subs_ext_lte_t *lte = lt_find(lt, event);

	if (lte == NULL) {
		lte = k_malloc(sizeof(ha_subs_ext_lte_t));
		if (lte) {
			sys_slist_append(&lt->_list, &lte->_node);
			lte->sdevuid = event->dev->sdevuid;

			zcreated = true;
		} else {
			LOG_ERR("Failed to allocate memory for new LT %p entry", lt);
		}
	}

	if (created)
		*created = zcreated;

	return lte;
}

static bool lt_update_cb(struct ha_ev_subs *sub,
				      ha_ev_t *event)
{
	ha_subs_ext_lt_t *const lt = (ha_subs_ext_lt_t *)sub->conf->user_data;
	lt_find_otherwise_allocate(lt, event, NULL);

	return true;
}

static bool lt_filter_duplicate_cb(struct ha_ev_subs *sub,
				      ha_ev_t *event)
{
	bool created;
	ha_subs_ext_lt_t *const lt = (ha_subs_ext_lt_t *)sub->conf->user_data;
	lt_find_otherwise_allocate(lt, event, &created);

	return created;
}

static bool lt_filter_count_cb(struct ha_ev_subs *sub,
				  ha_ev_t *event)
{
	bool created;
	ha_subs_ext_lt_t *const lt = (ha_subs_ext_lt_t *)sub->conf->user_data;
	ha_subs_ext_lte_t *lte = lt_find_otherwise_allocate(lt, event, &created);

	if (lte == NULL) {
		return false;
	} else {
		return ++lte->param_value.count <= lt->filtering_param.max_count;
	}
}

static bool lt_filter_interval_cb(struct ha_ev_subs *sub,
				     ha_ev_t *event)
{
	bool created;
	ha_subs_ext_lt_t *const lt = (ha_subs_ext_lt_t *)sub->conf->user_data;
	ha_subs_ext_lte_t *lte = lt_find_otherwise_allocate(lt, event, &created);

	const uint32_t interval = lt->filtering_param.interval;

	if (lte == NULL) {
		return false;
	} else if (created) {
		lte->param_value.timestamp = event->timestamp;
		return true;
	} else {
		if (event->timestamp - lte->param_value.timestamp >= interval) {
			lte->param_value.timestamp = event->timestamp;
			return true;
		} else {
			return false;
		}
	}
}

static bool lt_filter_subsampling_cb(struct ha_ev_subs *sub,
					ha_ev_t *event)
{
	bool created;
	ha_subs_ext_lt_t *const lt = (ha_subs_ext_lt_t *)sub->conf->user_data;
	ha_subs_ext_lte_t *lte = lt_find_otherwise_allocate(lt, event, &created);

	const uint32_t subsampling = lt->filtering_param.subsampling;

	if (lte == NULL) {
		return false;
	} else {
		if (lte->param_value.mod == subsampling) {
			lte->param_value.mod = 0u;
		}

		return lte->param_value.mod++ == 0u;
	}
}

int ha_subs_ext_conf_set(struct ha_ev_subs_conf *conf,
			 struct ha_subs_ext_lookup_table *lookup_table,
			 ha_subs_ext_lookup_type_t lookup_type,
			 ha_subs_ext_filtering_type_t filtering_type,
			 ha_subs_ext_filtering_param_t filtering_param)
{
	if (!conf || !lookup_table)
		return -EINVAL;

	if (conf->user_data)
		return -EINVAL; /* user_data already set */

	if (conf->flags & HA_EV_SUBS_CONF_FILTER_FUNCTION)
		return -EINVAL;

	/* Check if lookup type is supported */
	switch (lookup_type) {
	case HA_SUBS_EXT_LOOKUP_TYPE_ANY:
	case HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID:
		break;
	case HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID_ENDPOINT:
	case HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR:
	case HA_SUBS_EXT_LOOKUP_TYPE_DEVADDR_ENDPOINT:
	case HA_SUBS_EXT_LOOKUP_TYPE_ENDPOINTID:
		return -ENOTSUP; /* TODO: not supported yet */
	default:
		return -ENOTSUP;
	}

	switch (filtering_type) {
	case HA_SUBS_EXT_FILTERING_TYPE_NONE:
		conf->filter_cb = lt_update_cb;
		break;
	case HA_SUBS_EXT_FILTERING_TYPE_DUPLICATE:
		conf->filter_cb = lt_filter_duplicate_cb;
		break;
	case HA_SUBS_EXT_FILTERING_TYPE_COUNT:
		conf->filter_cb = lt_filter_count_cb;
		break;
	case HA_SUBS_EXT_FILTERING_TYPE_INTERVAL:
		conf->filter_cb = lt_filter_interval_cb;
		break;
	case HA_SUBS_EXT_FILTERING_TYPE_SUBSAMPLING:
		conf->filter_cb = lt_filter_subsampling_cb;
		break;
	case HA_SUBS_EXT_FILTERING_TYPE_INTERVAL_MS:
	default:
		return -ENOTSUP;
	}

	conf->flags |= HA_EV_SUBS_CONF_FILTER_FUNCTION;

	/* Initialize lookup table */
	lookup_table->filtering_type = filtering_type;
	lookup_table->filtering_param = filtering_param;
	lookup_table->lookup_type = lookup_type;
	sys_slist_init(&lookup_table->_list);

	/* Set extended filter context */
	conf->user_data = (void *)lookup_table;

	return 0;
}

int ha_subs_ext_lt_clear(struct ha_subs_ext_lookup_table *lt)
{
	if (!lt)
		return -EINVAL;

	sys_snode_t *node;

	while ((node = sys_slist_get(&lt->_list)) != NULL) {
		k_free(node);
	}

	return 0;
}