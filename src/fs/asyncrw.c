/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "asyncrw.h"
#include "fs/app_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fs_async, LOG_LEVEL_INF);

#define FS_ASYNC_ARGS_CHECK 1u

static void async_thread(void *_a, void *_b, void *_c);

// K_THREAD_DEFINE(async, 0x400, async_thread, NULL, NULL, NULL, K_PRIO_COOP(1), 0u, 0u);
K_THREAD_DEFINE(async, 0x400, async_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(1u), 0u, 0u);

K_FIFO_DEFINE(fifo);

#if defined(CONFIG_APP_FS_ASYNC_READ)
static void process_read(struct fs_async *afile);
#endif
#if defined(CONFIG_APP_FS_ASYNC_WRITE)
static void process_write(struct fs_async *afile);
#endif

#define FS_ASYNC_FLAG_ACTIVE_BIT 0u
#define FS_ASYNC_FLAG_ACTIVE	 BIT(FS_ASYNC_FLAG_ACTIVE_BIT)
#define FS_ASYNC_FLAG_ABORT_BIT	 1u
#define FS_ASYNC_FLAG_ABORT		 BIT(FS_ASYNC_FLAG_ABORT_BIT)

static struct k_spinlock fs_async_lock;

static void async_thread(void *_a, void *_b, void *_c)
{
	struct fs_async *afile;

	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	for (;;) {
		afile = (struct fs_async *)k_fifo_get(&fifo, K_FOREVER);

		if (afile) {
			switch (afile->opt & (FS_ASYNC_READ | FS_ASYNC_WRITE)) {
#if defined(CONFIG_APP_FS_ASYNC_READ)
			case FS_ASYNC_READ:
				process_read(afile);
				break;
#endif
#if defined(CONFIG_APP_FS_ASYNC_WRITE)
			case FS_ASYNC_WRITE:
				process_write(afile);
				break;
#endif
			default:
				break;
			}
		}
	}
}

static void schedule_rw(struct fs_async *afile)
{
	if (atomic_cas(&afile->_flags, 0u, FS_ASYNC_FLAG_ACTIVE)) {
		k_fifo_put(&fifo, afile);
	}
}

static size_t buf_get_max_pl_len(struct fs_async *afile)
{
	return afile->_ms.block_size - sizeof(struct fs_async_buf);
}

static bool buf_is_full(struct fs_async *afile, struct fs_async_buf *buf)
{
	return buf->len + buf->offset == buf_get_max_pl_len(afile);
}

int buf_alloc(struct fs_async *afile, struct fs_async_buf **pbuf)
{
	int ret;

	ret = k_mem_slab_alloc(&afile->_ms, (void **)pbuf, K_NO_WAIT);
	if (ret == 0) {
		(*pbuf)->len	= buf_get_max_pl_len(afile);
		(*pbuf)->offset = 0u;
	} else {

		*pbuf = NULL;
	}

	return ret;
}

void buf_free(struct fs_async *afile, struct fs_async_buf **pbuf)
{
	k_mem_slab_free(&afile->_ms, (void **)pbuf);

	*pbuf = NULL;
}

#if defined(CONFIG_APP_FS_ASYNC_READ)
static void process_read(struct fs_async *afile)
{
	ssize_t ret;
	size_t req_len;
	k_spinlock_key_t key;
	struct fs_async_buf *buf;
	bool zcontinue = true;

	while (zcontinue && !atomic_test_bit(&afile->_flags, FS_ASYNC_FLAG_ABORT_BIT)) {
		/* Allocate buffer */
		ret = buf_alloc(afile, &buf);
		if (ret == -ENOMEM) {
			LOG_DBG("(%p) No more bugger available", afile);
			break;
		} else if (ret < 0) {
			LOG_ERR("(%p) buf_alloc failed ret: %d", afile, ret);
			buf->len  = ret; /* Forward error code */
			zcontinue = false;
		}

		/* Read file chunk  */
		req_len = buf->len;
		ret		= fs_read(&afile->_zfp, buf->data, req_len);
		if (ret < 0) {
			LOG_ERR("(%p) fs_read(. %p %u) -> %d", afile, buf->data, req_len, ret);
			buf->len  = ret; /* Forward error code */
			zcontinue = false;
		} else {
			buf->len = ret;
			afile->file_final_size += buf->len;

			/* Reached EOF */
			if (buf->len != req_len) {
				zcontinue = false;
			}
		}

		/* Append read block */
		key = k_spin_lock(&fs_async_lock);
		sys_slist_append(&afile->buf_q, &buf->_handle);
		k_spin_unlock(&fs_async_lock, key);

		k_sem_give(&afile->_sem);
	}

	atomic_clear_bit(&afile->_flags, FS_ASYNC_FLAG_ACTIVE_BIT);
}
#endif

