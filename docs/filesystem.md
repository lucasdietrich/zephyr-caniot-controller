# Filesystem

Following table describe filesystems available for the application:

| Filesystem | Description                 | nucleo_f429zi | qemu |
| ---------- | --------------------------- | ------------- | -------- |
| SD:        | SDMMC FAT32 - Persistent    | yes           | no       |
| RAM:       | RAM memory FAT32 - Volatile | (yes)         | yes      |

## Files structure

## File upload/download

Files in the filesystem can be accesses using the following API: `/files`.

# Async API

Async API usage example:

```c
static void async_test_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(async_test,
		4096,
		async_test_thread,
		NULL,
		NULL,
		NULL,
		K_PRIO_PREEMPT(4u),
		0,
		3000u);

#define BLOCK_SIZE  1024u
#define BLOCK_COUNT 1u

__ALIGNED(4) static char buf[BLOCK_SIZE * BLOCK_COUNT];
__ALIGNED(4) struct fs_async afile;

static void async_test_thread(void *_a, void *_b, void *_c)
{
	int ret;

	ret = app_fs_create_big_file("/SD:/tmp/test.bin", 2 * 1024u * 1024u);
	if (ret) {
		LOG_ERR("Failed to create test file ret: %d", ret);
		return;
	}

	for (;;) {
		struct fs_async_config acfg = {
			.file_path = "/SD:/tmp/test.bin",
			.opt	   = FS_ASYNC_READ | FS_ASYNC_CREATE | FS_ASYNC_READ_SIZE,
			.ms_block_count = BLOCK_COUNT,
			.ms_block_size	= BLOCK_SIZE,
			.ms_buf		= buf,
		};

		ret = fs_async_open(&afile, &acfg);
		if (ret) {
			LOG_ERR("Failed to open file ret: %d", ret);
			break;
		}

		size_t rcvd_len = 0u;

		uint32_t start = k_uptime_get_32();

		for (;;) {
			static char data[1024u];
			ret = fs_async_read(&afile, data, sizeof(data), K_FOREVER);
			if (ret < 0) {
				LOG_ERR("Failed to read file ret: %d", ret);
				break;
			} else {
				rcvd_len += ret;

				if (ret < sizeof(data)) {
					LOG_INF("End of file rcvd_len: %u", rcvd_len);
					break;
				}
			}
		}

		uint32_t end = k_uptime_get_32();

		LOG_INF("Read %u bytes in %u ms", rcvd_len, end - start);

		fs_async_close(&afile);

		k_sleep(K_SECONDS(5));
	}
}
```

# Optimizations

Read from SD card (SPI) using Zephyr API and async API.
- nucleo_f429 read from SD using fs_read(): 2MB/3650ms
- nucleo_f429 read from SD using fs_async_read(): 2MB/4500ms
