/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BLE_BLE_H_
#define _BLE_BLE_H_

#include <zephyr/kernel.h>

int ble_init(void);

int ble_observer_start(void);

#endif /* _BLE_BLE_H_ */