#if defined(CONFIG_APP_FS_ASYNC_WRITE)
static void process_write(struct fs_async *afile)
{
	/* TODO */
}
#endif

static void async_context_clear(struct fs_async *afile)
{
	memset(afile, 0x00u, sizeof(*afile));

	afile->status = FS_ASYNC_STATUS_UNDEFINED;
}

static inline bool is_aligned_32(uint32_t data)
{
	return (data & 0x3) ? false : true;
}

int fs_async_open(struct fs_async *afile, struct fs_async_config *cfg)
{
	int ret;
	fs_mode_t mode	   = 0u;
	bool start_reading = false;
	bool get_size	   = false;
	bool truncate	   = false;

#if FS_ASYNC_ARGS_CHECK
	if (!afile || !cfg || !cfg->ms_buf || !cfg->ms_block_count || !cfg->ms_block_size ||
		!cfg->file_path || !is_aligned_32((uint32_t)afile))
		return -EINVAL;
#endif

	async_context_clear(afile);

	if (cfg->opt & FS_ASYNC_READ) {
		mode = FS_O_READ;
#if defined(CONFIG_APP_FS_ASYNC_READ)
		start_reading = true;
#endif
		if (cfg->opt & FS_ASYNC_READ_SIZE) get_size = true;
	} else if (cfg->opt & FS_ASYNC_WRITE) {
		mode = FS_O_WRITE;
		if (cfg->opt & FS_ASYNC_APPEND) mode |= FS_O_APPEND;
		if (cfg->opt & FS_ASYNC_CREATE) mode |= FS_O_CREATE;
		if (cfg->opt & FS_ASYNC_TRUNCATE) truncate = true;
	} else {
		return -ENOTSUP;
	}

	ret = k_mem_slab_init(&afile->_ms, cfg->ms_buf, cfg->ms_block_size,
						  cfg->ms_block_count);
	if (ret != 0) return ret;

	ret = k_sem_init(&afile->_sem, 0u, cfg->ms_block_count);
	if (ret != 0) return ret;

	atomic_set(&afile->_flags, 0u);

	fs_file_t_init(&afile->_zfp);
	sys_slist_init(&afile->buf_q);

	afile->file_size	   = -1;
	afile->file_final_size = 0u;
	afile->opt			   = cfg->opt;

	/* Read size of the file prior to open it */
	if (get_size) {
		/* Get file stats */
		struct fs_dirent dirent;
		ret = fs_stat(cfg->file_path, &dirent);
		if (ret == 0) {
			if (dirent.type != FS_DIR_ENTRY_FILE) {
				ret = -ENOENT;
				goto exit;
			}

			afile->file_size = dirent.size;
		} else if (ret == -ENOENT) {
			/* Forward error */
			goto exit;
		} else {
			LOG_ERR("(%p) Failed to get file stats ret: %d", afile, ret);
			goto exit;
		}
	}

	LOG_INF("(%p) Initialzed opt: %u block size: %u count: %u fsize: %d", afile, cfg->opt,
			cfg->ms_block_size, cfg->ms_block_count, afile->file_size);

	ret = fs_open(&afile->_zfp, cfg->file_path, mode);
	if (ret) {
		LOG_ERR("(%p) Failed to open %s [mode: %u] ret: %d", afile, cfg->file_path, mode,
				ret);
		goto exit;
	}

	/* Truncate file in case it already exists */
	if (truncate) {
		ret = fs_truncate(&afile->_zfp, 0u);
		if (ret != 0) {
			LOG_ERR("(%p) Failed to truncate file ret: %d", afile, ret);
			goto exit;
		}
	}

#if defined(CONFIG_APP_FS_ASYNC_READ)
	if (start_reading) {
		schedule_rw(afile);
	}
#endif

	afile->status = FS_ASYNC_STATUS_ACTIVE;

exit:
	return ret;
}

