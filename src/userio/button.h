/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USERIO_BUTTON_H_
#define _USERIO_BUTTON_H_

int button_init(void (*callback)(void));;

int button_register_callback(void (*callback)(void));

#endif /* _USERIO_BUTTON_H_ */