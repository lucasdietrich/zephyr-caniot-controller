#include "utils.h"


int zcan_to_caniot(const struct zcan_frame *zcan,
		   struct caniot_frame *caniot)
{
	if ((zcan == NULL) || (caniot == NULL)) {
		return -EINVAL;
	}

	caniot_clear_frame(caniot);
	caniot->id = caniot_canid_to_id((uint16_t)zcan->id);
	caniot->len = MIN(zcan->dlc, 8U);
	memcpy(caniot->buf, zcan->data, caniot->len);

	return 0U;
}

// static
int caniot_to_zcan(struct zcan_frame *zcan,
		   const struct caniot_frame *caniot)
{
	if ((zcan == NULL) || (caniot == NULL)) {
		return -EINVAL;
	}

	memset(zcan, 0x00U, sizeof(struct zcan_frame));

	zcan->id = caniot_id_to_canid(caniot->id);
	zcan->dlc = MIN(caniot->len, 8U);
	memcpy(zcan->data, caniot->buf, zcan->dlc);

	return 0U;
}