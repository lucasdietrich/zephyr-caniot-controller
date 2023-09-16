/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_FMT_H_
#define _UTILS_FMT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum fmt_encoding_type {
	FMT_ENC_INT32,
	FMT_ENC_UINT32,
	FMT_ENC_FLOAT,
	FMT_ENC_FLOAT_DIGITS,
	FMT_ENC_EXP,
	FMT_ENC_EXP_DIGITS,
};
typedef enum fmt_encoding_type fmt_enc_type_t;

enum  fmt_unit {
    FMT_UNIT_NONE,
    FMT_UNIT_HZ,
    FMT_UNIT_KHZ,
    FMT_UNIT_MHZ,
    FMT_UNIT_GHZ,
    FMT_UNIT_PERCENT,
    FMT_UNIT_CELSIUS,
    FMT_UNIT_MILLIAMPS,
    FMT_UNIT_MILLIWATTS,
    FMT_UNIT_MILLIVOLTS,
    FMT_UNIT_DBM,
};

typedef enum fmt_unit fmt_unit_t;

struct fmt_encoding_config {
	fmt_enc_type_t type;
    fmt_unit_t unit;

	union {
		/* For FMT_ENC_FLOAT_DIGITS
		 * and FMT_ENC_EXP_DIGITS */
		uint8_t digits;
	};
};
typedef struct fmt_encoding_config fmt_enc_cfg_t;

#define FMT_INT32  ((fmt_enc_cfg_t){.type = FMT_ENC_INT32})
#define FMT_UINT32 ((fmt_enc_cfg_t){.type = FMT_ENC_UINT32})
#define FMT_FLOAT  ((fmt_enc_cfg_t){.type = FMT_ENC_FLOAT})
#define FMT_FLOAT_DIGITS(_digits)                                                        \
	((fmt_enc_cfg_t){.type = FMT_ENC_FLOAT_DIGITS, .digits = _digits})
#define FMT_EXP ((fmt_enc_cfg_t){.type = FMT_ENC_EXP})
#define FMT_EXP_DIGITS(_digits)                                                          \
	((fmt_enc_cfg_t){.type = FMT_ENC_EXP_DIGITS, .digits = _digits})

/* A value to be formatted */
union fmt_value {
	float fvalue;	 /* store value as float */
	uint32_t uvalue; /* store value as unsigned */
	int32_t svalue;	 /* store value as signed */
};
typedef union fmt_value fmt_val_t;

ssize_t
fmt_encode_value(char *buf, size_t buf_size, fmt_val_t *value, const fmt_enc_cfg_t *cfg);

#endif /* _UTILS_FMT_H_ */