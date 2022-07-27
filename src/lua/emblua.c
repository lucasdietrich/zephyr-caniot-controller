#include <zephyr.h>

#include <fs/fs.h>
#include <string.h>
#include <stdio.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(app_fs, LOG_LEVEL_INF);

#include "../appfs.h"

struct emblua
{
	const char *start;
	const char *end;
	const char *name;
};

#define _EMBLUA_START_SYM(_name) \
	_binary_ ## _name ## _lua_start

#define _EMBLUA_START_END(_name) \
	_binary_ ##_name ## _lua_end

#define DECLARE_EMB_LUA_SCRIPT(_name) \
	extern uint32_t _EMBLUA_START_SYM(_name); \
	extern uint32_t _EMBLUA_START_END(_name); \
	STRUCT_SECTION_ITERABLE(emblua, _name) = { \
		.start = (const char *) &_EMBLUA_START_SYM(_name), \
		.end = (const char *) &_EMBLUA_START_END(_name), \
		.name = STRINGIFY(_name) STRINGIFY(.lua) \
	}

/* find a way to somehow automatically generate this list */
DECLARE_EMB_LUA_SCRIPT(helloworld);
DECLARE_EMB_LUA_SCRIPT(math);
DECLARE_EMB_LUA_SCRIPT(entry);

#define FS_DEFAULT_MOUNT_POINT "RAM:"
#define FS_DEFAULT_DIRECTORY "lua"

/**
 * @brief Populate the filesystem with default files.
 * 
 * @return int 
 */
int appfs_lua_populate(void)
{
	int rc = 0;
	char path[256];

	/* iterate over all lua scripts and add them to the filesystem */
	STRUCT_SECTION_FOREACH(emblua, script) {
		const char *loc = script->start;
		const size_t size = script->end - script->start;
		LOG_DBG("Adding file %s to FS, at 0x%x [ %u B ]",
			script->name, (uint32_t)loc, size);

		/* Create directory if required and doesn't exist */
		if (strlen(FS_DEFAULT_DIRECTORY) > 0) {
			ssize_t written = snprintf(path, sizeof(path), "/%s/%s",
						   FS_DEFAULT_MOUNT_POINT,
						   FS_DEFAULT_DIRECTORY);

			struct fs_dirent entry;
			rc = fs_stat(path, &entry);
			if (rc == -ENOENT) {
				rc = fs_mkdir(path);
			}

			if (rc != 0) {
				LOG_ERR("Failed to create directory %s, ret=%d",
					path, rc);
				goto exit;
			}

			snprintf(&path[written], sizeof(path) - written,
				 "/%s", script->name);
		} else {
			snprintf(path, sizeof(path), "/%s/%s",
				 FS_DEFAULT_MOUNT_POINT,
				 script->name);
		}

		rc = app_fs_file_add(path, loc, size);
		if (rc < 0) {
			LOG_ERR("Error adding file %s [%d]", path, rc);
			return rc;
		}

		LOG_INF("Script %s added -> %d", path, rc);
	}
exit:
	return rc;
}