/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief API for asynchronous file read/write
 */

#ifndef _APP_FS_ASYNCRW_H_
#define _APP_FS_ASYNCRW_H_

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
typedef enum {
	/* Read from file */
	FS_ASYNC_READ = 1 << 0,

	/* Write into file */
	FS_ASYNC_WRITE = 1 << 1,

	/* Append to file */
	FS_ASYNC_APPEND = 1 << 2,

	/* Read file size before opening */
	FS_ASYNC_READ_SIZE = 1 << 3,

	/* Create file if not exist */
	FS_ASYNC_CREATE = 1 << 4,

	/* Truncate file if exist */
	FS_ASYNC_TRUNCATE = 1 << 5,
} fs_async_option_t;

struct fs_async_buf {
	sys_snode_t _handle;
	ssize_t len;
	uint16_t offset;
	char data[];
};

struct fs_async_config {
	const char *file_path;
	fs_async_option_t opt;

	char *ms_buf;		  /* Must be aligned to 4 bytes */
	size_t ms_block_size; /* Must be multiple of 4 */
	size_t ms_block_count;
};

typedef enum {
	FS_ASYNC_STATUS_UNDEFINED = 0,
	FS_ASYNC_STATUS_ACTIVE,
	FS_ASYNC_STATUS_EOF,
	FS_ASYNC_STATUS_CLOSED,
	FS_ASYNC_STATUS_ERR,
} fs_async_status_t;

struct fs_async {
	/* For FIFO */
	void *_handle; /* Must be aligned to 4 bytes */

	/* State */
	fs_async_status_t status;
	fs_async_option_t opt;
	sys_slist_t buf_q;
	ssize_t file_size;
	size_t file_final_size;

	/* Internal state */
	atomic_t _flags;
	struct fs_file_t _zfp;
	struct k_mem_slab _ms;
	struct k_sem _sem;
};

/* API */

/**
 * @brief Initialize async context and open file with given configuration
 *
 * @param afile
 * @param cfg Configuration
 * @return int
 */
int fs_async_open(struct fs_async *afile, struct fs_async_config *cfg);

/**
 * @brief Read data from a file asynchronously, this function will block until
 *       data is available (depending on given requested length) or timeout
 *
 * Note: If timeout is K_FOREVER, this function will block until requested data
 *      length is available.
 *
 * Note: If return value is different from requested length when timeout is
 *     K_FOREVER, it means that EOF is reached.
 *
 * Note: If return value is 0, it means that EOF is reached.
 *
 * @param afile Async file context
 * @param data Buffer to store data
 * @param len Length of data to read
 * @param timeout Timeout
 * - K_NO_WAIT: Do not block, return immediately
 * - K_FOREVER: Block until requested data len is available
 * - K_MSEC(x): Block until requested data len is available or timeout
 * @return int Return number of bytes read on success, negative value on error
 */
int fs_async_read(struct fs_async *afile, void *data, size_t len, k_timeout_t timeout);

/**
 * @brief Write data to a file asynchronously, this function will block until
 *      data is written (depending on given requested length) or timeout
 *
 * Note: If timeout is K_FOREVER, this function will block until requested data
 *     length is written.
 *
 * @param afile Async file context
 * @param data Data to write
 * @param len Length of data to write
 * @param timeout Timeout
 * @return int Return number of bytes written on success, negative value on error
 */
int fs_async_write(struct fs_async *afile, void *data, size_t len, k_timeout_t timeout);

/**
 * @brief Close file
 *
 * @param afile Async file context
 * @return int
 */
int fs_async_close(struct fs_async *afile);

/**
 * @brief Get file size after file has been opened with FS_ASYNC_READ_SIZE option
 *
 * @param afile Async file context
 * @return int
 */
int fs_async_get_file_size(struct fs_async *afile);

/**
 * @brief Return true if EOF is reached for given file
 *
 * @param afile
 * @return int
 */
bool fs_async_is_eof(struct fs_async *afile);

#endif /* _APP_FS_ASYNCRW_H_ */