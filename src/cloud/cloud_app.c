#include <zephyr.h>

#include <data/json.h>

#include "cloud.h"

#include "ha/devices.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cloud_app, LOG_LEVEL_DBG);

static struct ha_ev_subs *sub;


static const struct json_obj_descr xiaomi_record[] = {
	// JSON_OBJ_DESCR_PRIM(struct ha_ev_xiaomi, state, JSON_TOK_STRING),
	// JSON_OBJ_DESCR_PRIM(struct ha_ev_xiaomi, battery, JSON_TOK_NUMBER),
	// JSON_OBJ_DESCR_PRIM(struct ha_ev_xiaomi, voltage, JSON_TOK_NUMBER),
};

void cloud_on_queued(struct ha_ev_subs *sub, ha_ev_t *event)
{
	ARG_UNUSED(sub);
	ARG_UNUSED(event);

	cloud_notify(0u);
}

int cloud_app_init(void)
{
	const struct ha_ev_subs_conf conf = {
		.flags = HA_EV_SUBS_DEVICE_DATA | HA_EV_SUBS_ON_QUEUED_HOOK,
		.on_queued = cloud_on_queued
	};

	return ha_ev_subscribe(&conf, &sub);
}

int cloud_app_process(atomic_val_t flags)
{
	ha_ev_t *ev;

	while ((ev = ha_ev_wait(sub, K_NO_WAIT)) != NULL) {
		LOG_DBG("Got event %p", ev);

		struct ha_xiaomi_dataset *data = (struct ha_xiaomi_dataset *)ev->data;

		ha_ev_unref(ev);
	}

	return 0;
}

int cloud_app_cleanup(void)
{
	return ha_ev_unsubscribe(sub);
}
