/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DFU_H_
#define _DFU_H_

#include <zephyr/dfu/mcuboot.h>

int dfu_image_read_header(struct mcuboot_img_header *header);

void dfu_image_check(void);

#endif /* _DFU_H_ */