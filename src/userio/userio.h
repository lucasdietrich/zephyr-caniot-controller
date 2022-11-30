/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USERIO_H_
#define _USERIO_H_

#include <stdint.h>

#include "system.h"

/**
 * @brief Initialize user I/O module
 */
void userio_init(void);

/**
 * @brief Apply changes to outputs (LEDs) in function of the received event on 
 * the interface
 * 
 * @param iface 
 * @param ev 
 * @return int 
 */
int userio_iface_show_event(sysev_if_t iface, sysev_if_ev_t ev);

#endif /* _USERIO_H_ */