#if defined(CONFIG_APP_FS_ASYNC_READ)
int read_async(struct fs_async *afile, void *data, size_t len, k_timeout_t timeout)
{
#if FS_ASYNC_ARGS_CHECK
	if (!afile || !data || !len) return -EINVAL;
	if (afile->status != FS_ASYNC_STATUS_ACTIVE && afile->status != FS_ASYNC_STATUS_EOF)
		return -EIO;
#endif

	if (afile->status == FS_ASYNC_STATUS_EOF) return 0;

	int ret;
	k_spinlock_key_t key;
	uint32_t now, elapsed;
	bool buf_consumed; // Tells whether buffer has been complely consumed
	void *copy_from;
	size_t copy_len;

	uint32_t last			 = 0u;
	size_t rcvd_len			 = 0u;
	struct fs_async_buf *buf = NULL;
	bool timeout_calc		 = false;

	/* If timeout is an actual timeout, enable timeout calculation */
	if (!K_TIMEOUT_EQ(timeout, K_FOREVER) && !K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		last		 = k_uptime_get_32();
		timeout_calc = true;
	}

	do {
		/* Wait for a buffer to be available */
		ret = k_sem_take(&afile->_sem, timeout);
		LOG_DBG("(%p) k_sem_take timeout: %u ret: %d", afile,
				k_ticks_to_ms_floor32(timeout.ticks), ret);
		if (ret != 0) {
			goto exit;
		}

		/* First peek the buffer, without removing it from the queue */
		buf = (struct fs_async_buf *)sys_slist_peek_head(&afile->buf_q);
		__ASSERT_NO_MSG(buf != NULL);

		/* Check for error forwarded in the buffer */
		if (buf->len < 0) {
			ret = buf->len;
			goto exit;
		}

		/* Check if we have to copy all of the buffer or just a part */
		copy_len  = len - rcvd_len;
		copy_from = buf->data + buf->offset;
		LOG_DBG("(%p) buf (%p) len: %d offset: %u copy_len: %u", afile, buf, buf->len,
				(uint32_t)buf->offset, copy_len);
		if (copy_len < buf->len) {

			/* Keep buffer in the queue */
			buf->len -= copy_len;
			buf->offset += copy_len;

			buf_consumed = false;

			/* Release semaphore (for buffer not consumed) */
			k_sem_give(&afile->_sem);
		} else {
			copy_len = buf->len;

			/* Remove buffer from the queue */
			key = k_spin_lock(&fs_async_lock);
			sys_slist_remove(&afile->buf_q, NULL, &buf->_handle);
			k_spin_unlock(&fs_async_lock, key);

			buf_consumed = true;
		}

		/* Copy data */
		memcpy((uint8_t *)data + rcvd_len, copy_from, copy_len);
		rcvd_len += copy_len;

		/* If buffer is not full, we have reached the end of the file */
		if (buf_consumed && !buf_is_full(afile, buf)) {
			ret			  = rcvd_len;
			afile->status = FS_ASYNC_STATUS_EOF;
			break;
		}

		/* Return buffer to the pool */
		if (buf_consumed) buf_free(afile, &buf);

		/* As a the buffer has been released, schedule a new read operation. */
		schedule_rw(afile);

		/* Update next timeout */
		if (timeout_calc) {
			now		= k_uptime_get_32();
			elapsed = now - last;
			last	= now;

			timeout.ticks -= MIN(timeout.ticks, k_ms_to_ticks_floor32(elapsed));

			if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
				return -EAGAIN;
			}
		}
	} while ((rcvd_len < len) && !K_TIMEOUT_EQ(timeout, K_NO_WAIT));

	ret = rcvd_len;
exit:
	return ret;
}
#endif

int fs_async_read(struct fs_async *afile, void *data, size_t len, k_timeout_t timeout)
{
	int ret = 0u;
#if defined(CONFIG_APP_FS_ASYNC_READ)
	ret = read_async(afile, data, len, timeout);
#else
	ret = fs_read(&afile->_zfp, data, len);
#endif
	return ret;
}

int fs_async_write(struct fs_async *afile, void *data, size_t len, k_timeout_t timeout)
{
	int ret;

#if defined(CONFIG_APP_FS_ASYNC_WRITE)
	/* TODO */
	ret = -ENOTSUP;
#else
	ret = fs_write(&afile->_zfp, data, len);
#endif

	return ret;
}

int fs_async_close(struct fs_async *afile)
{
#if FS_ASYNC_ARGS_CHECK
	if (!afile) return -EINVAL;
	if (afile->status == FS_ASYNC_STATUS_CLOSED) return -EBADF;
#endif

	atomic_set_bit(&afile->_flags, FS_ASYNC_FLAG_ABORT);

	return fs_close(&afile->_zfp);
}

int fs_async_get_file_size(struct fs_async *afile)
{
#if FS_ASYNC_ARGS_CHECK
	if (!afile) return -EINVAL;
	if (afile->status != FS_ASYNC_STATUS_ACTIVE || afile->status != FS_ASYNC_STATUS_EOF ||
		afile->status != FS_ASYNC_STATUS_CLOSED) {
		return -EINVAL;
	}
#endif

	return afile->file_size;
}

bool fs_async_is_eof(struct fs_async *afile)
{
	return (afile->status == FS_ASYNC_STATUS_EOF);
}