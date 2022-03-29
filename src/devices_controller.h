#ifndef _DEVICES_CONTROLLER_H_
#define _DEVICES_CONTROLLER_H_

#include <stddef.h>

#include "ble/xiaomi_record.h"

// xiaomi_record_t *dev_ble_xiaomi_get_first_record(void);

// xiaomi_record_t *dev_ble_xiaomi_get_next_record(xiaomi_record_t *cur);

// xiaomi_record_t *dev_ble_xiaomi_get_record(uint32_t index);

size_t dev_ble_xiaomi_iterate(void (*callback)(xiaomi_record_t *rec,
					       void *user_data),
			      void *user_data);

#endif /* _DEVICES_CONTROLLER_H_ */