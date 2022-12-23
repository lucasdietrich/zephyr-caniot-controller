#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(core)

target_sources(app PRIVATE rest_server.c web_server.c prometheus_client.c files_server.c routes_g.c)
target_sources_ifdef(CONFIG_APP_HTTP_TEST_SERVER app PRIVATE test_server.c)
target_sources_ifdef(CONFIG_APP_DFU app PRIVATE dfu_server.c)