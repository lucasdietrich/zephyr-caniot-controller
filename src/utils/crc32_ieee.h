/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CRC32_IEEE_H_
#define _CRC32_IEEE_H_

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_ieee_u8(uint8_t *buf, size_t len);

uint32_t crc32_ieee_u32(uint32_t *buf, size_t len);

#endif /* _CRC32_IEEE_H_ */