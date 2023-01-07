/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file emu.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Emulate devices data source
 * @version 0.1
 * @date 2022-08-09
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _HA_EMU_H_
#define _HA_EMU_H_

#include <zephyr/kernel.h>

#include <caniot/caniot.h>

extern struct k_msgq emu_caniot_rxq;

int emu_caniot_send(struct caniot_frame *f);

#endif /* _HA_EMU_H_ */