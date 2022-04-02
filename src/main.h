#ifndef _MAIN_H_
#define _MAIN_H_

typedef struct {
	float die_temperature;
	uint32_t timestamp;
	struct k_sem sem;
	struct k_mutex mutex;
} die_temperature_handle_t;

#endif /* _MAIN_H_ */