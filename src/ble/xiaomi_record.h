#ifndef _BLE_XIAOMI_RECORD_H_
#define _BLE_XIAOMI_RECORD_H_

#include <stdint.h>

#include <bluetooth/addr.h>

typedef struct {
	/**
	 * @brief Device measured temperature, base unit : 1e-2 °C
	 */
	int16_t temperature;

	/**
	 * @brief Device measured humidity, base unit : 1 %
	 */
	uint8_t humidity; /* 1 % */

	/**
	 * @brief Device measured battery level, base unit: 1 mV
	 */
	uint16_t battery;
}  __attribute__((packed)) xiaomi_measurements_t;

typedef struct {
	/**
	 * @brief Record device address
	 */
	bt_addr_le_t addr;

	/**
	 * @brief Uptime when the measurements were retrieved
	 */
	uint32_t uptime;
	
	/**
	 * @brief Measurements
	 */
	xiaomi_measurements_t measurements;
} __attribute__((packed)) xiaomi_record_t;

/**
 * @brief size = 8 + n * (7 + 9)
 * Where n is the number of devices (here, n = 15, size = 248)
 */
typedef struct {
	/**
	 * @brief Frame time
	 */
	uint32_t frame_time;

	/**
	 * @brief Records count
	 */
	uint32_t count;

	/**
	 * @brief List of records
	 */
	xiaomi_record_t records[15];
} xiaomi_dataframe_t;

#endif /* _BLE_XIAOMI_RECORD_H_ */