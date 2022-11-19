/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_MISC_H_
#define _UTILS_MISC_H_

#include <zephyr/kernel.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/net/net_ip.h>
#include <sys/types.h>

#define CHECK_OR_EXIT(cond) \
	if (!(cond)) { \
		goto exit; \
	}

static inline uint32_t k_uptime_delta32(uint32_t *reftime)
{
	__ASSERT_NO_MSG(reftime);

	uint32_t uptime, delta;

	uptime = k_uptime_get_32();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}


/**
 * @brief Append memory to a buffer
 * 
 * @param dst buffer to store the result
 * @param dst_len buffer length
 * @param src source buffer
 * @param src_len source buffer length
 * @return ssize_t 
 */
ssize_t mem_append(void *dst,
		   size_t dst_len,
		   const void *src,
		   size_t src_len);

/**
 * @brief Append a string to a buffer
 * 
 * @param dst buffer to store the result
 * @param dst_len buffer length
 * @param string String to append
 * @return ssize_t 
 */
ssize_t mem_append_string(void *dst,
			  size_t dst_len,
			  const char *string);

/**
 * @brief Append and concatenate strings to a buffer
 * 
 * @param dst buffer to store the result
 * @param dst_len buffer length
 * @param strings Strings to concatenate, all strings should be fined 
 *  		   (i.e. not null) and must be null-terminated)
 * @param count Number of strings to concatenate
 * @return ssize_t 
 */
ssize_t mem_append_strings(void *dst,
			   size_t dst_len,
			   const char **strings,
			   size_t count);

int ipv4_to_str(struct in_addr *addr, char *buffer, size_t len);

int strcicmp(char const *a, char const *b);

int strncicmp(char const *a, char const *b, size_t len);

/* Convert string to lowercase */
void str_tolower(char *str);

int get_repr_can_frame(struct can_frame *frame, char *buf, size_t len);

#endif /* _UTILS_H_ */