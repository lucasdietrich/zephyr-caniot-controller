#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>

#include <net/net_ip.h>
#include <drivers/can.h>

#include <sys/types.h>

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

int get_repr_can_frame(struct zcan_frame *frame, char *buf, size_t len);

typedef struct {
	void *data;
	size_t size;
	size_t filling;
} buffer_t;

int buffer_init(buffer_t *buffer, void *data, size_t size);

int buffer_reset(buffer_t *buffer);

int buffer_append(buffer_t *buffer, void *data, size_t size);

int buffer_append_string(buffer_t *buffer, const char *string);

int buffer_append_strings(buffer_t *buffer, const char **strings, size_t count);

#endif /* _UTILS_H_ */