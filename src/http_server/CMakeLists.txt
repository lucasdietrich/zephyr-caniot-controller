#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

SET(app_sources 
    http_server.c 
    http_utils.c 
    http_session.c 
    http_request.c 
    http_response.c 
    http_headers.c
    routes.c 
    routes_g.c
    rest_server.c 
    web_server.c 
    prometheus_client.c 
    files_server.c
)

target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_APP_HTTP_TEST_SERVER app PRIVATE test_server.c)
target_sources_ifdef(CONFIG_APP_DFU app PRIVATE dfu_server.c)