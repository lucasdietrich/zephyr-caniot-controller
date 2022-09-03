#include "manager.h"

#include <zephyr.h>

#include <memory.h>

#include "hardcoded_creds.h"

int harcoded_creds_init(void)
{
	return 0;
}

int harcoded_cred_get(cred_id_t id, struct cred *c)
{
	if (!c) {
		return -EINVAL;
	}

	const struct hardcoded_cred *hc;

	for (hc = creds_harcoded_array;
	     hc < creds_harcoded_array + CREDS_HARDCODED_MAX_COUNT; hc++) {
		if ((hc->id == id) && hc->len && hc->data) {
			c->data = hc->data;
			c->len = hc->len;
			return 0;
		}
	}

	return -ENOENT;
}

int harcoded_cred_copy_to(cred_id_t id, char *buf, size_t *size)
{
	struct cred c;

	if (!buf || !size) {
		return -EINVAL;
	}

	int ret = harcoded_cred_get(id, &c);
	if (ret == 0) {
		if (*size >= c.len) {
			memcpy(buf, c.data, c.len);
			*size = c.len;
			ret = c.len;
		} else {
			*size = 0;
			ret = -ENOMEM;
		}
	}
	return ret;
}