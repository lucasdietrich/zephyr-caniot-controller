/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/routes.h"
#include "fs/app_utils.h"
#include "utils/buffers.h"
#include "web_server.h"

#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(web_server, LOG_LEVEL_INF);

#define HTML_BEGIN_TO_TITLE "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>"
#define HTML_TITLE_TO_BODY	"</title></head><body>"
#define HTML_BODY_TO_END	"</body></html>"

#define HTML_TAG(name)	   "<" name ">"
#define HTML_END_TAG(name) "</" name ">"

#define HTML_URL(url, name) "<a href=\"" url "\">" name "</a>"
#define HTML_QUICK_URL(url) HTML_URL(url, url)
#define HTML_LI(content)	"<li>" content "</li>"

static bool index_route_iterate_cb(const struct route_descr *descr,
								   const struct route_descr *parents[],
								   size_t depth,
								   void *user_data)
{
	buffer_t *const buf = user_data;

	char url[256] = {0};

	if ((descr->flags & ROUTE_IS_LEAF_MASK) == ROUTE_IS_LEAF) {
		int written = route_build_url(url, sizeof(url), parents, depth);
		if (written >= 0) {
			snprintf(url + written, sizeof(url), "%s", descr->part.str);
		}

		buffer_snprintf(buf, "<li>%s : <a href=\"%s\">%s</a></li>",
						http_method_str(http_route_flag_to_method(descr->flags)), url,
						url);
	} else {
		// buffer_snprintf(buf, "<b>%s</b><br/>", descr->part.str);
	}

	return true;
}

int web_server_index_html(http_request_t *req, http_response_t *resp)
{
	buffer_append_string(&resp->buffer,
						 HTML_BEGIN_TO_TITLE "stm32f429zi index.html" HTML_TITLE_TO_BODY);

	buffer_append_string(&resp->buffer, "<fieldset><legend>URL list</legend><ul>");
	http_routes_iterate(index_route_iterate_cb, &resp->buffer);
	buffer_append_string(&resp->buffer, "</ul></fieldset>" HTML_BODY_TO_END);

	return 0;
}

// #define FS_PATH "/RAM:/lua"
#define FS_PATH "/RAM:"
// #define FS_PATH "/"
#define FS_ROOT "/"

bool files_iterate_cb(const char *path, struct fs_dirent *dirent, void *user_data)
{
	buffer_t *const buf = user_data;

	LOG_DBG("path=%s, name=%s, type=%d", path, dirent->name, dirent->type);

	char abspath[257];
	if (*path == '/' && (path[1u] == '\0')) {
		strcpy(abspath + 1, dirent->name);
		*abspath = '/';
	} else {
		snprintf(abspath, sizeof(abspath), "%s/%s", path, dirent->name);
	}

	if (dirent->type == FS_DIR_ENTRY_DIR) {
		buffer_snprintf(buf, "<li><b><a href=\"/fetch%s\">%s</a></b></li>", abspath,
						abspath);
	} else {
		buffer_snprintf(buf, "<li><a href=\"/files%s\">%s</a> (size = %u)</li>", abspath,
						abspath, dirent->size);
	}

	return true;
}

int web_server_files_html(http_request_t *req, http_response_t *resp)
{
	int ret;

	const char *fspath = ""; // TODO
	if (fspath == NULL || strlen(fspath) == 0) {
		fspath = FS_ROOT;
	}

	buffer_append_string(&resp->buffer,
						 HTML_BEGIN_TO_TITLE "stm32f429zi files.html" HTML_TITLE_TO_BODY);

	buffer_append_string(&resp->buffer,
						 "<fieldset><legend>Files list in " FS_PATH "</legend><ul>");
	buffer_snprintf(&resp->buffer, "<li><a href=\"/fetch%s\">.</a></li>", fspath);
	buffer_snprintf(&resp->buffer, "<li><a href=\"/fetch%s/..\">..</a></li>", fspath);

	ret = app_fs_iterate_dir_files(fspath, files_iterate_cb, &resp->buffer);

	buffer_append_string(&resp->buffer, "</ul></fieldset>" HTML_BODY_TO_END);

	return ret;
}