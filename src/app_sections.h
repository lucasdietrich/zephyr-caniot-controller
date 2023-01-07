/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_SECTIONS_H_
#define _APP_SECTIONS_H_

#include <zephyr/kernel.h>

/* only in ARM CORTEX context */
#if defined(CONFIG_APP_BIG_BUFFER_TO_CCM)
#define __buf_data_section   __ccm_data_section
#define __buf_bss_section    __ccm_bss_section
#define __buf_noinit_section __ccm_noinit_section
#else
#define __buf_data_section
#define __buf_bss_section
#define __buf_noinit_section __noinit
#endif

#endif /* _APP_CONFIG_H_ */