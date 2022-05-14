#ifndef _BLE_XIAOMI_RECORD_H_
#define _BLE_XIAOMI_RECORD_H_

#include <stdint.h>

#include <bluetooth/addr.h>

/* size is 7B */
typedef struct {
	/**
	 * @brief RSSI
	 */
	int8_t rssi;
	/**
	 * @brief Device measured temperature, base unit : 1e-2 °C
	 */
	int16_t temperature;

	/**
	 * @brief Device measured humidity, base unit : 1e-2 %
	 */
	uint16_t humidity; /* 1e-2 % */

	/**
	 * @brief Device measured battery voltage, base unit: 1 mV
	 */
	uint16_t battery_mv;

	/**
	 * @brief Device measured battery level, base unit:  %
	 * Measurement is valid if battery_level > 0
	 */
	uint8_t battery_level;
}  __attribute__((packed)) xiaomi_measurements_t;

typedef struct {
	/**
	 * @brief Record device address
	 */
	bt_addr_le_t addr;

	/**
	 * @brief Time when the measurements were retrieved
	 */
	uint32_t time;
	
	/**
	 * @brief Measurements
	 */
	xiaomi_measurements_t measurements;
} __attribute__((packed)) xiaomi_record_t;

/**
 * @brief size = 8 + n * (7 + 4 + 8)
 * Where n is the number of devices (here, n = 14, size = 248)
 */
typedef struct {
	/**
	 * @brief Frame time
	 */
	uint32_t time;

	/**
	 * @brief Records count
	 */
	uint32_t count;

	/**
	 * @brief List of records
	 */
	xiaomi_record_t records[13];
} xiaomi_dataframe_t;

#endif /* _BLE_XIAOMI_RECORD_H_ */